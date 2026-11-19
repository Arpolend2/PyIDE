#include "newfile.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

bool writeNewDownload(const QString &name, const QString &extension, const QByteArray &contents,
                      QString *path, QString *error)
{
    QString filename = name.trimmed();
    if (filename.isEmpty() || filename == QLatin1String(".") || filename == QLatin1String("..")
            || filename.contains(QLatin1Char('/')) || filename.contains(QLatin1Char('\\'))) {
        *error = QCoreApplication::translate("NewFile", "Enter a file name without a path.");
        return false;
    }
    if (!filename.endsWith(extension, Qt::CaseInsensitive))
        filename += extension;
    const QString downloads = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    if (downloads.isEmpty() || !QDir().mkpath(downloads)) {
        *error = QCoreApplication::translate("NewFile", "Could not access the Downloads directory.");
        return false;
    }
    *path = QDir(downloads).filePath(filename);
    const int fd = ::open(QFile::encodeName(*path).constData(), O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
        *error = QCoreApplication::translate("NewFile", "Could not create file: %1").arg(QString::fromLocal8Bit(std::strerror(errno)));
        return false;
    }
    QFile file;
    if (!file.open(fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle)) {
        ::close(fd);
        *error = QCoreApplication::translate("NewFile", "Could not write new file %1: %2").arg(*path, file.errorString());
        return false;
    }
    if (file.write(contents) != contents.size() || !file.flush()) {
        *error = QCoreApplication::translate("NewFile", "Could not write new file %1: %2").arg(*path, file.errorString());
        return false;
    }
    file.close();
    error->clear();
    return true;
}
