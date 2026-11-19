#pragma once

#include <QObject>
#include <QUrl>

class FileOpenReceiver : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit FileOpenReceiver(QObject *parent = nullptr);
    ~FileOpenReceiver() override;
    QString error() const { return m_error; }
    bool start();
    void requestStartupArgument(const QString &argument);
    Q_INVOKABLE void request(const QString &argument);

signals:
    void requested(const QUrl &url);
    void errorChanged();

private:
    void setError(const QString &error);
    QString m_error;
    QString m_serviceError;
    bool m_started = false;
};
