#include "installer.h"
#include "ui_installer.h"
#include "copythread.h"
#include <QDirIterator>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QFileDialog>

static const char *FATAL_ERROR = QT_TRANSLATE_NOOP("Installer", "Fatal Error");
static const char *WARNING = QT_TRANSLATE_NOOP("Installer", "Warning");

Installer::Installer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Installer)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // Connect signals
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Installer::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &Installer::page_3);
    connect(ui->buttonBox_2, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_3, &QDialogButtonBox::accepted, this, &QApplication::quit);

    connect(ui->installPathBrowse, &QToolButton::clicked, this, &Installer::selectInstallDir);

    //TODO
    QString which_fet;
    QProcess process;
    process.start("which", QStringList() << "fet");
    process.waitForFinished(1000);
    if (process.state() == QProcess::NotRunning) {
        if (process.exitStatus() == QProcess::NormalExit) {
            if (process.exitCode() == 0) {
                which_fet = process.readAllStandardOutput();
                which_fet = which_fet.trimmed();
            }
        } else {
            QMessageBox::warning(this, tr(WARNING), tr("Search for existing installation failed"));
        }
    } else {
        process.kill();
        QMessageBox::warning(this, tr(WARNING), tr("Search for existing installation not possible"));
    }
    if (!which_fet.isEmpty()) {
        //qDebug() << "which fet?" << which_fet;
        ui->existing_path->setText(which_fet);
        ui->existing_fet->show();

        QString fet_dir{QFileInfo{which_fet}.absolutePath()};
        QString uninstall{fet_dir + "/fet_uninstall"};
        if (QFileInfo::exists(uninstall)) {
            ui->existingCheckBox->setChecked(true);
            ui->existingCheckBox->show();
        } else {
            ui->existingCheckBox->hide();
        }
        qDebug() << "uninstaller:" << uninstall << QFileInfo::exists(uninstall);
    } else {
        ui->existing_fet->hide();
    }
}

void Installer::setInstallPath(QString ipath)
{
    ui->installPath->setText(ipath);
    dst_dir = ipath;
    ui->desktopSetup->setVisible(ipath == defaultInstallationPath);
}

void Installer::page_2()
{
    ui->stackedWidget->setCurrentIndex(1);
    dst_dir = QDir::home();
    dst_dir.cd(".local");
    defaultInstallationPath = dst_dir.path();
    ui->installPath->setText(defaultInstallationPath);

    //TODO? Get source path
    QDir d0{QCoreApplication::applicationDirPath()};
    QString src{d0.absoluteFilePath("install")};
    src_dir.setPath(src);
    if (!src_dir.exists()) {
        qDebug() << "Source directory does not exist.";
        QMessageBox::critical(this, tr(FATAL_ERROR), "BUG: installation files not found");
        qApp->exit(2);
        return;
    }


}

void Installer::selectInstallDir()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Directory"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        QFileInfo finfo{dir};
        if (finfo.isWritable()) {
            setInstallPath(dir);
        } else {
            QMessageBox::warning(this, tr(WARNING), tr("User can not write to selected directory: ") + dir);
        }
    }
}

void Installer::page_3()
{
    ui->installProgress->setMinimum(0);
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->stackedWidget->setCurrentIndex(2);

    // Get path to install to
    dst_dir.setPath(src_dir.absoluteFilePath("../target_dir")); //TODO!!!

    QString ipath{dst_dir.absoluteFilePath("share/fet")};
    if (!dst_dir.mkpath(ipath)) {
        QMessageBox::critical(this, tr(FATAL_ERROR), tr("Creating target directory failed: ") + ipath);
        qApp->exit(1);
        return;
    }

    // Open file to record installed files
    QString filelist{ipath + "/installed_files"};
    file_log.setFileName(filelist);
    if (!file_log.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, tr(FATAL_ERROR), tr("Could not create the installed-files list"));
        qApp->exit(1);
        return;
    }
    log_stream.setDevice(&file_log);

    // Use background thread to perform copying

    CopyWorker* copyWorker = new CopyWorker;
    copyWorker->moveToThread(&workerThread);

    // Connect signals
    connect(&workerThread, &QThread::finished, copyWorker, &QObject::deleteLater);
    connect(this, &Installer::copy, copyWorker, &CopyWorker::copyDirectory);
    connect(copyWorker, &CopyWorker::number_of_files, this, &Installer::handleNumberOfFiles);
    connect(copyWorker, &CopyWorker::copied, this, &Installer::handleFileCopied);
    connect(copyWorker, &CopyWorker::failed_copy, this, &Installer::handleCopyFailed);
    connect(copyWorker, &CopyWorker::copying_done, this, &Installer::handleCopyingFinished);

    workerThread.start();

    // Start copying
    emit copy(src_dir, dst_dir);
}

void Installer::handleNumberOfFiles(int n)
{
    ui->installProgress->setMaximum(n);
    ui->installProgress->setValue(0);
}

void Installer::handleFileCopied(QString filepath)
{
    log_stream << filepath << "\n";
    //log_stream.flush();
    int n = ui->installProgress->value();
    if (n == ui->installProgress->maximum()) {
        QMessageBox::critical(this, "BUG", "Installed files miscounted");
    } else {
        ui->installProgress->setValue(n + 1);
    }
    ui->installDetails->appendPlainText("+ " + dst_dir.relativeFilePath(filepath));
}

void Installer::handleCopyFailed(QString filepath)
{
    QMessageBox::critical(this, tr(FATAL_ERROR), tr("Could not copy to: ") + filepath);
}

void Installer::handleCopyingFinished()
{
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(true);
}
