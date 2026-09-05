#ifndef INSTALLER_H
#define INSTALLER_H

#include <QApplication>
#include <QTranslator>
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
    explicit Installer(QWidget *parent = nullptr);
    ~Installer();

private slots:
    void scanSource();
    void selectInstallDir();
    void setInstallPath(QString ipath);
    void handleNumberOfFiles(int n);
    void handleFileCopied(QString filepath);
    void handleCopyFailed(QString filepath);
    void handleCopyingFinished(QString msg);
    void installationComplete();

private:
    Ui::Installer *ui;
    void closeEvent(QCloseEvent *event) override;

    void page_1();
    void page_2();
    void page_3();
    void tidyPartial();
    void error_exit(int cc);

    bool installationExists();

    QString defaultInstallationPath;
    bool installationPartial{false};
    QString filelist; // path to file containing installed file list
    QFile file_log;
    QTextStream log_stream;
    QDir src_dir;
    QDir dst_dir;
    QString uninstall;

signals:
    void copy(const QDir& srcDir, const QDir& dstDir);
    void exit_cc(int cc);
};

#endif // INSTALLER_H
