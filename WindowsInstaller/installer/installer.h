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
    void allowNonEmpty(bool checked);
    void handleNumberOfFiles(int n);

    void handleDirWritten(QString filepath);
    void handleDirNotCopied(QString filepath);
    void handleDirWriteFailed(QString filepath);
    void handleDirOverwriteFailed(QString filepath);

    void handleFileCopied(QString filepath);
    void handleCopyFailed(QString filepath);
    void handleLinkCopied(QPair<QString, QString> filepaths);
    void handleLinkFailed(QPair<QString, QString> filepaths);

    void handleCopyingFinished();

    void removedFile(QString f, bool ok);
    void removedDir(QString f, bool ok);
    void removingDone();

private:
    Ui::Installer *ui;
    void closeEvent(QCloseEvent *event) override;

    linktest testSymLink(QString rpath);

    void print_3(QString line, bool bold = false);
    void progressOne();

    QDir fet_dir;
    bool bugflag{false};
    QStringList copyErrors;
    bool scanComplete;
    bool scanOk;
    QString defaultInstallationPath;
    bool installationPartial{false};
    InstallFiles installFiles; // files and directories to be installed
    QStringList dstDirectories; // collect the directories in the installation
    QStringList dstFiles; // collect the files in the installation
    QString filelist; // path to file containing installed file list
    QFile file_log;
    QTextStream log_stream;
    QDir src_dir;
    QDir dst_dir;
    QString uninstall;
    QStringList xdirs;
    QStringList xfiles;

signals:
    void doCopy(const QDir& srcDir, const QDir& dstDir, const InstallFiles& iFiles);
    void doRemove(const QDir& dstDir, const QStringList& dirs, const QStringList& files);
};

#endif // INSTALLER_H
