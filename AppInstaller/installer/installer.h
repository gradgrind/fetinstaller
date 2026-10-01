#ifndef INSTALLER_H
#define INSTALLER_H

#include <QApplication>
#include <QTranslator>
#include <QWidget>
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

class Installer : public QWidget
{
    Q_OBJECT
    QThread workerThread;
    CopyWorker* copyWorker;
    AppInfo appinfo;

public:
    explicit Installer(QWidget *parent = nullptr);
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
    bool registerApp();
    bool linkDirectoryHierarchy(const QString &srcPath, const QString &dstPath);

    bool bugflag{false};
    bool registered;
    QStringList copyErrors;
    bool scanComplete;
    bool scanOk;
    QString defaultInstallationPath;
    bool installationPartial{false};
    InstallFiles installFiles; // files and directories to be installed
    QString xfilepath; // path to file containing installed file list
    QFile file_log;
    QStringList localfiles;
    QDir src_dir;
    QDir dst_dir;
    QString uninstall;

signals:
    void doCopy(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles);
};

#endif // INSTALLER_H
