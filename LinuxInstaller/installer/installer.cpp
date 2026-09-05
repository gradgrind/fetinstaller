#include "installer.h"
#include "ui_installer.h"
#include "copythread.h"
#include <QDirListing>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QFileDialog>
#include <QTimer>

static const char *FATAL_ERROR = QT_TRANSLATE_NOOP("Installer", "Fatal Error");
static const char *WARNING = QT_TRANSLATE_NOOP("Installer", "Warning");

static const char *WARN_EXISTING_UNINSTALL = QT_TRANSLATE_NOOP("Installer", R"(
There is already a FET installation at %1.

Do you want to uninstall it?)");

static const char *WARN_EXISTING = QT_TRANSLATE_NOOP("Installer", R"(
There is already a FET installation at %1.

You must remove this before you can install the new version here.)");

//TODO: This is probably only for the "X" button. After an error, the partial installation
// should be done anyway.
static const char *WARN_UNFINISHED = QT_TRANSLATE_NOOP("Installer", R"(
The installation is not complete. If you quit now, any already installed files will be removed.

Do you really want to cancel the installation?)");


//TODO
void Installer::tidyPartial() {
    qDebug() << "TODO: tidy()";
}

void Installer::closeEvent(QCloseEvent *event)
{
    if ( installationPartial ) { //TODO: set to true during file copying, etc.
        if ( QMessageBox::warning(this, tr(WARNING), tr(WARN_UNFINISHED),
                QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes ) {
            tidyPartial();
            event->accept();
        } else {
            event->ignore(); // Don't close the window
        }
    } else {
        event->accept();
    }
}

void Installer::error_exit(int cc)
{
    if ( installationPartial )
        tidyPartial();
    qApp->exit(cc);
}

Installer::Installer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Installer)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // *** Connect signals ***

    // This one is for quitting the program on errors
    connect(this, &Installer::exit_cc, this, &Installer::error_exit, Qt::QueuedConnection);

    // page switching
    connect(ui->buttonBox_0, &QDialogButtonBox::accepted, this, &Installer::page_1);
    connect(ui->buttonBox_0, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Installer::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &Installer::page_3);
    connect(ui->buttonBox_2, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_3, &QDialogButtonBox::accepted, this, &Installer::installationComplete);

    // select destination directory
    connect(ui->installPathBrowse, &QToolButton::clicked, this, &Installer::selectInstallDir);

    // Check installation data, collect files to be installed
    ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(false);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    //TODO: If the time is too short, a black window might get shown at first ...
    QTimer::singleShot(100, this, &Installer::scanSource);
}

Installer::~Installer() {
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void Installer::scanSource()
{
    // This runs quickly enough not to be run in a background thread. It sets a busy cursor,
    // but the processing should be so quick that this will not be visible.

    // Collect the files here for copying later: their paths are relative to the source root.
    // All files, except from root directories starting with "_" (currently just "_bin"), are copied.
    QStringList installationFiles;
    QStringList installationDirs;
    QList<QPair<QString, QString>> installationLinksRel; // relative symlinks (within the installation)
    QList<QPair<QString, QString>> installationLinksAbs; // absolute symlinks (outside the installation)

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

    //qDebug() << "Searching" << src_dir.path();
    using F = QDirListing::IteratorFlag;
    // Recursive search, but don't recurse into symlinked directories.
    int badfiles{0};
    int warnings{0};
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
            // I need to test whether QFile::link can create links with non-existent targets and
            // whether directory links work the same as file links.

            QString linkPath = finfo.readSymLink(); // target path, relative or absolute
            //qDebug() << "LINK" << rpath << "->" << linkPath; // << "&" << finfo.symLinkTarget(); // raw & absolute path

            bool linkTargetExists = finfo.exists();

            if (QFileInfo(linkPath).isRelative()) {
                // A relative link within the install package is acceptable, as long as its target exists.
                // A relative link outside the package is an error.
                QString lrpath = src_dir.relativeFilePath(finfo.symLinkTarget());
                if (lrpath.startsWith("..")) {
                    // outside the package
                    ui->symlink_messages->appendPlainText(
                        tr("ERROR, relative symlink outside package: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->symlink_messages->appendPlainText("");
                    badfiles++;
                } else {
                    // within the package
                    if (linkTargetExists) {
                        installationLinksRel.append({rpath, linkPath});
                    } else {
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
                if (src_dir.relativeFilePath(linkPath).startsWith("..")) {
                    // outside the package
                    QString x;
                    if ( linkTargetExists ) {
                        x = tr(" (doesn't exist!)");
                    }
                    ui->symlink_messages->appendPlainText(
                        tr("WARNING, absolute symlink: %1 -> %2%3")
                            .arg(rpath, linkPath, x));
                    warnings++;
                    ui->symlink_messages->appendPlainText("");
                    installationLinksAbs.append({rpath, linkPath});
                } else {
                    // inside the package
                    ui->symlink_messages->appendPlainText(
                        tr("ERROR, absolute symlink within package: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->symlink_messages->appendPlainText("");
                    badfiles++;
                }
            }
        } else {
            // Normal file or directory
            if ( finfo.isDir() ) {
                installationDirs.append(rpath);
            } else {
                // Normal file
                if ( finfo.isReadable() ) {
                    installationFiles.append(rpath);
                } else {
                    ui->symlink_messages->appendPlainText(
                        tr("ERROR, file not readable: %1").arg(rpath));
                    badfiles++;
                }
            }
        }
    }
    // Save results
    installFiles.installationFiles = installationFiles;
    installFiles.installationDirs = installationDirs;
    installFiles.installationLinksRel = installationLinksRel;
    installFiles.installationLinksAbs = installationLinksAbs;

    QApplication::restoreOverrideCursor();
    if (badfiles == 0) {
        // Allow continuation
        ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        ui->symlink_messages->appendPlainText(
            tr("%1 invalid files – installation is not possible.").arg(badfiles));
    }

    //TODO!!!
    //if ( badfiles == 0 && warnings == 0 )
    if ( true )
        // If there is nothing to report, jump straight to the next page
        page_1();
}

void Installer::page_1()
{
    ui->stackedWidget->setCurrentIndex(1);

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

void Installer::page_2()
{
    // If a previous installation is to be uninstalled, do it now (if possible)
    if (!uninstall.isEmpty() && ui->existingCheckBox->isChecked()) {
        QProcess::execute(uninstall);
    }

    ui->stackedWidget->setCurrentIndex(2);

    // Default installation path
    defaultInstallationPath = QDir::home().absoluteFilePath(".local");

    //TODO--
    defaultInstallationPath = "/home/mt/Development/fet/installer/tmp";

    setInstallPath(defaultInstallationPath);
}

void Installer::selectInstallDir()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Directory"),
        QDir::homePath(),
        QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        setInstallPath(dir);
    }
}

void Installer::setInstallPath(QString ipath)
{
    ui->installPath->setText(ipath);
    dst_dir = ipath;
    ui->desktopSetup->setVisible(ipath == defaultInstallationPath);

    // Check destination
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(false);
    bool ok{true};
    ui->would_overwrite->clear();
    // Destination writable? (not reliable on Windows?)
    if ( !QFileInfo{ipath}.isWritable() ) {
        ui->would_overwrite->appendPlainText(tr("Destination not writable: %1").arg(ipath));
        ok = false;
    } else {
        // Check for FET installation here

        //TODO--
        if ( dst_dir.exists("bin/fetXXX-TODO") ) {

        //if ( dst_dir.exists("bin/fet") ) {
            ui->would_overwrite->appendPlainText(tr(WARN_EXISTING).arg(dst_dir.path()));
            // Check for FET uninstaller
            if ( dst_dir.exists("bin/fet_uninstall") ) {
                if ( QMessageBox::warning(
                        this,
                        tr(WARNING),
                        tr(WARN_EXISTING_UNINSTALL).arg(dst_dir.path()),
                        QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes ) {

                    // Try to uninstall it
                    QProcess::execute(dst_dir.filePath("bin/fet_uninstall"));
                } else {
                    return;
                }
            } else {
                return;
            }
        }

        // Check for overwrites ...

        ui->would_overwrite->appendPlainText("");
        ui->would_overwrite->appendPlainText(tr("Files exist already:"));
        ui->would_overwrite->appendPlainText("");
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        // Collect all source files, skip the directories
        QStringList flist = installFiles.installationFiles;
        for (const auto& fpair : std::as_const(installFiles.installationLinksAbs)) {
            flist.append(fpair.first);
        }
        for (const auto& fpair : std::as_const(installFiles.installationLinksRel)) {
            flist.append(fpair.first);
        }
        // Test for the existence of each file in the destination directory
        for (const auto& f : flist) {

            //TODO--
            if ( !f.startsWith("bin") ) continue;
            QFileInfo fin( dst_dir.absoluteFilePath(f) );
            qDebug() << "?" << f << dst_dir.exists(f) << fin.exists() << fin.symLinkTarget();
            continue;

            //qDebug() << "?" << f << dst_dir.exists(f) << dst_dir.absoluteFilePath(f);
            if ( dst_dir.exists(f) ) {
                ok = false;
                ui->would_overwrite->appendPlainText(dst_dir.absoluteFilePath(f));
            }
        }
    }
    if ( ok )
        ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void Installer::page_3()
{
    ui->installProgress->setMinimum(0);
    ui->installProgress->setValue(0);
    // Disable the OK button until the copying has finished
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->stackedWidget->setCurrentIndex(3);

    //TODO: Is it correct to start copying immediately?

    dstDirectories.clear(); // collect the directories in the installation
    dstFiles.clear(); // collect the files in the installation

    // Use background thread to perform copying

    CopyWorker* copyWorker = new CopyWorker;
    copyWorker->moveToThread(&workerThread);

    // Connect signals
    connect(&workerThread, &QThread::finished, copyWorker, &QObject::deleteLater);
    connect(this, &Installer::doCopy, copyWorker, &CopyWorker::copyDirectory);

    void dir_nocopy(QString filepath);
    void dir_written(QString filepath);
    void dir_failed_write(QString filepath);
    void dir_failed_overwrite(QString filepath);

    connect(copyWorker, &CopyWorker::number_of_files, this, &Installer::handleNumberOfFiles);

    connect(copyWorker, &CopyWorker::dir_nocopy, this, &Installer::handleDirNotCopied);
    connect(copyWorker, &CopyWorker::dir_written, this, &Installer::handleDirWritten);
    connect(copyWorker, &CopyWorker::dir_failed_write, this, &Installer::handleDirWriteFailed);
    connect(copyWorker, &CopyWorker::dir_failed_overwrite, this, &Installer::handleDirOverwriteFailed);

    connect(copyWorker, &CopyWorker::file_copied, this, &Installer::handleFileCopied);
    connect(copyWorker, &CopyWorker::failed_copy, this, &Installer::handleCopyFailed);
    connect(copyWorker, &CopyWorker::link_copied, this, &Installer::handleLinkCopied);
    connect(copyWorker, &CopyWorker::failed_link, this, &Installer::handleLinkFailed);

    connect(copyWorker, &CopyWorker::copying_done, this, &Installer::handleCopyingFinished);

    workerThread.start();

    // Start copying
    emit doCopy(src_dir, dst_dir, installFiles);
}

// In the slots below, the filepath arguments are all relative to the destination base

void Installer::handleNumberOfFiles(int n)
{
    ui->installProgress->setMaximum(n);
}

void Installer::handleDirWritten(QString filepath)
{
    dstDirectories.append(filepath);
    incrementProgress();
    ui->installDetails->appendPlainText("+ " + filepath + "/");
}

void Installer::handleDirNotCopied(QString filepath)
{
    dstDirectories.append(filepath);
    incrementProgress();
    ui->installDetails->appendPlainText("(+) " + filepath + "/");
}

void Installer::handleDirWriteFailed(QString filepath)
{
    // TODO: show absolute path?
    QMessageBox::critical(
        this,
        tr(FATAL_ERROR),
        tr("Could not create directory: %1").arg(filepath));
    emit exit_cc(2);
}

void Installer::handleDirOverwriteFailed(QString filepath)
{
    // TODO: show absolute path?
    QMessageBox::critical(
        this,
        tr(FATAL_ERROR),
        tr("Existing item is not writable directory: %1").arg(filepath));
    emit exit_cc(2);
}

void Installer::incrementProgress()
{
    int n = ui->installProgress->value();
    if (n == ui->installProgress->maximum()) {
        QMessageBox::critical(this, "BUG", "Installed files miscounted");
        emit exit_cc(2);
    } else {
        ui->installProgress->setValue(n + 1);
    }
}

void Installer::handleFileCopied(QString filepath)
{
    //TODO: log_stream << filepath << "\n";
    dstFiles.append(filepath);
    incrementProgress();

    //TODO: Use relative paths? Also in the file itself?
    ui->installDetails->appendPlainText("+ " + filepath);
}

void Installer::handleCopyFailed(QString filepath)
{
    QMessageBox::critical(this, tr(FATAL_ERROR), tr("Could not copy file to: %1").arg(filepath));
    emit exit_cc(2);
}

void Installer::handleLinkCopied(QPair<QString, QString> filepaths)
{
    //TODO: log_stream << filepaths.first << "\n";
    dstFiles.append(filepaths.first);
    incrementProgress();

    //TODO: Use relative paths? Also in the file itself?
    ui->installDetails->appendPlainText("+ " + filepaths.second);
}

void Installer::handleLinkFailed(QPair<QString, QString> filepaths)
{
    QMessageBox::critical(
        this, tr(FATAL_ERROR),
        tr("Could not link %1 to: %2").arg(filepaths.first, filepaths.second));
}

//TODO: Could this take too long?
void Installer::handleCopyingFinished(QString msg)
{
    // Open file to record installed files
    QString filelistpath{"share/fet/installed_files"};
    filelist = dst_dir.absoluteFilePath(filelistpath);
    file_log.setFileName(filelist);
    if (!file_log.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        QMessageBox::critical(this, tr(FATAL_ERROR), tr("Could not create the installed-files list"));
        emit exit_cc(1);
        return;
    }
    log_stream.setDevice(&file_log);
    dstFiles.append(filelistpath);
    for (auto it = dstFiles.begin(); it != dstFiles.end(); ++it) {
        log_stream << *it << "\n";
    }
    // List the directories in reverse order (starting at the leaves)
    for (auto it = dstDirectories.rbegin(); it != dstDirectories.rend(); ++it) {
        log_stream << *it << "/\n"; // suffix "/"
    }
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

//TODO: call this on error exits
void Installer::uninstallPartial()
{
    // Remove installed files and directories
    QStringList xdirs; // not uninstalled directories
    QStringList xfiles; // not uninstalled files
    for (auto it = dstFiles.begin(); it != dstFiles.end(); ++it) {
        if ( !dst_dir.remove(*it) ) {
            xfiles.append(*it);
        }
    }
    // Remove the directories in reverse order (starting at the leaves)
    for (auto it = dstDirectories.rbegin(); it != dstDirectories.rend(); ++it) {
        if ( !dst_dir.rmdir(*it) ) {
            xdirs.append(*it);
        }
    }

    // Report the results
    //TODO

}