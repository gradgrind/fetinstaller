#ifndef INSTALLER_H
#define INSTALLER_H

#include <ui_installer.h>

#include <QWidget>
#include <QThread>
#include <QDir>
#include <QString>

namespace Ui {
class Installer;
}

class Installer : public QWidget
{
    Q_OBJECT
    QThread workerThread;

public:
    Installer(QWidget *parent = nullptr);
    ~Installer() {
        workerThread.quit();
        workerThread.wait();
        delete ui;
    }

    QString defaultInstallationPath;

private slots:
    void selectInstallDir();
    void setInstallPath(QString ipath);
    void handleNumberOfFiles(int n);
    void handleFileCopied(QString filepath);
    void handleCopyFailed(QString filepath);
    void handleCopyingFinished(QString msg);

private:
    Ui::Installer *ui;

    void page_2();
    void page_3();

    bool installationExists(QDir rootdir);

    QString filelist; // path to file containing installed file list
    QFile file_log;
    QTextStream log_stream;
    QDir src_dir;
    QDir dst_dir;
    QString uninstall;

signals:
    void copy(const QDir& srcDir, const QDir& dstDir);
};

#endif // INSTALLER_H
