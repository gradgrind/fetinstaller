#ifndef APPINFO_H
#define APPINFO_H

#include <QObject>
#include <QDir>
#include <QLocale>
#include <QSettings>
#include <QStringList>
#include <QMap>

struct ExecValue {
    QString extension;
    QString execName;
    QString extra;
};

class AppInfo : QObject
{
    Q_OBJECT

    bool read0(QString& var, QString key, bool critical=false);
    bool reados(QString& var, QString key);
    bool read(QString& var, QString key, bool critical=false);
    QSettings* appsettings;
    QString appconf; // configuration file (full path)
    QString ostype;

public:
    AppInfo();
    ~AppInfo() {
        delete appsettings;
    }
    bool init(QLocale locale);

    QDir SOURCE_DIR;
    QString PROJECTNAME;
    QString PROJECTLONGNAME;
    QString PROJECTVERSION;
    QMap<QString, ExecValue> EXECUTABLES;
    QString LAUNCH;

    // These are currently only relevant on Windows:
    QString Publisher;
    QString WebLink;
};

#endif // APPINFO_H
