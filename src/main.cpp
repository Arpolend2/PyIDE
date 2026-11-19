#include "runner.h"
#include "editordocument.h"
#include "pythonhighlighter.h"
#include "textedithelper.h"
#include "fileopenreceiver.h"

#include <auroraapp.h>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QScopedPointer>
#include <QDir>
#include <QStandardPaths>
#include <QLocale>
#include <QTranslator>
#include <QtQml>

int main(int argc, char **argv)
{
    QScopedPointer<QGuiApplication> app(Aurora::Application::application(argc, argv));
    app->setOrganizationName(QStringLiteral("ru.arpolend"));
    app->setApplicationName(QStringLiteral("PyIDE"));
    app->setApplicationDisplayName(QStringLiteral("PyIDE"));
    app->setApplicationVersion(QStringLiteral(APP_VERSION));

    QTranslator translator;
    const QLocale locale = QLocale::system();
    if (locale.language() == QLocale::Russian) {
        const QString translations = Aurora::Application::pathTo(QStringLiteral("translations")).toLocalFile();
        if (translator.load(locale, QStringLiteral("PythonRunner"), QStringLiteral("_"), translations))
            app->installTranslator(&translator);
        else
            qWarning("Could not load Russian application translations");
    }

    qmlRegisterType<PythonHighlighter>("PythonRunner", 1, 0, "PythonHighlighter");
    qmlRegisterType<TextEditHelper>("PythonRunner", 1, 0, "TextEditHelper");
    EditorDocument document;
    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!dataPath.isEmpty()) {
        document.enableCache(QDir(dataPath).filePath(QStringLiteral("editor-cache.json")));
        document.restoreCache();
        QObject::connect(app.data(), &QCoreApplication::aboutToQuit, &document, &EditorDocument::flushCache);
    } else {
        document.enableCache(QString());
        qWarning("Could not locate the application data directory for the editor cache.");
    }
    FileOpenReceiver fileOpenReceiver;
    Runner runner(Aurora::Application::pathTo(QStringLiteral("python")).toLocalFile(),
                  Aurora::Application::pathTo(QString()).toLocalFile(),
                  QStringLiteral(PYTHON_EXECUTABLE_PATH));
    QScopedPointer<QQuickView> view(Aurora::Application::createView());
    view->rootContext()->setContextProperty(QStringLiteral("runner"), &runner);
    view->rootContext()->setContextProperty(QStringLiteral("editorDocument"), &document);
    view->rootContext()->setContextProperty(QStringLiteral("fileOpenReceiver"), &fileOpenReceiver);
    view->setSource(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (view->status() == QQuickView::Error)
        return 1;
    fileOpenReceiver.start();
    view->show();
    const QStringList arguments = QCoreApplication::arguments();
    if (arguments.size() > 1)
        fileOpenReceiver.requestStartupArgument(arguments.at(1));
    return app->exec();
}
