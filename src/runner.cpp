#include "runner.h"
#include "newfile.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

Runner::Runner(const QString &backendDir, const QString &pythonHome,
               const QString &pythonExecutable, QObject *parent)
    : QObject(parent), m_backendDir(backendDir), m_pythonHome(pythonHome),
      m_pythonExecutable(pythonExecutable)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::stateChanged, this, &Runner::runningChanged);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        appendOutput(QString::fromUtf8(m_process.readAllStandardOutput()));
    });
    connect(&m_process, &QProcess::started, this, [this]() {
        if (!m_pendingInput.isEmpty()) {
            if (m_process.write(m_pendingInput) != m_pendingInput.size()) {
                appendOutput(tr("Could not send editor contents to Python.\n"));
                m_pendingInput.clear();
                m_process.kill();
                return;
            }
            m_pendingInput.clear();
        }
        m_process.closeWriteChannel();
        if (m_operation == Operation::PipInstall) {
            setStatus(tr("Installing test libraries with pip"));
        } else {
            setStatus(m_operation == Operation::Script ? tr("Running") : tr("Testing libraries"));
            m_plotTimer.start();
        }
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            setStatus(tr("Could not start Python: %1").arg(m_process.errorString()));
            m_plotTimer.stop();
            m_operation = Operation::None;
            m_testDir.reset();
        }
    });
    connect(&m_process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
        appendOutput(QString::fromUtf8(m_process.readAllStandardOutput()));
        if (m_operation == Operation::PipInstall) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0) {
                appendOutput(tr("\nDownload complete. Testing imports and plotting from the pip directory.\n"));
                m_operation = Operation::PipTest;
                startScientificTest(m_testDir->path());
            } else {
                setStatus(tr("pip installation failed or was stopped. See output."));
                m_operation = Operation::None;
                m_testDir.reset();
            }
            return;
        }
        m_plotTimer.stop();
        updatePlots();
        if (m_operation == Operation::BundledTest || m_operation == Operation::PipTest) {
            if (exitStatus == QProcess::NormalExit && exitCode == 0)
                setStatus(m_operation == Operation::PipTest
                              ? tr("pip library test passed.") : tr("Bundled library test passed."));
            else
                setStatus(m_operation == Operation::PipTest
                              ? tr("pip library test failed or was stopped. See output.")
                              : tr("Bundled library test failed or was stopped. See output."));
        } else {
            setStatus(exitStatus == QProcess::NormalExit
                          ? tr("Finished with exit code %1").arg(exitCode)
                          : tr("Process stopped or crashed."),
                      exitStatus == QProcess::NormalExit && exitCode == 0);
        }
        m_operation = Operation::None;
        m_testDir.reset();
    });
    m_plotTimer.setInterval(500);
    connect(&m_plotTimer, &QTimer::timeout, this, &Runner::updatePlots);
}

Runner::~Runner()
{
    if (running()) {
        m_process.kill();
        m_process.waitForFinished();
    }
}

void Runner::selectFile(const QUrl &url)
{
    if (running() || !url.isLocalFile())
        return;
    const QString path = url.toLocalFile();
    if (path.isEmpty())
        return;
    m_filePath = path;
    emit filePathChanged();
    setStatus(tr("Ready"));
}

void Runner::run()
{
    if (running())
        return;
    const QFileInfo file(m_filePath);
    if (m_filePath.isEmpty() || !file.isFile() || !file.isReadable()) {
        setStatus(tr("Select a readable Python file."));
        return;
    }
    if (!prepareRun())
        return;

    QProcessEnvironment env = pythonEnvironment();
    QStringList pythonPaths;
    pythonPaths << m_backendDir;
    if (!env.value(QStringLiteral("PYTHONPATH")).isEmpty())
        pythonPaths << env.value(QStringLiteral("PYTHONPATH"));
    env.insert(QStringLiteral("PYTHONPATH"), pythonPaths.join(QDir::listSeparator()));
    env.insert(QStringLiteral("MPLBACKEND"), QStringLiteral("module://aurora_plot_backend"));
    env.insert(QStringLiteral("MPLCONFIGDIR"), m_plotDir->path() + QStringLiteral("/config"));
    env.insert(QStringLiteral("AURORA_PLOT_DIR"), m_plotDir->path());
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(file.absolutePath());
    m_operation = Operation::Script;
    setStatus(tr("Starting Python"));
    m_process.start(m_pythonExecutable, QStringList() << QStringLiteral("-u") << file.absoluteFilePath());
}

bool Runner::prepareRun()
{
    if (!QFileInfo(m_pythonExecutable).isExecutable()) {
        setStatus(tr("The bundled Python executable is missing or inaccessible."));
        return false;
    }
    if (!QFileInfo::exists(m_backendDir + QStringLiteral("/aurora_plot_backend.py"))) {
        setStatus(tr("The matplotlib backend is missing from the application."));
        return false;
    }
    m_plotDir.reset(new QTemporaryDir);
    if (!m_plotDir->isValid()) {
        setStatus(tr("Could not create a temporary directory for plots."));
        return false;
    }
    m_output.clear();
    if (m_outputTruncated) {
        m_outputTruncated = false;
        emit outputTruncatedChanged();
    }
    if (!m_outputSaveError.isEmpty()) {
        m_outputSaveError.clear();
        emit outputSaveErrorChanged();
    }
    m_pendingInput.clear();
    m_plots.clear();
    emit outputChanged();
    emit plotsChanged();
    return true;
}

void Runner::runEditor(const QString &text, const QString &path)
{
    if (running() || !prepareRun())
        return;
    QString scriptPath = path;
    if (scriptPath.isEmpty()) {
        const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloads.isEmpty() || !QDir().mkpath(downloads)) {
            setStatus(tr("Could not access the Downloads directory for the unsaved script."));
            return;
        }
        scriptPath = QDir(downloads).filePath(QStringLiteral("untitled.py"));
    }
    const QFileInfo file(scriptPath);
    if (!file.dir().exists()) {
        setStatus(tr("The script's directory no longer exists."));
        return;
    }
    QProcessEnvironment env = pythonEnvironment();
    env.insert(QStringLiteral("PYTHONPATH"), m_backendDir + QDir::listSeparator()
               + env.value(QStringLiteral("PYTHONPATH")));
    env.insert(QStringLiteral("MPLBACKEND"), QStringLiteral("module://aurora_plot_backend"));
    env.insert(QStringLiteral("MPLCONFIGDIR"), m_plotDir->path() + QStringLiteral("/config"));
    env.insert(QStringLiteral("AURORA_PLOT_DIR"), m_plotDir->path());
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(file.absolutePath());
    m_pendingInput = text.toUtf8();
    m_operation = Operation::Script;
    setStatus(tr("Starting Python"));
    const QString command = QStringLiteral(
        "def _run_editor():\n"
        " import os, sys\n"
        " path = sys.argv[1]\n"
        " sys.argv = [path]\n"
        " sys.path[0] = os.path.dirname(os.path.abspath(path))\n"
        " source = sys.stdin.buffer.read().decode('utf-8')\n"
        " sys.stdin = open(os.devnull)\n"
        " code = compile(source, path, 'exec')\n"
        " builtins = __builtins__\n"
        " main = sys.modules['__main__'].__dict__\n"
        " main.clear()\n"
        " main.update(__name__='__main__', __file__=path, __package__=None, __cached__=None, __builtins__=builtins)\n"
        " exec(code, main)\n"
        "_run_editor()\n");
    m_process.start(m_pythonExecutable, QStringList() << QStringLiteral("-u") << QStringLiteral("-c")
                    << command << file.absoluteFilePath());
}

void Runner::startScientificTest(const QString &packageDir)
{
    const QString script = QDir(m_backendDir).filePath(QStringLiteral("scientific_check.py"));
    if (!QFileInfo(script).isReadable()) {
        setStatus(tr("The scientific library test is missing from the application."));
        m_operation = Operation::None;
        m_testDir.reset();
        return;
    }
    QProcessEnvironment env = pythonEnvironment();
    env.insert(QStringLiteral("PYTHONPATH"), packageDir);
    env.insert(QStringLiteral("MPLBACKEND"), QStringLiteral("module://aurora_plot_backend"));
    env.insert(QStringLiteral("MPLCONFIGDIR"), m_plotDir->path() + QStringLiteral("/config"));
    env.insert(QStringLiteral("AURORA_PLOT_DIR"), m_plotDir->path());
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(m_plotDir->path());
    setStatus(tr("Starting library test"));
    m_process.start(m_pythonExecutable, QStringList() << QStringLiteral("-u") << QStringLiteral("-S")
                    << script << packageDir);
}

void Runner::testBundledLibraries()
{
    if (running() || !prepareRun())
        return;
    m_operation = Operation::BundledTest;
    startScientificTest(QDir(m_pythonHome).filePath(QStringLiteral("lib/site-packages")));
}

void Runner::testPipLibraries()
{
    if (running() || !prepareRun())
        return;
    if (!QFileInfo(QDir(m_backendDir).filePath(QStringLiteral("scientific_check.py"))).isReadable()) {
        setStatus(tr("The scientific library test is missing from the application."));
        return;
    }
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (dataDir.isEmpty() || !QDir().mkpath(dataDir)) {
        setStatus(tr("Could not create the application data directory."));
        return;
    }
    m_testDir.reset(new QTemporaryDir(dataDir + QStringLiteral("/pip-test-XXXXXX")));
    if (!m_testDir->isValid()) {
        m_testDir.reset();
        setStatus(tr("Could not create the pip test directory."));
        return;
    }
    QProcessEnvironment env = pythonEnvironment();
    env.insert(QStringLiteral("PYTHONPATH"), QDir(m_pythonHome).filePath(QStringLiteral("lib/pip.whl")));
    env.insert(QStringLiteral("PYTHONUNBUFFERED"), QStringLiteral("1"));
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(m_testDir->path());
    m_operation = Operation::PipInstall;
    setStatus(tr("Starting pip test"));
    m_process.start(m_pythonExecutable, QStringList()
                    << QStringLiteral("-u") << QStringLiteral("-S") << QStringLiteral("-m")
                    << QStringLiteral("pip") << QStringLiteral("--isolated") << QStringLiteral("install")
                    << QStringLiteral("--disable-pip-version-check") << QStringLiteral("--no-input")
                    << QStringLiteral("--no-cache-dir") << QStringLiteral("--progress-bar") << QStringLiteral("off")
                    << QStringLiteral("--only-binary=:all:") << QStringLiteral("--ignore-installed")
                    << QStringLiteral("--target") << m_testDir->path()
                    << QStringLiteral("numpy==2.4.6") << QStringLiteral("matplotlib==3.10.8")
                    << QStringLiteral("contourpy==1.3.3") << QStringLiteral("cycler==0.12.1")
                    << QStringLiteral("fonttools==4.68.0") << QStringLiteral("kiwisolver==1.5.1")
                    << QStringLiteral("packaging==26.3") << QStringLiteral("pillow==13.0.0")
                    << QStringLiteral("pyparsing==3.3.3") << QStringLiteral("python-dateutil==4.0.0")
                    << QStringLiteral("tzdata==2026.6"));
}

QProcessEnvironment Runner::pythonEnvironment() const
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString libDir = QDir(m_pythonHome).filePath(QStringLiteral("lib"));
    const QString existing = env.value(QStringLiteral("LD_LIBRARY_PATH"));
    env.insert(QStringLiteral("LD_LIBRARY_PATH"), libDir
               + (existing.isEmpty() ? QString() : QDir::listSeparator() + existing));
    env.insert(QStringLiteral("PYTHONHOME"), m_pythonHome);
    env.insert(QStringLiteral("PYTHONNOUSERSITE"), QStringLiteral("1"));
    const QString existingPythonPath = env.value(QStringLiteral("PYTHONPATH"));
    env.insert(QStringLiteral("PYTHONPATH"), QDir(libDir).filePath(QStringLiteral("site-packages"))
               + QDir::listSeparator() + QDir(libDir).filePath(QStringLiteral("pip.whl"))
               + (existingPythonPath.isEmpty() ? QString() : QDir::listSeparator() + existingPythonPath));
    return env;
}

void Runner::stop()
{
    if (running())
        m_process.kill();
}

bool Runner::saveOutput(const QString &name)
{
    QString path, error;
    if (m_output.isEmpty())
        error = tr("There is no output to save.");
    else
        writeNewDownload(name, QStringLiteral(".txt"), m_output.toUtf8(), &path, &error);
    if (m_outputSaveError != error) {
        m_outputSaveError = error;
        emit outputSaveErrorChanged();
    }
    if (!error.isEmpty())
        return false;
    emit outputSaved(QFileInfo(path).fileName());
    return true;
}

void Runner::appendOutput(const QString &text)
{
    if (text.isEmpty())
        return;
    m_output += text;
    const int limit = 1024 * 1024;
    if (m_output.size() > limit) {
        m_output.remove(0, m_output.size() - limit);
        if (!m_outputTruncated) {
            m_outputTruncated = true;
            emit outputTruncatedChanged();
        }
    }
    emit outputChanged();
}

void Runner::setStatus(const QString &text, bool finishedSuccessfully)
{
    if (m_status == text && m_finishedSuccessfully == finishedSuccessfully)
        return;
    m_status = text;
    m_finishedSuccessfully = finishedSuccessfully;
    emit statusChanged();
}

void Runner::updatePlots()
{
    if (!m_plotDir)
        return;
    const QDir dir(m_plotDir->path());
    const QStringList files = dir.entryList(QStringList() << QStringLiteral("plot-*.png"),
                                             QDir::Files, QDir::Name);
    QVariantList plots;
    for (const QString &file : files)
        plots.append(QUrl::fromLocalFile(dir.absoluteFilePath(file)));
    if (plots != m_plots) {
        m_plots = plots;
        emit plotsChanged();
    }
}
