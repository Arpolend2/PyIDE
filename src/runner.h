#pragma once

#include <QObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTimer>
#include <QUrl>
#include <QVariant>
#include <memory>

class Runner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString filePath READ filePath NOTIFY filePathChanged)
    Q_PROPERTY(QString output READ output NOTIFY outputChanged)
    Q_PROPERTY(bool outputTruncated READ outputTruncated NOTIFY outputTruncatedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool finishedSuccessfully READ finishedSuccessfully NOTIFY statusChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QVariantList plots READ plots NOTIFY plotsChanged)
    Q_PROPERTY(QString outputSaveError READ outputSaveError NOTIFY outputSaveErrorChanged)

public:
    explicit Runner(const QString &backendDir, const QString &pythonHome,
                    const QString &pythonExecutable, QObject *parent = nullptr);
    ~Runner() override;
    QString filePath() const { return m_filePath; }
    QString output() const { return m_output; }
    bool outputTruncated() const { return m_outputTruncated; }
    QString status() const { return m_status; }
    bool finishedSuccessfully() const { return m_finishedSuccessfully; }
    bool running() const { return m_process.state() != QProcess::NotRunning; }
    QVariantList plots() const { return m_plots; }
    QString outputSaveError() const { return m_outputSaveError; }

    Q_INVOKABLE void selectFile(const QUrl &url);
    Q_INVOKABLE void run();
    Q_INVOKABLE void runEditor(const QString &text, const QString &path);
    Q_INVOKABLE void testBundledLibraries();
    Q_INVOKABLE void testPipLibraries();
    Q_INVOKABLE void stop();
    Q_INVOKABLE bool saveOutput(const QString &name);

signals:
    void filePathChanged();
    void outputChanged();
    void outputTruncatedChanged();
    void statusChanged();
    void runningChanged();
    void plotsChanged();
    void outputSaveErrorChanged();
    void outputSaved(const QString &fileName);

private:
    enum class Operation { None, Script, BundledTest, PipInstall, PipTest };
    bool prepareRun();
    void startScientificTest(const QString &packageDir);
    void appendOutput(const QString &text);
    void setStatus(const QString &text, bool finishedSuccessfully = false);
    void updatePlots();
    QProcessEnvironment pythonEnvironment() const;

    QString m_backendDir;
    QString m_pythonHome;
    QString m_pythonExecutable;
    QString m_filePath;
    QString m_output;
    bool m_outputTruncated = false;
    QString m_status;
    bool m_finishedSuccessfully = false;
    QString m_outputSaveError;
    QByteArray m_pendingInput;
    QVariantList m_plots;
    QProcess m_process;
    QTimer m_plotTimer;
    std::unique_ptr<QTemporaryDir> m_plotDir;
    std::unique_ptr<QTemporaryDir> m_testDir;
    Operation m_operation = Operation::None;
};
