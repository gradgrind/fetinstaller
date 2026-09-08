#include "installer.h"
#include "ui_installer.h"
#include "copythread.h"
#include <QDirListing>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QFileDialog>
#include <QTimer>

static const char *BAD_INSTALLER = QT_TRANSLATE_NOOP("Installer", R"(
  Please check that your installer has not been corrupted.
  If necessary, contact the distributor.)");

static const char *WARN_EXISTING = QT_TRANSLATE_NOOP("Installer", R"(
There seems to be a FET installation at '%1' already.<br>
You must remove this before you can install the new version here.)");

static const char *WARN_NOT_EMPTY = QT_TRANSLATE_NOOP("Installer", R"(
The installation directory is not empty.<br>
Check that you really want to place the installation there.)");

void Installer::closeEvent(QCloseEvent *event)
{
    if ( installationPartial ) { // set to true during file copying, etc.
        event->ignore(); // don't close the window
    } else {
        event->accept();
    }
}

Installer::Installer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Installer)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // *** Connect signals ***

    // page switching
    connect(ui->buttonBox_0, &QDialogButtonBox::accepted, this, &Installer::page_1);
    connect(ui->buttonBox_0, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Installer::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &Installer::page_3);
    connect(ui->buttonBox_2, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_3, &QDialogButtonBox::accepted, this, &Installer::page_4);
    connect(ui->buttonBox_4, &QDialogButtonBox::accepted, qApp, &QApplication::quit);

    // select destination directory
    connect(ui->refresh_2, &QPushButton::clicked, this, &Installer::refreshView);
    connect(ui->setDefaultPath, &QPushButton::clicked, this, &Installer::selectDefaultDir);
    connect(ui->installPathBrowse, &QToolButton::clicked, this, &Installer::selectInstallDir);
    connect(ui->removeExisting, &QPushButton::clicked, this, &Installer::uninstallExisting);
    connect(ui->installNonEmpty, &QCheckBox::clicked, this, &Installer::allowNonEmpty);

    //NOTE: If the time is too short, a blank window might get shown at first ...
    QTimer::singleShot(100, this, &Installer::page_0);
}

Installer::~Installer() {
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void addBoldLine(QPlainTextEdit* e, QString line)
{
    e->appendHtml("<b>" + line + "</b");
}

void Installer::page_0()
{
    // This runs quickly enough not to be run in a background thread. It sets a busy cursor,
    // but the processing should be so quick that this will not be visible.

    ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(false);
    scanComplete =false;
    scanOk = false;

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

        addBoldLine(ui->messages_0, "BUG: installation files not found.");
        addBoldLine(ui->messages_0, tr(BAD_INSTALLER));
        return;
    }

    // Collect files to be installed
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

    using F = QDirListing::IteratorFlag;
    // Recursive search, but don't recurse into symlinked directories.
    int badfiles{0};
    int warnings{0};
    for (const auto &dirEntry : QDirListing(
             src_dir.path(),
             F::Recursive | F::IncludeHidden | F::ExcludeOther | F::ResolveSymlinks | F::IncludeBrokenSymlinks)) {
        QString rpath = src_dir.relativeFilePath(dirEntry.filePath());
        if (rpath.startsWith("_")) {
            continue;
        }
        const QFileInfo finfo = dirEntry.fileInfo();
        if (finfo.isSymLink()) {
            // I need to test whether QFile::link can create links with non-existent targets and
            // whether directory links work the same as file links.
            QString linkPath = finfo.readSymLink(); // target path, relative or absolute
            bool linkTargetExists = finfo.exists();
            if (QFileInfo(linkPath).isRelative()) {
                // A relative link within the install package is acceptable, as long as its target exists.
                // A relative link outside the package is an error.
                QString lrpath = src_dir.relativeFilePath(finfo.symLinkTarget());
                if (lrpath.startsWith("..")) {
                    // outside the package
                    addBoldLine(
                        ui->messages_0,
                        tr("ERROR, relative symlink outside package: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->messages_0->appendPlainText("");
                    badfiles++;
                } else {
                    // within the package
                    if (linkTargetExists) {
                        installationLinksRel.append({rpath, linkPath});
                    } else {
                        addBoldLine(
                            ui->messages_0,
                            tr("ERROR, target missing for relative symlink: %1 -> %2")
                                .arg(rpath, linkPath));
                        ui->messages_0->appendPlainText("");
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
                    ui->messages_0->appendPlainText(
                        tr("WARNING, absolute symlink: %1 -> %2%3")
                            .arg(rpath, linkPath, x));
                    warnings++;
                    ui->messages_0->appendPlainText("");
                    installationLinksAbs.append({rpath, linkPath});
                } else {
                    // inside the package
                    addBoldLine(
                        ui->messages_0,
                        tr("ERROR, absolute symlink within package: %1 -> %2")
                            .arg(rpath, linkPath));
                    ui->messages_0->appendPlainText("");
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
                    addBoldLine(
                        ui->messages_0,
                        tr("ERROR, file not readable: %1").arg(rpath));
                    badfiles++;
                }
            }
        }
    }
    // Save results
    installFiles.installationFiles = installationFiles;
    // Sort alphabetically, so parent directories always come before their children:
    installationDirs.sort();
    installFiles.installationDirs = installationDirs;
    installFiles.installationLinksRel = installationLinksRel;
    installFiles.installationLinksAbs = installationLinksAbs;

    QApplication::restoreOverrideCursor();
    scanOk = badfiles == 0;
    if (scanOk) {
        // Allow continuation
        ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        addBoldLine(
            ui->messages_0,
            "–––––>>>");
        if ( src_dir.exists("_bin/allow_file_errors") ) {
            addBoldLine(
                ui->messages_0,
                tr("%1 invalid files – installation is not recommended, rather fix the source!")
                    .arg(badfiles));
            ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(true);
        } else {
            addBoldLine(
                ui->messages_0,
                tr("%1 invalid files – please fix the source!")
                    .arg(badfiles));
        }
    }
    scanComplete = true;

    if ( scanOk && warnings == 0 )
        // If there is nothing to report, jump straight to the next page
        page_1();
}

void Installer::page_1()
{
    ui->stackedWidget->setCurrentIndex(1);

    // Default installation path
    defaultInstallationPath = QDir::home().absoluteFilePath(".local");

    // Seek existing installation
    QString which_fet;
    bool which_fet_failed{false};
    QProcess process;
    process.start("which", QStringList() << "fet");
    process.waitForFinished(1000);
    if ( process.state() == QProcess::NotRunning ) {
        if (process.exitStatus() == QProcess::NormalExit) {
            if ( process.exitCode() == 0 ) {
                which_fet = process.readAllStandardOutput();
                which_fet = which_fet.trimmed();
            }
        } else {
            which_fet_failed = true;
        }
    } else {
        process.kill();
        which_fet_failed = true;
    }
    if ( which_fet_failed ) {
        ui->existing_fet->setCurrentIndex(2);
    } else if ( which_fet.isEmpty() ) {
        ui->existing_fet->setCurrentIndex(0);
    } else {
        ui->existing_fet->setCurrentIndex(1);
        ui->existing_path->setText(which_fet);

        QDir fet_dir{QFileInfo{which_fet}.canonicalPath()};
        uninstall = fet_dir.filePath("fet_uninstall");
        if (QFileInfo::exists(uninstall)) {
            ui->existingCheckBox->setChecked(true);
            ui->existingCheckBox->show();
        } else {
            uninstall.clear();
            ui->existingCheckBox->hide();
        }
    }
}

void Installer::page_2()
{
    // If a previous installation is to be uninstalled, do it now (if possible)
    if (!uninstall.isEmpty() && ui->existingCheckBox->isChecked()) {
        QProcess::execute(uninstall);
    }
    ui->stackedWidget->setCurrentIndex(2);
    setInstallPath(defaultInstallationPath);
}

void Installer::selectDefaultDir()
{
    setInstallPath(defaultInstallationPath);
}

void Installer::selectInstallDir()
{
    QString p0{ui->installPath->text()};
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Directory"),
        p0 == defaultInstallationPath ? QDir::homePath() : p0,
        QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty()) {
        setInstallPath(dir);
    }
}

void Installer::allowNonEmpty(bool checked)
{
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(checked);
}

void Installer::setInstallPath(QString ipath)
{
    ui->removeExisting->hide();
    if ( ipath.isEmpty() ) {
        // Reentering setInstallpath, use existing path
        ipath = dst_dir.absolutePath();
        if ( !dst_dir.exists() ) {
            dst_dir.mkdir(ipath);
        }
    } else {
        ui->installPath->setText(ipath);
        dst_dir = ipath;
    }
    ui->setDefaultPath->setEnabled(ipath != defaultInstallationPath);
    ui->desktopSetup->hide();
    ui->installNonEmpty->setChecked(false);
    ui->installNonEmpty->hide();

    // Check destination
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->would_overwrite->clear();
    // Destination writable? (not reliable on Windows?)
    if ( !QFileInfo{ipath}.isWritable() ) {
        addBoldLine(ui->would_overwrite, tr("Destination not writable: %1").arg(ipath));
        return;
    }
    // Check for FET installation here
    if ( dst_dir.exists("bin/fet") ) {
        addBoldLine(ui->would_overwrite, tr(WARN_EXISTING).arg(dst_dir.path()));
        // Check for FET uninstaller
        if ( dst_dir.exists("bin/fet_uninstall") ) {
            ui->removeExisting->show();
        }
        return;
    }
    // Check for overwrites
    if ( dst_dir.exists() ) {
        if ( !dst_dir.isEmpty() ) {
            // Check for overwrites ...
            bool ok{true};
            addBoldLine(
                ui->would_overwrite,
                tr("These files exist already (blocking installation):"));
            ui->would_overwrite->appendPlainText("");
            QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

            // Test for the existence of each file in the destination directory
            for ( const auto& f : std::as_const(installFiles.installationFiles) ) {
                if ( dst_dir.exists(f) ) {
                    ok = false;
                    ui->would_overwrite->appendPlainText(dst_dir.filePath(f));
                }
            }
            for ( const auto& fpair : std::as_const(installFiles.installationLinksAbs) ) {
                QFileInfo f{dst_dir.filePath(fpair.first)};
                if ( f.exists() || !f.readSymLink().isEmpty() ) {
                    ok = false;
                    ui->would_overwrite->appendPlainText(f.filePath());
                }
            }
            for ( const auto& fpair : std::as_const(installFiles.installationLinksRel) ) {
                QFileInfo f{dst_dir.filePath(fpair.first)};
                if ( f.exists() || !f.readSymLink().isEmpty() ) {
                    ok = false;
                    ui->would_overwrite->appendPlainText(f.filePath());
                }
            }

            // Check any existing directories are valid directories and writable
            for ( const auto& d : std::as_const(installFiles.installationDirs) ) {
                QFileInfo dd{dst_dir.filePath(d)};
                if ( dd.exists() ) {
                    if ( !dd.isDir() || !dd.isWritable() ) {
                        ok = false;
                        ui->would_overwrite->appendPlainText(dd.filePath());
                    }
                } else if ( !dd.readSymLink().isEmpty() ) {
                    ok = false;
                    ui->would_overwrite->appendPlainText(dd.filePath());
                }
            }
            QApplication::restoreOverrideCursor();

            if ( ok ) {
                ui->would_overwrite->clear();
                // Check for empty installation directory.
                // In the default installation directory this is to be expected, but
                // otherwise probably not.
                if ( ipath != defaultInstallationPath && !dst_dir.isEmpty() ) {
                    addBoldLine(
                        ui->would_overwrite,
                        tr(WARN_NOT_EMPTY));
                    ui->installNonEmpty->show();
                    return;
                }
            } else {
                ui->would_overwrite->appendPlainText("");
                addBoldLine(
                    ui->would_overwrite,
                    "–––––>>>");
                addBoldLine(
                    ui->would_overwrite,
                    tr("*** Installation is not possible ***"));
                return;
            }
        }
    } else {
        // Destination folder doesn't exist
        if ( !dst_dir.mkdir(ipath) ) {
            addBoldLine(
                ui->would_overwrite,
                tr("Couldn't create installation folder: %1").arg(ipath));
            return;
        }
    }
    if ( ipath == defaultInstallationPath ) {
        ui->desktopSetup->show();
    }
    ui->would_overwrite->appendPlainText(tr("No conflicts."));
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void Installer::uninstallExisting()
{
    // Try to remove the installation in dst_dir
    QProcess::execute(dst_dir.filePath("bin/fet_uninstall"));
    setInstallPath();
}

void Installer::refreshView()
{
    setInstallPath();
}

void Installer::page_3()
{
    ui->installProgress->setMinimum(0);
    ui->installProgress->setValue(0);
    // Disable the OK button until the copying has finished
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(false);

    ui->stackedWidget->setCurrentIndex(3);

    installationPartial = true;

    dstDirectories.clear(); // collect the directories in the installation
    dstFiles.clear(); // collect the files in the installation

    // Use background thread to perform copying

    copyWorker = new CopyWorker;
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
    copyErrors.clear();
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
    progressOne();
    print_3("+ " + dst_dir.filePath(filepath) + "/");
}

void Installer::handleDirNotCopied(QString filepath)
{
    dstDirectories.append(filepath);
    progressOne();
    print_3("(+) " + dst_dir.filePath(filepath) + "/");
}

void Installer::handleDirWriteFailed(QString filepath)
{
    copyErrors.append(tr("ERROR, could not create directory: %1").arg(dst_dir.filePath(filepath)));
}

void Installer::handleDirOverwriteFailed(QString filepath)
{
    copyErrors.append(tr("ERROR, existing item is not writable directory: %1").arg(dst_dir.filePath(filepath)));
}

void Installer::print_3(QString line, bool bold)
{
    if ( !bugflag ) {
        if ( bold )
            addBoldLine(ui->installDetails, line);
        else
            ui->installDetails->appendPlainText(line);
    }
}

void Installer::progressOne()
{
    int p = ui->installProgress->value();
    int max = ui->installProgress->maximum();
    if ( p < max ) {
        ui->installProgress->setValue(p + 1);
    } else if ( !bugflag ) {
        print_3("\nBUG: progress > 100%", true);
        bugflag = true; // suppress further reports
    }
}

void Installer::handleFileCopied(QString filepath)
{
    dstFiles.append(filepath);
    progressOne();
    print_3("+ " + dst_dir.filePath(filepath));
}

void Installer::handleCopyFailed(QString filepath)
{
    copyErrors.append(tr("ERROR, could not copy file to: %1").arg(dst_dir.filePath(filepath)));
}

void Installer::handleLinkCopied(QPair<QString, QString> filepaths)
{
    dstFiles.append(filepaths.first);
    progressOne();
    print_3("+ " + dst_dir.filePath(filepaths.second));
}

void Installer::handleLinkFailed(QPair<QString, QString> filepaths)
{
    copyErrors.append(
        tr("ERROR, could not link '%1' to: %2")
            .arg(dst_dir.filePath(filepaths.first),
                dst_dir.filePath(filepaths.second)));
}

void Installer::handleCopyingFinished()
{
    ui->launch->setChecked(false);
    ui->launch->hide();
    // This seems to run quickly. If it should turn out to block the GUI for too long,
    // it should perhaps be moved to a background thread.
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if ( copyErrors.isEmpty() ) {
        if ( dst_dir.absolutePath() == defaultInstallationPath ) {
            // Only perform these operations if installing to "~/.local". For them to work with
            // other installation locations, the relevant (modified) files would still need to
            // be placed in "~/.local".

            // Update file-type associations and desktop files
            print_3("");
            print_3("update-mime-database");
            QProcess::execute("update-mime-database",
                              QStringList() << dst_dir.absoluteFilePath("share/mime"));
            print_3("update-desktop-database");
            QProcess::execute("update-desktop-database",
                              QStringList() << dst_dir.absoluteFilePath("share/applications"));
        }

        // Open file to record installed files
        QString filelistpath{"share/fet/installed_files"};
        filelist = dst_dir.absoluteFilePath(filelistpath);
        file_log.setFileName(filelist);
        if (file_log.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            log_stream.setDevice(&file_log);
            // List the files in reverse order (starting with the symlinks)
            for (auto it = dstFiles.rbegin(); it != dstFiles.rend(); ++it) {
                log_stream << *it << "\n";
            }
            // Add the installed-files list.
            log_stream << filelistpath << "\n";
            // List the directories in reverse order (starting at the leaves)
            for (auto it = dstDirectories.rbegin(); it != dstDirectories.rend(); ++it) {
                log_stream << *it << "/\n"; // suffix "/"
            }
            file_log.close();
            ui->launch->setChecked(true);
            ui->launch->show();
            installationPartial = false;
        } else {
            print_3("");
            print_3("–––––>>>", true);
            print_3(tr("ERROR, could not create the installed-files list"), true);
        }
    } else {
        print_3("");
        for ( const auto& e : std::as_const(copyErrors) ) {
            print_3(e, true);
        }
        print_3("");
        print_3("–––––>>>", true);
        print_3(tr("%1 copying errors").arg(copyErrors.length()), true);
    }
    QApplication::restoreOverrideCursor();
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void Installer::page_4()
{
    if ( installationPartial ) {
        // Installation incomplete, go to the additional page to tidy up
        ui->stackedWidget->setCurrentIndex(4);
        ui->buttonBox_4->button(QDialogButtonBox::Ok)->setEnabled(false);

        // Remove installed files and directories
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

        connect(this, &Installer::doRemove, copyWorker, &CopyWorker::removePartial);
        connect(copyWorker, &CopyWorker::remove_file, this, &Installer::removedFile);
        connect(copyWorker, &CopyWorker::remove_dir, this, &Installer::removedDir);
        connect(copyWorker, &CopyWorker::removing_done, this, &Installer::removingDone);

        xdirs.clear(); // not uninstalled directories
        xfiles.clear(); // not uninstalled files
        emit doRemove(dst_dir, dstDirectories, dstFiles);
    } else {
        // Installation complete, don't switch to the additional page
        if (ui->launch->isChecked()) {
            QProcess runfet;
            runfet.setProgram(dst_dir.filePath("bin/fet"));
            runfet.startDetached();
        }
        qApp->quit();
    }
}

void Installer::removedFile(QString f, bool ok) {
    if ( ok ) {
        ui->uninstall->appendPlainText(" - " + dst_dir.filePath(f));
    } else {
        xfiles.append(f);
    }
}

void Installer::removedDir(QString f, bool ok) {
    if ( ok ) {
        ui->uninstall->appendPlainText(" -/ " + dst_dir.filePath(f));
    } else {
        xdirs.append(f);
    }
}

void Installer::removingDone() {
    for ( const auto& f : std::as_const(xfiles) ) {
        addBoldLine(ui->uninstall, tr("ERROR, could not remove: %1").arg(dst_dir.filePath(f)));
    }
    for ( const auto& f : std::as_const(xdirs) ) {
        addBoldLine(ui->uninstall, tr("WARNING, could not remove directory: %1/").arg(dst_dir.filePath(f)));
    }
    ui->uninstall->appendPlainText("");
    addBoldLine(
        ui->uninstall,
        "–––––>>>");
    if ( !xfiles.isEmpty() ) {
        addBoldLine(
            ui->uninstall,
            tr("%1 files could not be removed.").arg(xfiles.length()));
    }
    if ( !xdirs.isEmpty() ) {
        addBoldLine(
            ui->uninstall,
            tr("%1 folders could not be removed.").arg(xdirs.length()));
    } else if ( xfiles.isEmpty() ) {
        ui->uninstall->appendPlainText(tr("Incomplete installation removed."));
    }
    installationPartial = false;
    QApplication::restoreOverrideCursor();
    ui->buttonBox_4->button(QDialogButtonBox::Ok)->setEnabled(true);
}
