#pragma once

#include <QObject>
#include <QTimer>
#include <QUrl>

class EditorDocument : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY filePathChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY dirtyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString cacheError READ cacheError NOTIFY cacheErrorChanged)
public:
    explicit EditorDocument(QObject *parent = nullptr);
    ~EditorDocument() override;
    QString text() const { return m_text; }
    QString filePath() const { return m_path; }
    QString fileName() const;
    bool dirty() const { return m_text != m_savedText; }
    QString error() const { return m_error; }
    QString cacheError() const { return m_cacheError; }
    void setText(const QString &text);
    void enableCache(const QString &path);
    bool restoreCache();
    void flushCache();

    Q_INVOKABLE bool openFile(const QUrl &url, bool discardChanges = false);
    Q_INVOKABLE bool newFile(bool discardChanges = false);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveNew(const QString &name);

signals:
    void textChanged();
    void filePathChanged();
    void dirtyChanged();
    void errorChanged();
    void cacheErrorChanged();
    void saved(const QString &fileName);

private:
    void setError(const QString &error);
    void setCacheError(const QString &error);
    void scheduleCache();
    QByteArray encodedText() const;
    void markSaved(const QString &path, const QByteArray &bytes);

    QString m_text;
    QString m_savedText;
    QString m_path;
    QString m_error;
    QByteArray m_savedHash;
    QByteArray m_lineEnding = "\n";
    bool m_bom = false;
    QString m_cachePath;
    QString m_cacheError;
    QTimer m_cacheTimer;
    bool m_cacheDirty = false;
};
