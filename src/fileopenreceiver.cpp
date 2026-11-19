#include "fileopenreceiver.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QRegularExpression>
#include <RuntimeManager/RuntimeDispatcher>

FileOpenReceiver::FileOpenReceiver(QObject *parent) : QObject(parent) {}

FileOpenReceiver::~FileOpenReceiver()
{
    if (m_started) {
        RuntimeManager::RuntimeDispatcher::instance()->unregisterIntent(QStringLiteral("OpenURI"));
        RuntimeManager::RuntimeDispatcher::instance()->stopService();
    }
}

void FileOpenReceiver::setError(const QString &error)
{
    if (m_error == error)
        return;
    m_error = error;
    emit errorChanged();
}

bool FileOpenReceiver::start()
{
    if (m_started)
        return true;
    auto *dispatcher = RuntimeManager::RuntimeDispatcher::instance();
    dispatcher->registerIntent(QStringLiteral("OpenURI"),
        [this](const QJsonObject &params, const RuntimeManager::RuntimeDispatcher::HandlerCb &reply) {
            const QJsonValue uri = params.value(QStringLiteral("uri"));
            if (uri.isString())
                request(uri.toString());
            else
                setError(tr("The file request does not contain a valid URI."));
            reply(QJsonObject());
        });
    const RuntimeManager::Error error = dispatcher->startService();
    if (error) {
        dispatcher->unregisterIntent(QStringLiteral("OpenURI"));
        m_serviceError = tr("Could not register the file-open service: %1").arg(error.message());
        setError(m_serviceError);
        qWarning("Could not register the file-open service: %s", qPrintable(error.message()));
        return false;
    }
    m_started = true;
    m_serviceError.clear();
    setError(QString());
    return true;
}

void FileOpenReceiver::requestStartupArgument(const QString &argument)
{
    if (argument != QLatin1String("%u"))
        request(argument);
}

void FileOpenReceiver::request(const QString &argument)
{
    static const QRegularExpression scheme(QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]*:"));
    if (argument.isEmpty() || argument.size() > 4096) {
        setError(tr("The file request is empty or too long."));
        return;
    }
    const QUrl url = scheme.match(argument).hasMatch()
            ? QUrl(argument)
            : QUrl::fromLocalFile(QFileInfo(argument).absoluteFilePath());
    if (!url.isValid() || !url.isLocalFile()
            || (!url.host().isEmpty() && url.host() != QLatin1String("localhost"))) {
        setError(tr("Only local file URLs can be opened."));
        return;
    }
    setError(m_serviceError);
    emit requested(url);
}
