#include "installer.h"
#include "copythread.h"
#include <QDirIterator>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QFileDialog>

static const char *FATAL_ERROR = QT_TRANSLATE_NOOP("Installer", "Fatal Error");
static const char *WARNING = QT_TRANSLATE_NOOP("Installer", "Warning");

static const char *WARN_EXISTING_UNINSTALL = QT_TRANSLATE_NOOP("Installer", R"(
There is already a FET installation at %1.

Do you want to uninstall it?)");

static const char *WARN_EXISTING_OVERWRITE = QT_TRANSLATE_NOOP("Installer", R"(
It looks like you are going to overwrite an existing FET installation.

This is probably a bad idea. Do you really want to continue?)");

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
    connect(ui->buttonBox_3, &QDialogButtonBox::accepted, this, &Installer::installationComplete);

    connect(ui->installPathBrowse, &QToolButton::clicked, this, &Installer::selectInstallDir);

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
        ui->existing_path->setText(which_fet);
        ui->existing_fet->show();

        QString fet_dir{QFileInfo{which_fet}.absolutePath()};
        uninstall = fet_dir + "/fet_uninstall";
        if (QFileInfo::exists(uninstall)) {
            ui->existingCheckBox->setChecked(true);
            ui->existingCheckBox->show();
        } else {
            uninstall.clear();
            ui->existingCheckBox->hide();
        }
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
    // If a previous installation is to be uninstalled, do it now (if possible)
    if (!uninstall.isEmpty() && ui->existingCheckBox->isChecked()) {
        QProcess::execute(uninstall);
    }

    ui->stackedWidget->setCurrentIndex(1);

    // Default installation path
    defaultInstallationPath = QDir::home().absoluteFilePath(".local");
    setInstallPath(defaultInstallationPath);

    // Get source path
    src_dir = QCoreApplication::applicationDirPath();
    src_dir.cdUp();

    // A simple check that the source directory is valid (contains a FET install bundle)
    if (!QFileInfo::exists(src_dir.filePath("bin/fet"))
        || !QFileInfo::exists(src_dir.filePath("share/fet"))) {

        QMessageBox::critical(this, tr(FATAL_ERROR), "BUG: installation files not found");
        qApp->exit(2);
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

bool Installer::installationExists(QDir rootdir)
{
    // Simple test (not reliable!) of whether an existing installation will be overwritten
    return QFileInfo::exists(dst_dir.filePath("bin/fet"))
        || QFileInfo::exists(dst_dir.filePath("share/fet"));
}

void Installer::page_3()
{
    if (installationExists(dst_dir)) {

        if (QFileInfo::exists(dst_dir.filePath("bin/fet_uninstall"))
            && QMessageBox::warning(this, tr(WARNING),
                                    tr(WARN_EXISTING_UNINSTALL).arg(dst_dir.path()),
                                    QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {

            // Try to uninstall it
            QProcess::execute(dst_dir.filePath("bin/fet_uninstall"));
        }

        // Test success of uninstall
        if (installationExists(dst_dir)) {
            if (QMessageBox::warning(this, tr(WARNING),
                tr(WARN_EXISTING_OVERWRITE),
                QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) {

                return;
            }
        }
    }

    ui->installProgress->setMinimum(0);
    ui->installProgress->setValue(0);
    // Disable the OK button until the copying has finished
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->stackedWidget->setCurrentIndex(2);

    QString ipath{dst_dir.absoluteFilePath("share/fet")};
    if (!dst_dir.mkpath(ipath)) {
        QMessageBox::critical(this, tr(FATAL_ERROR), tr("Creating target directory failed: ") + ipath);
        qApp->exit(1);
        return;
    }

    // Open file to record installed files
    filelist = ipath + "/installed_files";
    file_log.setFileName(filelist);
    if (!file_log.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, tr(FATAL_ERROR), tr("Could not create the installed-files list"));
        qApp->exit(1);
        ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(true);
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
}

void Installer::handleFileCopied(QString filepath)
{
    log_stream << filepath << "\n";
    int n = ui->installProgress->value();
    if (n == ui->installProgress->maximum()) {
        QMessageBox::critical(this, "BUG", "Installed files miscounted");
    } else {
        ui->installProgress->setValue(n + 1);
    }

    //TODO: Use relative paths? Also in the file itself?
    ui->installDetails->appendPlainText("+ " + dst_dir.relativeFilePath(filepath));
}

void Installer::handleCopyFailed(QString filepath)
{
    QMessageBox::critical(this, tr(FATAL_ERROR), tr("Could not copy to: ") + filepath);
}

void Installer::handleCopyingFinished(QString msg)
{
    log_stream << filelist << "\n";
    file_log.close();
    ui->installDetails->appendPlainText("");
    ui->installDetails->appendPlainText(msg);
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void Installer::installationComplete()
{
    if (ui->launch->isChecked()) {
        QProcess runfet;
        runfet.setProgram(dst_dir.filePath("bin/fet"));
        runfet.startDetached();
    }

    qApp->quit();
}