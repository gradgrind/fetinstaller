#include "installer.h"
#include "ui_installer.h"
#include "copythread.h"
#include <QDirListing>
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

    // *** Connect signals ***

    // This one is for quitting the program on errors
    connect(this, &Installer::exit_cc, qApp, &QApplication::exit, Qt::QueuedConnection);

    // page switching
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Installer::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &Installer::page_3);
    connect(ui->buttonBox_2, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_3, &QDialogButtonBox::accepted, this, &Installer::installationComplete);

    // select destination directory
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

    // Get source path
    src_dir = QFileInfo(QCoreApplication::applicationDirPath()).canonicalFilePath();
    if (QFileInfo::exists(src_dir.filePath("install_source"))) {
        // Accept an "install_source" directory in the same directory as the installer executable
        src_dir.cd("install_source");
    } else {
        // Assume the application is in the "_bin" directory of the source directory
        src_dir.cdUp();
    }
    // A simple check that the source directory is valid (contains a FET install bundle)
    if (!QFileInfo::exists(src_dir.filePath("bin/fet"))
        || !QFileInfo::exists(src_dir.filePath("share/fet"))) {

        QMessageBox::critical(this, tr(FATAL_ERROR), "BUG: installation files not found");
        emit exit_cc(2);
        return;
    }

    scanSource();
}

Installer::~Installer() {
    workerThread.quit();
    workerThread.wait();
    delete ui;
}


void Installer::setInstallPath(QString ipath)
{
    ui->installPath->setText(ipath);
    dst_dir = ipath;
    ui->desktopSetup->setVisible(ipath == defaultInstallationPath);

    //TODO: Check for overwrites?
}

void Installer::scanSource()
{
    // Collect the files here for copying later: their paths relative to the source root.
    // All files except from root directories starting with "_" (currently just "_bin") are copied.
    QStringList rfiles; // relative paths

    /*
    QDirIterator it(src_dir.path(), QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString apath = it.next();
        QString rpath = src_dir.relativeFilePath(apath);
        if (rpath.startsWith("_")) {
            continue; // skip root directories starting with "_"
        }

//TODO: Can I stop recursing into symlinked directories?
        QFileInfo finfo{apath};
        if (finfo.isSymLink()) {

            qDebug() << "LINK" << finfo.readSymLink() << "&" << finfo.symLinkTarget(); // raw & absolute path
        }


        if (it.fileInfo().isFile()) {
            rfiles.append(rpath);
        }
    }
    */

    qDebug() << "Searching" << src_dir.path();
    using F = QDirListing::IteratorFlag;
    // Recursive search, but don't recurse into symlinked directories.
    int badfiles = 0;
    for (const auto &dirEntry : QDirListing(
            src_dir.path(),
            F::Recursive | F::IncludeHidden | F::ExcludeOther | F::ResolveSymlinks | F::IncludeBrokenSymlinks)) {
        QString rpath = src_dir.relativeFilePath(dirEntry.filePath());

        //qDebug() << "R" << rpath;

        if (rpath.startsWith("_")) {
            continue;
        }


        const QFileInfo finfo = dirEntry.fileInfo();
        if (finfo.isSymLink()) {

            QString linkPath = finfo.readSymLink();
            //qDebug() << "LINK" << rpath << "->" << linkPath; // << "&" << finfo.symLinkTarget(); // raw & absolute path

            bool linkTargetExists = false;
            if (finfo.exists()) {
                linkTargetExists = true;
                if (finfo.isDir()) {
                    qDebug() << "DIRECTORY!";
                    rpath += "/";
                }
            } else {
                qDebug() << "MISSING LINK:" << finfo.isDir() << QFileInfo(linkPath).isRelative() << rpath;
                //ui->symlink_messages->appendPlainText(
                //    tr("WARNING, symlink missing target: %1 -> %2")
                //        .arg(rpath, linkPath));
            }

            if (QFileInfo(linkPath).isRelative()) {
                // A relative link within the install package is acceptable, as long as its target exists.
                // A relative link outside the package is an error.
                QString lrpath = src_dir.relativeFilePath(finfo.symLinkTarget());
                if (lrpath.startsWith("..")) {
                    // outside the package
                    qDebug() << "ERROR, relative symlink outside package:" << rpath << "->" << linkPath;
                    ui->symlink_messages->appendPlainText(
                        tr("ERROR, relative symlink outside package: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->symlink_messages->appendPlainText("");
                    badfiles++;
                } else {
                    // within the package
                    if (!linkTargetExists) {
                        ui->symlink_messages->appendPlainText(
                            tr("ERROR, target missing for relative symlink: %1 -> %2")
                                .arg(rpath, linkPath));
                        ui->symlink_messages->appendPlainText("");
                        badfiles++;
                    }
                }
            } else {
                // An absolute link within the install package is an error.
                // An absolute link outside the package will be accepted, but a warning will be issued.
                QString lrpath = src_dir.relativeFilePath(linkPath);
                if (lrpath.startsWith("..")) {
                    // outside the package

                    //TODO: mention linkTargetExists?

                    qDebug() << "WARNING, absolute symlink:" << rpath << "->" << linkPath;
                    ui->symlink_messages->appendPlainText(
                        tr("WARNING, absolute symlink: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->symlink_messages->appendPlainText("");
                } else {
                    // inside the package
                    qDebug() << "ERROR, absolute symlink within package:" << rpath << "->" << linkPath;
                    ui->symlink_messages->appendPlainText(
                        tr("ERROR, absolute symlink within package: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->symlink_messages->appendPlainText("");
                    badfiles++;
                }
            }






        }

    //TODO...

    }

    if (badfiles != 0) {
        qDebug() << badfiles << "invalid files. DON'T CONTINUE";
        //TODO: Report "N invalid files" ... disallow continuation
    }

}

void Installer::page_2()
{

    //TODO: Check that all relative symlinks have targets within the installation directory
    // and report any missing targets. Actually this should be at the very beginning of page 1!

    //ui->symlink_messages

    // If a previous installation is to be uninstalled, do it now (if possible)
    if (!uninstall.isEmpty() && ui->existingCheckBox->isChecked()) {
        QProcess::execute(uninstall);
    }

    ui->stackedWidget->setCurrentIndex(1);

    // Default installation path
    defaultInstallationPath = QDir::home().absoluteFilePath(".local");
    setInstallPath(defaultInstallationPath);
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

bool Installer::installationExists()
{
    // Simple test (not reliable!) of whether an existing installation will be overwritten
    return QFileInfo::exists(dst_dir.filePath("bin/fet"))
        || QFileInfo::exists(dst_dir.filePath("share/fet"));
}

void Installer::page_3()
{
    if (installationExists()) {

        if (QFileInfo::exists(dst_dir.filePath("bin/fet_uninstall"))
            && QMessageBox::warning(this, tr(WARNING),
                                    tr(WARN_EXISTING_UNINSTALL).arg(dst_dir.path()),
                                    QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes) {

            // Try to uninstall it
            QProcess::execute(dst_dir.filePath("bin/fet_uninstall"));
        }

        // Test success of uninstall
        if (installationExists()) {
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