QT += core gui qml quick testlib dbus
CONFIG += console testcase c++11 link_pkgconfig
CONFIG -= app_bundle
PKGCONFIG += runtime-manager-qt5
TARGET = editor_tests
INCLUDEPATH += ../src
SOURCES += editor_tests.cpp ../src/editordocument.cpp ../src/pythonhighlighter.cpp \
           ../src/textedithelper.cpp ../src/runner.cpp ../src/fileopenreceiver.cpp ../src/newfile.cpp
HEADERS += ../src/editordocument.h ../src/pythonhighlighter.h \
           ../src/textedithelper.h ../src/runner.h ../src/fileopenreceiver.h ../src/newfile.h
RESOURCES += ../resources.qrc
translation.target = PythonRunner_ru.qm
translation.depends = $$PWD/../translations/PythonRunner_ru.ts
translation.commands = lrelease $$shell_quote($$PWD/../translations/PythonRunner_ru.ts) -qm $$shell_quote($$OUT_PWD/PythonRunner_ru.qm)
QMAKE_EXTRA_TARGETS += translation
PRE_TARGETDEPS += PythonRunner_ru.qm
