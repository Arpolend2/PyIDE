#include "editordocument.h"
#include "pythonhighlighter.h"
#include "textedithelper.h"
#include "runner.h"
#include "fileopenreceiver.h"
#include "newfile.h"

#include <QDir>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QFile>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickTextDocument>
#include <QTemporaryDir>
#include <QTranslator>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTextDocument>
#include <QUuid>

class EditorTests : public QObject
{
    Q_OBJECT
private slots:
    void document();
    void cache();
    void highlightingAndEdits();
    void runEditor();
    void fileRequests();
    void russianTranslation();
    void qmlLoads();
};

void EditorTests::russianTranslation()
{
    QTranslator translator;
    QVERIFY(translator.load(QLocale(QStringLiteral("ru_RU")), QStringLiteral("PythonRunner"),
                            QStringLiteral("_"), QCoreApplication::applicationDirPath()));
    QVERIFY(QCoreApplication::installTranslator(&translator));
    {
        EditorDocument document;
        QCOMPARE(document.fileName(), QString::fromUtf8("Без имени.py"));
        QString path;
        QString error;
        QVERIFY(!writeNewDownload(QString(), QStringLiteral(".py"), QByteArray(), &path, &error));
        QCOMPARE(error, QString::fromUtf8("Введите имя файла без пути."));
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData("import QtQuick 2.5\nQtObject { property string label: qsTranslate(\"EditorPage\", \"Run\") }",
                          QUrl());
        QScopedPointer<QObject> object(component.create());
        QVERIFY2(object, qPrintable(component.errorString()));
        QCOMPARE(object->property("label").toString(), QString::fromUtf8("Запустить"));
        QCOMPARE(QCoreApplication::translate("ResultsPage", "Output"), QString::fromUtf8("Вывод"));
        QCOMPARE(QCoreApplication::translate("EditorPage", "Python editor with NumPy and Matplotlib"),
                 QString::fromUtf8("Редактор Python с NumPy и Matplotlib"));
    }
    QVERIFY(QCoreApplication::removeTranslator(&translator));
    QCOMPARE(QCoreApplication::translate("EditorPage", "Run"), QStringLiteral("Run"));
}

void EditorTests::document()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = QDir(dir.path()).filePath(QStringLiteral("test.py"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    QCOMPARE(file.write("\xef\xbb\xbfprint(1)\r\n"), qint64(13));
    file.close();
    EditorDocument doc;
    const QString missingPath = QDir(dir.path()).filePath(QStringLiteral("missing.py"));
    QVERIFY(!doc.openFile(QUrl::fromLocalFile(missingPath), false));
    QVERIFY(doc.error().contains(missingPath));
    QSignalSpy saved(&doc, SIGNAL(saved(QString)));
    QVERIFY(doc.openFile(QUrl::fromLocalFile(path), false));
    QCOMPARE(saved.count(), 0);
    QCOMPARE(doc.text(), QStringLiteral("print(1)\n"));
    QVERIFY(!doc.dirty());
    doc.setText(QStringLiteral("print(2)\n"));
    QVERIFY(doc.dirty());
    QVERIFY(!doc.newFile(false));
    QVERIFY(doc.save());
    QCOMPARE(saved.count(), 1);
    QCOMPARE(saved.takeFirst().at(0).toString(), QStringLiteral("test.py"));
    QVERIFY(doc.save());
    QCOMPARE(saved.count(), 1);
    saved.clear();
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("\xef\xbb\xbfprint(2)\r\n"));
    file.close();
    doc.setText(QStringLiteral("print(3)\n"));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("external");
    file.close();
    QVERIFY(!doc.save());
    QCOMPARE(saved.count(), 0);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("external"));
    file.close();
    QVERIFY(doc.newFile(true));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("\xe2\x82", 2);
    file.close();
    QVERIFY(!doc.openFile(QUrl::fromLocalFile(path), false));
}

void EditorTests::cache()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = QDir(dir.path()).filePath(QStringLiteral("source.py"));
    const QString cachePath = QDir(dir.path()).filePath(QStringLiteral("cache.json"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("saved\n");
    file.close();
    {
        EditorDocument doc;
        doc.enableCache(cachePath);
        QVERIFY(doc.openFile(QUrl::fromLocalFile(path), false));
        doc.setText(QStringLiteral("unsaved\n"));
        QTRY_VERIFY(QFile::exists(cachePath));
    }
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    file.write("external\n");
    file.close();
    {
        EditorDocument doc;
        doc.enableCache(cachePath);
        QVERIFY(doc.restoreCache());
        QCOMPARE(doc.filePath(), path);
        QCOMPARE(doc.text(), QStringLiteral("unsaved\n"));
        QVERIFY(doc.dirty());
        QVERIFY(!doc.cacheError().isEmpty());
        QVERIFY(!doc.save());
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("external\n"));
        file.close();
        doc.setText(QStringLiteral("second edit"));
        doc.flushCache();
    }
    EditorDocument restored;
    restored.enableCache(cachePath);
    QVERIFY(restored.restoreCache());
    QCOMPARE(restored.text(), QStringLiteral("second edit"));
    QVERIFY(restored.dirty());
}

void EditorTests::fileRequests()
{
    FileOpenReceiver receiver;
    QSignalSpy requests(&receiver, SIGNAL(requested(QUrl)));
    receiver.requestStartupArgument(QStringLiteral("%u"));
    QCOMPARE(requests.count(), 0);
    QVERIFY(receiver.error().isEmpty());
    receiver.requestStartupArgument(QStringLiteral("file:///tmp/startup%20file.py"));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.takeFirst().at(0).toUrl().toLocalFile(), QStringLiteral("/tmp/startup file.py"));
    receiver.request(QStringLiteral("/tmp/a b.py"));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.takeFirst().at(0).toUrl().toLocalFile(), QStringLiteral("/tmp/a b.py"));
    receiver.request(QStringLiteral("file:///tmp/a%20b.py"));
    QCOMPARE(requests.takeFirst().at(0).toUrl().toLocalFile(), QStringLiteral("/tmp/a b.py"));
    receiver.request(QStringLiteral("file:///tmp/a b.py"));
    QCOMPARE(requests.takeFirst().at(0).toUrl().toLocalFile(), QStringLiteral("/tmp/a b.py"));
    receiver.request(QStringLiteral("https://example.test/a.py"));
    QCOMPARE(requests.count(), 0);
    QVERIFY(!receiver.error().isEmpty());
    receiver.request(QStringLiteral("file://remote/a.py"));
    QCOMPARE(requests.count(), 0);
    QVERIFY(receiver.start());
    QDBusConnection bus = QDBusConnection::sessionBus();
    QDBusMessage intent = QDBusMessage::createMethodCall(
        QStringLiteral("ru.arpolend.PyIDE"),
        QStringLiteral("/ru/omp/RuntimeManager/IntentHandler1"),
        QStringLiteral("ru.omp.RuntimeManager.IntentHandler1"),
        QStringLiteral("HandleIntent"));
    QVariantMap params;
    params.insert(QStringLiteral("uri"), QStringLiteral("file:///tmp/from%20intent.py"));
    intent << QStringLiteral("OpenURI") << params;
    QDBusPendingCallWatcher watcher(bus.asyncCall(intent));
    QTRY_VERIFY(watcher.isFinished());
    const QDBusMessage reply = watcher.reply();
    QVERIFY2(reply.type() != QDBusMessage::ErrorMessage, qPrintable(reply.errorMessage()));
    QCOMPARE(requests.count(), 1);
    QCOMPARE(requests.takeFirst().at(0).toUrl().toLocalFile(), QStringLiteral("/tmp/from intent.py"));
}

void EditorTests::highlightingAndEdits()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\nTextEdit {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    QQuickItem *item = qobject_cast<QQuickItem *>(object.data());
    QVERIFY(item);
    item->setProperty("text", QStringLiteral("print('hello') # comment\n'''one\ntwo'''\npri"));
    auto quickDoc = item->property("textDocument").value<QQuickTextDocument *>();
    QVERIFY(quickDoc);
    PythonHighlighter highlighter;
    highlighter.setTextDocument(quickDoc);
    QVERIFY(highlighter.isCodePosition(0));
    QVERIFY(!highlighter.isCodePosition(7));
    QVERIFY(!highlighter.isCodePosition(17));
    QVERIFY(!highlighter.isCodePosition(34));
    QVERIFY(highlighter.completions(QStringLiteral("pri")).contains(QStringLiteral("print")));
    QCOMPARE(highlighter.completions(QStringLiteral("pri")).first(), QStringLiteral("print()"));
    QVERIFY(highlighter.completions(QStringLiteral("for")).contains(QStringLiteral("for item in items:")));
    TextEditHelper helper;
    helper.setTarget(item);
    QTextDocument *doc = quickDoc->textDocument();
    const QString old = doc->toPlainText();
    QVERIFY(helper.replace(old.length() - 3, old.length(), QStringLiteral("print")));
    QVERIFY(doc->toPlainText().endsWith(QStringLiteral("print")));
    doc->undo();
    QCOMPARE(doc->toPlainText(), old);
    QVERIFY(helper.replace(0, old.length(), QString()));
    QCOMPARE(doc->toPlainText(), QString());
    doc->undo();
    QCOMPARE(doc->toPlainText(), old);
}

void EditorTests::runEditor()
{
    const QString home = QString::fromLocal8Bit(qgetenv("TEST_PYTHON_HOME"));
    QVERIFY(!home.isEmpty());
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile module(QDir(dir.path()).filePath(QStringLiteral("localmod.py")));
    QVERIFY(module.open(QIODevice::WriteOnly));
    module.write("value = 42\n");
    module.close();
    Runner runner(QStringLiteral("/src/python"), home, home + QStringLiteral("/bin/python3.11"));
    QVERIFY(runner.status().isEmpty());
    const QString path = QDir(dir.path()).filePath(QStringLiteral("script.py"));
    runner.runEditor(QStringLiteral("import localmod, sys, __main__\n"
                                    "print(localmod.value, __name__, __file__, sys.argv, hasattr(__main__, '_run_editor'))\n"
                                    "print(repr(sys.stdin.read()))\n"), path);
    QTRY_VERIFY(!runner.running());
    QVERIFY2(runner.status().contains(QStringLiteral("exit code 0")), qPrintable(runner.status() + runner.output()));
    QVERIFY(runner.finishedSuccessfully());
    QVERIFY2(runner.output().contains(QStringLiteral("42 __main__")), qPrintable(runner.output()));
    QVERIFY(runner.output().contains(QStringLiteral("False")));
    QVERIFY(runner.output().contains(QStringLiteral("''")));
    runner.runEditor(QString(), path);
    QTRY_VERIFY(!runner.running());
    QVERIFY2(runner.status().contains(QStringLiteral("exit code 0")), qPrintable(runner.status() + runner.output()));
    runner.runEditor(QStringLiteral("print('x' * 200000)\n"), path);
    QTRY_VERIFY(!runner.running());
    QVERIFY2(runner.status().contains(QStringLiteral("exit code 0")), qPrintable(runner.status() + runner.output()));
    QVERIFY(runner.output().size() >= 200000);
    QString outputName = QUuid::createUuid().toString();
    outputName.remove(QLatin1Char('{')).remove(QLatin1Char('}')).remove(QLatin1Char('-'));
    const QString outputPath = QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation))
            .filePath(outputName + QStringLiteral(".txt"));
    QSignalSpy saved(&runner, SIGNAL(outputSaved(QString)));
    QVERIFY2(runner.saveOutput(outputName), qPrintable(runner.outputSaveError()));
    QCOMPARE(saved.count(), 1);
    QFile outputFile(outputPath);
    QVERIFY(outputFile.open(QIODevice::ReadOnly));
    QCOMPARE(outputFile.readAll(), runner.output().toUtf8());
    outputFile.close();
    QVERIFY(!runner.saveOutput(outputName));
    QCOMPARE(saved.count(), 1);
    QVERIFY(outputFile.remove());
    runner.runEditor(QStringLiteral("print('x' * 1100000)\n"), path);
    QTRY_VERIFY(!runner.running());
    QVERIFY(runner.outputTruncated());
    QCOMPARE(runner.output().size(), 1024 * 1024);
    runner.runEditor(QStringLiteral("#") + QString(200000, QLatin1Char('x'))
                     + QStringLiteral("\nprint('large source')"), path);
    QTRY_VERIFY(!runner.running());
    QVERIFY(!runner.outputTruncated());
    QVERIFY2(runner.output().contains(QStringLiteral("large source")), qPrintable(runner.status() + runner.output()));
    runner.runEditor(QStringLiteral("import matplotlib.pyplot as plt\nplt.plot([1, 2])\nplt.show()\n"), path);
    QTRY_VERIFY_WITH_TIMEOUT(!runner.running(), 20000);
    QVERIFY2(runner.status().contains(QStringLiteral("exit code 0")), qPrintable(runner.status() + runner.output()));
    QCOMPARE(runner.plots().size(), 1);
    runner.runEditor(QStringLiteral("raise SystemExit(2)\n"), path);
    QTRY_VERIFY(!runner.running());
    QVERIFY(!runner.finishedSuccessfully());
    QVERIFY(runner.status().contains(QStringLiteral("exit code 2")));
    runner.runEditor(QStringLiteral("while True: pass\n"), path);
    QTRY_VERIFY(runner.running());
    runner.stop();
    QTRY_VERIFY(!runner.running());
}

void EditorTests::qmlLoads()
{
    const QString previousVersion = QCoreApplication::applicationVersion();
    QCoreApplication::setApplicationVersion(QStringLiteral("test-version"));
    qmlRegisterType<PythonHighlighter>("PythonRunner", 1, 0, "PythonHighlighter");
    qmlRegisterType<TextEditHelper>("PythonRunner", 1, 0, "TextEditHelper");
    EditorDocument document;
    document.setText(QStringLiteral("restored draft"));
    FileOpenReceiver fileOpenReceiver;
    const QString home = QString::fromLocal8Bit(qgetenv("TEST_PYTHON_HOME"));
    Runner runner(QStringLiteral("/src/python"), home, home + QStringLiteral("/bin/python3.11"));
    QQmlEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("editorDocument"), &document);
    engine.rootContext()->setContextProperty(QStringLiteral("runner"), &runner);
    engine.rootContext()->setContextProperty(QStringLiteral("fileOpenReceiver"), &fileOpenReceiver);
    QQmlComponent component(&engine, QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    QScopedPointer<QObject> object(component.create());
    QVERIFY2(object, qPrintable(component.errorString()));
    QTest::qWait(500);
    auto *coverComponent = qobject_cast<QQmlComponent *>(object->property("cover").value<QObject *>());
    QVERIFY(coverComponent);
    QScopedPointer<QObject> cover(coverComponent->create(coverComponent->creationContext()));
    QVERIFY2(cover, qPrintable(coverComponent->errorString()));
    QObject *coverText = cover->findChild<QObject *>(QStringLiteral("coverText"));
    QVERIFY(coverText);
    QCOMPARE(coverText->property("text").toString(), QStringLiteral("restored draft"));
    QObject *page = object->findChild<QObject *>(QStringLiteral("editorPage"));
    QObject *editor = object->findChild<QObject *>(QStringLiteral("pythonEditor"));
    QObject *search = object->findChild<QObject *>(QStringLiteral("searchInput"));
    QObject *replacement = object->findChild<QObject *>(QStringLiteral("replaceInput"));
    QVERIFY(page);
    auto *info = page->findChild<QQuickItem *>(QStringLiteral("appInfoMenuLabel"));
    auto *runItem = page->findChild<QQuickItem *>(QStringLiteral("runMenuItem"));
    QVERIFY(info);
    QVERIFY(runItem);
    QCOMPARE(info->property("text").toString(),
             QStringLiteral("PyIDE test-version\nPython editor with NumPy and Matplotlib"));
    QCOMPARE(info->parentItem(), runItem->parentItem());
    QVERIFY(info->y() < runItem->y());
    QVERIFY(editor);
    QCOMPARE(editor->property("text").toString(), QStringLiteral("restored draft"));
    QVERIFY(search);
    QVERIFY(replacement);
    QVERIFY(page->property("resultsPage").value<QObject *>() == nullptr);
    runner.runEditor(QStringLiteral("print('result')\n"), QString());
    QTRY_VERIFY(page->property("resultsPage").value<QObject *>() != nullptr);
    QTRY_VERIFY(!runner.running());
    QObject *stack = object->property("pageStack").value<QObject *>();
    QVERIFY(stack);
    QVERIFY(QMetaObject::invokeMethod(stack, "navigateForward", Q_ARG(QVariant, QVariant())));
    QTRY_VERIFY(object->property("coverShowsResults").toBool());
    QCOMPARE(coverText->property("text").toString(), runner.output());
    QTRY_VERIFY(!stack->property("busy").toBool());
    QVERIFY(QMetaObject::invokeMethod(stack, "navigateBack",
                                      Q_ARG(QVariant, QVariant()), Q_ARG(QVariant, QVariant())));
    QTRY_VERIFY(!object->property("coverShowsResults").toBool());
    QCOMPARE(coverText->property("text").toString(), document.text());
    QObject *results = page->property("resultsPage").value<QObject *>();
    QVERIFY(results->findChild<QObject *>(QStringLiteral("resultsSaveShortcut")));
    runner.runEditor(QStringLiteral("pass\n"), QString());
    QTRY_VERIFY(!runner.running());
    QTRY_VERIFY(page->property("resultsPage").value<QObject *>() == nullptr);
    QVERIFY(!page->property("forwardNavigation").toBool());
    runner.runEditor(QStringLiteral("print('result')\n"), QString());
    QTRY_VERIFY(page->property("resultsPage").value<QObject *>() != nullptr);
    QVERIFY(page->property("forwardNavigation").toBool());
    QObject *searchShortcut = object->findChild<QObject *>(QStringLiteral("editorSearchShortcut"));
    QObject *closeShortcut = object->findChild<QObject *>(QStringLiteral("searchCloseShortcut"));
    QVERIFY(searchShortcut);
    QVERIFY(closeShortcut);
    QVERIFY(QMetaObject::invokeMethod(searchShortcut, "activated"));
    QVERIFY(page->property("searchVisible").toBool());
    QVERIFY(QMetaObject::invokeMethod(closeShortcut, "activated"));
    QVERIFY(!page->property("searchVisible").toBool());
    QVERIFY(object->findChild<QObject *>(QStringLiteral("searchPreviousShortcut")));
    QVERIFY(object->findChild<QObject *>(QStringLiteral("searchNextShortcut")));
    QVERIFY(object->findChild<QObject *>(QStringLiteral("searchReplaceShortcut")));
    QVERIFY(object->findChild<QObject *>(QStringLiteral("searchPrevious")));
    QVERIFY(object->findChild<QObject *>(QStringLiteral("searchNext")));
    QVERIFY(object->findChild<QObject *>(QStringLiteral("searchClose")));
    document.setText(QStringLiteral("one two one"));
    QCOMPARE(editor->property("text").toString(), document.text());
    search->setProperty("text", QStringLiteral("one"));
    replacement->setProperty("text", QStringLiteral("three"));
    QObject *input = editor->property("_editor").value<QObject *>();
    QVERIFY(input);
    QObject *completionHint = object->findChild<QObject *>(QStringLiteral("pythonCompletionHint"));
    QVERIFY(completionHint);
    document.setText(QStringLiteral("pri"));
    input->setProperty("cursorPosition", 3);
    page->setProperty("completionStart", 0);
    page->setProperty("suggestions", QVariantList() << QStringLiteral("print()"));
    QCOMPARE(completionHint->property("text").toString(), QStringLiteral("nt()"));
    QVERIFY(QMetaObject::invokeMethod(page, "acceptCompletion", Q_ARG(QVariant, QStringLiteral("print()"))));
    QCOMPARE(document.text(), QStringLiteral("print()"));
    QCOMPARE(input->property("cursorPosition").toInt(), 6);
    document.setText(QStringLiteral("one two one"));
    input->setProperty("cursorPosition", 0);
    QVERIFY(QMetaObject::invokeMethod(page, "findText", Q_ARG(QVariant, false)));
    QCOMPARE(input->property("selectionStart").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(page, "findText", Q_ARG(QVariant, false)));
    QCOMPARE(input->property("selectionStart").toInt(), 8);
    QVERIFY(QMetaObject::invokeMethod(page, "findText", Q_ARG(QVariant, false)));
    QCOMPARE(input->property("selectionStart").toInt(), 0);
    QVERIFY(QMetaObject::invokeMethod(page, "findText", Q_ARG(QVariant, true)));
    QCOMPARE(input->property("selectionStart").toInt(), 8);
    QVERIFY(QMetaObject::invokeMethod(page, "replaceAllMatches"));
    QCOMPARE(document.text(), QStringLiteral("three two three"));
    replacement->setProperty("text", QString());
    search->setProperty("text", QStringLiteral("three"));
    QVERIFY(QMetaObject::invokeMethod(page, "replaceAllMatches"));
    QCOMPARE(document.text(), QStringLiteral(" two "));
    QObject *shortcut = object->findChild<QObject *>(QStringLiteral("saveShortcut"));
    QVERIFY(shortcut);
    QTemporaryDir dir;
    const QString path = QDir(dir.path()).filePath(QStringLiteral("save.py"));
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("old");
    file.close();
    QVERIFY(document.openFile(QUrl::fromLocalFile(path), true));
    document.setText(QStringLiteral("saved by shortcut"));
    QVERIFY(QMetaObject::invokeMethod(shortcut, "activated"));
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("saved by shortcut"));
    file.close();
    QObject *notification = object->findChild<QObject *>(QStringLiteral("saveNotification"));
    QVERIFY(notification);
    QVERIFY(notification->property("previewBody").toString().contains(QStringLiteral("save.py")));
    const QString otherPath = QDir(dir.path()).filePath(QStringLiteral("other file.py"));
    QFile otherFile(otherPath);
    QVERIFY(otherFile.open(QIODevice::WriteOnly));
    otherFile.write("opened externally");
    otherFile.close();
    fileOpenReceiver.request(QUrl::fromLocalFile(otherPath).toString());
    QTRY_COMPARE(document.filePath(), otherPath);
    QCOMPARE(document.text(), QStringLiteral("opened externally"));
    document.setText(QStringLiteral("unsaved edit"));
    fileOpenReceiver.request(QUrl::fromLocalFile(path).toString());
    QTRY_VERIFY(object->findChild<QObject *>(QStringLiteral("discardDialog")) != nullptr);
    QCOMPARE(document.filePath(), otherPath);
    QCOMPARE(document.text(), QStringLiteral("unsaved edit"));
    QCoreApplication::setApplicationVersion(previousVersion);
}

QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
