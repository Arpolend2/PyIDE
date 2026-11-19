#include "editordocument.h"
#include "newfile.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QTextCodec>

EditorDocument::EditorDocument(QObject *parent) : QObject(parent)
{
    m_cacheTimer.setSingleShot(true);
    m_cacheTimer.setInterval(500);
    connect(&m_cacheTimer, &QTimer::timeout, this, &EditorDocument::flushCache);
}

EditorDocument::~EditorDocument()
{
    if (m_cacheDirty)
        flushCache();
}

void EditorDocument::enableCache(const QString &path)
{
    m_cachePath = path;
    if (path.isEmpty())
        setCacheError(tr("Could not locate the editor cache directory."));
}

void EditorDocument::setCacheError(const QString &error)
{
    if (m_cacheError == error)
        return;
    m_cacheError = error;
    emit cacheErrorChanged();
}

void EditorDocument::scheduleCache()
{
    if (!m_cachePath.isEmpty()) {
        m_cacheDirty = true;
        m_cacheTimer.start();
    }
}

void EditorDocument::flushCache()
{
    if (m_cachePath.isEmpty() || !m_cacheDirty)
        return;
    m_cacheTimer.stop();
    if (!QDir().mkpath(QFileInfo(m_cachePath).absolutePath())) {
        setCacheError(tr("Could not create the editor cache directory."));
        return;
    }
    QJsonObject data;
    data.insert(QStringLiteral("version"), 1);
    data.insert(QStringLiteral("path"), m_path);
    data.insert(QStringLiteral("text"), m_text);
    data.insert(QStringLiteral("savedText"), m_savedText);
    data.insert(QStringLiteral("savedHash"), QString::fromLatin1(m_savedHash.toHex()));
    data.insert(QStringLiteral("lineEnding"), QString::fromLatin1(m_lineEnding.toHex()));
    data.insert(QStringLiteral("bom"), m_bom);
    QSaveFile file(m_cachePath);
    if (!file.open(QIODevice::WriteOnly)) {
        setCacheError(tr("Could not save the editor cache: %1").arg(file.errorString()));
        return;
    }
    const QByteArray bytes = QJsonDocument(data).toJson(QJsonDocument::Compact);
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        setCacheError(tr("Could not save the editor cache: %1").arg(file.errorString()));
        return;
    }
    m_cacheDirty = false;
    setCacheError(QString());
}

bool EditorDocument::restoreCache()
{
    if (m_cachePath.isEmpty() || !QFileInfo::exists(m_cachePath))
        return true;
    QFile file(m_cachePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setCacheError(tr("Could not read the editor cache: %1").arg(file.errorString()));
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setCacheError(tr("Could not read the editor cache: %1").arg(file.errorString()));
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(bytes, &parseError);
    const QJsonObject data = json.object();
    const QString path = data.value(QStringLiteral("path")).toString();
    const QString hash = data.value(QStringLiteral("savedHash")).toString();
    const QString lineEnding = data.value(QStringLiteral("lineEnding")).toString();
    if (parseError.error != QJsonParseError::NoError || !json.isObject()
            || data.value(QStringLiteral("version")).toInt() != 1
            || !data.value(QStringLiteral("path")).isString()
            || !data.value(QStringLiteral("text")).isString()
            || !data.value(QStringLiteral("savedText")).isString()
            || !data.value(QStringLiteral("savedHash")).isString()
            || !data.value(QStringLiteral("bom")).isBool()
            || (lineEnding != QLatin1String("0a") && lineEnding != QLatin1String("0d0a"))
            || (!path.isEmpty() && (!QDir::isAbsolutePath(path) || hash.size() != 64))
            || (path.isEmpty() && !hash.isEmpty())
            || (!hash.isEmpty() && QByteArray::fromHex(hash.toLatin1()).size() != 32)) {
        setCacheError(tr("The editor cache is invalid; it has not been overwritten."));
        return false;
    }
    const QString text = data.value(QStringLiteral("text")).toString();
    const QString savedText = data.value(QStringLiteral("savedText")).toString();
    if (!path.isEmpty() && text == savedText && openFile(QUrl::fromLocalFile(path), false)) {
        setCacheError(QString());
        return true;
    }
    const bool wasDirty = dirty();
    m_path = path;
    m_text = text;
    m_savedText = savedText;
    m_savedHash = QByteArray::fromHex(hash.toLatin1());
    m_lineEnding = QByteArray::fromHex(lineEnding.toLatin1());
    m_bom = data.value(QStringLiteral("bom")).toBool();
    setError(QString());
    emit textChanged();
    emit filePathChanged();
    if (wasDirty != dirty())
        emit dirtyChanged();
    if (!path.isEmpty()) {
        QFile original(path);
        if (!original.open(QIODevice::ReadOnly)
                || QCryptographicHash::hash(original.readAll(), QCryptographicHash::Sha256) != m_savedHash
                || original.error() != QFileDevice::NoError) {
            setCacheError(tr("The original file is unavailable or changed. Cached edits were restored; saving will not overwrite the changed file."));
            return true;
        }
    }
    setCacheError(QString());
    return true;
}

QString EditorDocument::fileName() const
{
    return m_path.isEmpty() ? tr("Untitled.py") : QFileInfo(m_path).fileName();
}

void EditorDocument::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

void EditorDocument::setText(const QString &text)
{
    if (m_text == text)
        return;
    const bool wasDirty = dirty();
    m_text = text;
    scheduleCache();
    emit textChanged();
    if (wasDirty != dirty())
        emit dirtyChanged();
}

bool EditorDocument::openFile(const QUrl &url, bool discardChanges)
{
    if (dirty() && !discardChanges) {
        setError(tr("Save or discard your changes before opening another file."));
        return false;
    }
    if (!url.isLocalFile()) {
        setError(tr("Choose a local Python file."));
        return false;
    }
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) {
        setError(tr("Could not open file %1: %2").arg(file.fileName(), file.errorString()));
        return false;
    }
    const QByteArray bytes = file.readAll();
    if (file.error() != QFileDevice::NoError) {
        setError(tr("Could not read file: %1").arg(file.errorString()));
        return false;
    }
    QByteArray content = bytes;
    const bool bom = content.startsWith("\xef\xbb\xbf");
    if (bom)
        content.remove(0, 3);
    QTextCodec::ConverterState state;
    QString text = QTextCodec::codecForName("UTF-8")->toUnicode(content.constData(), content.size(), &state);
    if (state.invalidChars || state.remainingChars) {
        setError(tr("This file is not valid UTF-8 and cannot be edited safely."));
        return false;
    }
    const QByteArray lineEnding = content.contains("\r\n") ? QByteArray("\r\n") : QByteArray("\n");
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    const bool wasDirty = dirty();
    m_text = text;
    m_savedText = text;
    m_path = QFileInfo(file.fileName()).absoluteFilePath();
    m_savedHash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    m_lineEnding = lineEnding;
    m_bom = bom;
    scheduleCache();
    setError(QString());
    emit textChanged();
    emit filePathChanged();
    if (wasDirty)
        emit dirtyChanged();
    return true;
}

bool EditorDocument::newFile(bool discardChanges)
{
    if (dirty() && !discardChanges) {
        setError(tr("Save or discard your changes before creating a file."));
        return false;
    }
    const bool wasDirty = dirty();
    m_text.clear();
    m_savedText.clear();
    m_path.clear();
    m_savedHash.clear();
    m_lineEnding = "\n";
    m_bom = false;
    scheduleCache();
    setError(QString());
    emit textChanged();
    emit filePathChanged();
    if (wasDirty)
        emit dirtyChanged();
    return true;
}

QByteArray EditorDocument::encodedText() const
{
    QByteArray bytes = m_text.toUtf8();
    if (m_lineEnding != "\n")
        bytes.replace("\n", m_lineEnding);
    if (m_bom)
        bytes.prepend("\xef\xbb\xbf");
    return bytes;
}

void EditorDocument::markSaved(const QString &path, const QByteArray &bytes)
{
    const bool wasDirty = dirty();
    const bool pathChanged = m_path != path;
    m_path = path;
    m_savedText = m_text;
    m_savedHash = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
    scheduleCache();
    setError(QString());
    if (pathChanged)
        emit filePathChanged();
    if (wasDirty)
        emit dirtyChanged();
    emit saved(fileName());
}

bool EditorDocument::save()
{
    if (m_path.isEmpty()) {
        setError(tr("Choose a name for the new file."));
        return false;
    }
    QFile original(m_path);
    if (!original.open(QIODevice::ReadOnly)) {
        setError(tr("Could not read the file before saving: %1").arg(original.errorString()));
        return false;
    }
    const QByteArray current = original.readAll();
    if (original.error() != QFileDevice::NoError) {
        setError(tr("Could not read the file before saving: %1").arg(original.errorString()));
        return false;
    }
    if (QCryptographicHash::hash(current, QCryptographicHash::Sha256) != m_savedHash) {
        setError(tr("The file changed outside the editor. Your changes have not been saved."));
        return false;
    }
    original.close();
    if (!dirty()) {
        setError(QString());
        emit saved(fileName());
        return true;
    }
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(tr("Could not save file: %1").arg(file.errorString()));
        return false;
    }
    const QByteArray bytes = encodedText();
    if (file.write(bytes) != bytes.size() || !file.commit()) {
        setError(tr("Could not save file: %1").arg(file.errorString()));
        return false;
    }
    markSaved(m_path, bytes);
    return true;
}

bool EditorDocument::saveNew(const QString &name)
{
    const QByteArray bytes = encodedText();
    QString path, error;
    if (!writeNewDownload(name, QStringLiteral(".py"), bytes, &path, &error)) {
        setError(error);
        return false;
    }
    markSaved(path, bytes);
    return true;
}
