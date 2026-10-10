#ifndef INSTALLER_H
#define INSTALLER_H

#include <QApplication>
#include <QTranslator>
#include <QWidget>
#include <QPlainTextEdit>
#include <QThread>
#include <QDir>
#include <QString>
#include <QFileInfo>
#include "copythread.h"
#include "appinfo.h"

namespace Ui {
class Installer;
}

struct linktest {
    QString message;    // empty, warning or error message
    QString link;       // empty if error
    QString target;     // empty if error
};

#if defined Q_OS_WIN

struct registerItem {
    QString path;
    QString value;
    qint64 nvalue{0};
    bool numeric{false};

};

#else

enum registerType {
    R_COPY,
    R_LINK,
    R_WRITE
};

struct registerItem {
    QString source;
    QString destination;
    QString message;
    registerType type;
};

#endif

class Installer : public QWidget
{
    Q_OBJECT
    QThread workerThread;
    CopyWorker* copyWorker;
    AppInfo appinfo;

public:
    explicit Installer(QLocale locale, QWidget *parent = nullptr);
    ~Installer();

private slots:
    void page_0();
    void page_1();
    void page_2();
    void page_3();
    void page_4();

    void refreshView();
    void selectDefaultDir();
    void selectInstallDir();
    void setInstallPath(QString ipath = QString{});
    void uninstallExisting();
    void handleNumberOfFiles(int n);

    void handleDirWritten(QString filepath);
    void handleDirExists(QString filepath);
    void handleDirWriteFailed(QString filepath);

    void handleFileCopied(QString filepath);
    void handleCopyFailed(QString filepath);
    void handleLinkCopied(QPair<QString, QString> filepaths);
    void handleLinkFailed(QPair<QString, QString> filepaths);

    void handleCopyingFinished();

private:
    Ui::Installer *ui;
    void closeEvent(QCloseEvent *event) override;

    linktest testLink(QString rpath);

    void print_line(QString line, bool bold = false);
    void progressOne();
    bool isInstalled(QString version = {});
    bool preRegister();
    bool registerApp();

    QPlainTextEdit* textOutput{nullptr};
    QStringList copyErrors;
    QString defaultInstallationPath;
    InstallFiles installFiles; // files and directories to be installed
    QList<registerItem> registrationList;
    QString desktopName;
    QString desktopPath;
    QStringList localfiles;
    QDir src_dir;
    QDir dst_dir;
    QDir app_dir; // within dst_dir, the actual installation directory
    QString versioned_app; // app name + "-" + app version
    QString uninstall_exe;

    bool bugflag{false};
    bool scanComplete;
    bool scanOk;
    bool mimefiles; // flag for cache updating
    bool installationPartial{false};

public:
    bool app_initialized{false};

signals:
    void doCopy(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles);
};

#endif // INSTALLER_H
