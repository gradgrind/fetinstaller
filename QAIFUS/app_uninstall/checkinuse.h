#ifndef CHECKINUSE_H
#define CHECKINUSE_H

#include <QCoreApplication>
#include <QDir>

uint dirInUse(QDir dir, qint64 pid0);

#endif // CHECKINUSE_H
