#pragma once

#include <QByteArray>
#include <QString>

bool writeNewDownload(const QString &name, const QString &extension, const QByteArray &contents,
                      QString *path, QString *error);
