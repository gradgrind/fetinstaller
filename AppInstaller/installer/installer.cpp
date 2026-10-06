#include "installer.h"
#include "ui_installer.h"
#include "copythread.h"
#include <QDirListing>
#include <QProcess>
#include <QPushButton>
#include <QFileDialog>
#include <QTimer>
#include <QStandardPaths>
#include <QMessageBox>

static const char *BAD_INSTALLER = QT_TRANSLATE_NOOP("Installer", R"(
  Please check that your installer has not been corrupted.<br>
  If necessary, contact the distributor.)");

void Installer::closeEvent(QCloseEvent *event)
{
    if ( installationPartial ) { // set to true during file copying, etc.
        event->ignore(); // don't close the window
    } else {
        event->accept();
    }
}

Installer::Installer(QLocale locale, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Installer)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    if ( !appinfo.init(locale) ) {
        return;
    }
    app_initialized = true;

    // Set application name in GUI
    ui->label_title->setText(ui->label_title->text().arg(appinfo.APPNAME, appinfo.APPLONGNAME));
    ui->label_page_0->setText(ui->label_page_0->text().arg(appinfo.APPNAME));
    ui->label_page_1->setText(ui->label_page_1->text().arg(appinfo.APPNAME));
    ui->removeExisting->setText(ui->removeExisting->text().arg(appinfo.APPNAME));
    ui->launch->setText(ui->launch->text().arg(appinfo.APPNAME));

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
    connect(ui->versioned, &QCheckBox::toggled, this, &Installer::refreshView);

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
    // but the processing should be so quick that this is not be visible.

    ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(false);
    scanComplete =false;
    scanOk = false;

    // Collect the files here for copying later: their paths are relative to the source root.
    // All files, except from the "_installer_" directory, are copied.
    QStringList installationFiles;
    QStringList installationDirs;
    QList<QPair<QString, QString>> installationLinks; // symlinks / Windows shortcuts

    src_dir = appinfo.SOURCE_DIR;

    // A simple check that the source directory is valid (contains an install bundle for the app)
    if ( QStandardPaths::findExecutable(
             appinfo.APPEXEC
             , QStringList() << src_dir.filePath(appinfo.EXECDIR)).isEmpty() ) {
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
    for ( const auto &dirEntry : QDirListing(
            src_dir.path(),
            F::Recursive | F::IncludeHidden) ) {
        QString rpath = src_dir.relativeFilePath(dirEntry.filePath());
        if ( rpath.startsWith("_installer_") )
            continue;
#if defined Q_OS_WIN
        // The installer is in the root directory
        if ( rpath.startsWith(QCoreApplication::applicationName()) )
            continue;
#endif
        linktest slink{testLink(rpath)};
        if ( !slink.message.isEmpty() ) {
            // link: error or warning
            if ( slink.link.isEmpty() ) {
                // error
                badfiles++;
                addBoldLine(ui->messages_0, slink.message);
                ui->messages_0->appendPlainText("");
            } else {
                // warning
                warnings++;
                ui->messages_0->appendPlainText(slink.message);
                ui->messages_0->appendPlainText("");
                installationLinks.append({slink.link, slink.target});
            }
        } else if ( !slink.link.isEmpty() ) {
            // valid link
            installationLinks.append({slink.link, slink.target});
        } else {
            // Normal file or directory?
            const QFileInfo finfo = dirEntry.fileInfo();
            if ( finfo.isDir() ) {
                // directory
                installationDirs.append(rpath);
            } else if ( finfo.isFile() ){
                // normal file
                if ( finfo.isReadable() ) {
                    installationFiles.append(rpath);
                } else {
                    addBoldLine(
                        ui->messages_0,
                        tr("ERROR, file not readable: %1").arg(rpath));
                    badfiles++;
                }
            } else {
                // something else!
                addBoldLine(
                    ui->messages_0,
                    tr("ERROR, invalid 'file': %1").arg(rpath));
                badfiles++;
            }
        }
    }
    // Save results
    installFiles.installationFiles = installationFiles;
    // Sort alphabetically, so parent directories always come before their children:
    installationDirs.sort();
    installFiles.installationDirs = installationDirs;
    installFiles.installationLinks = installationLinks;

    QApplication::restoreOverrideCursor();
    scanOk = badfiles == 0;
    if (scanOk) {
        // Allow continuation
        ui->buttonBox_0->button(QDialogButtonBox::Ok)->setEnabled(true);
    } else {
        addBoldLine(
            ui->messages_0,
            "–––––>>>");
        if ( src_dir.exists("_installer_/allow_file_errors") ) {
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
#ifdef Q_OS_WIN
    defaultInstallationPath = QDir::home().absoluteFilePath("AppData/Local/Programs");
#else
    defaultInstallationPath = QDir::home().absoluteFilePath(".local/apps");
#endif
    /* TODO-- Seek existing installation
    QString which_app{QStandardPaths::findExecutable(appinfo.APPEXEC)};
    if ( which_app.isEmpty() ) {
        ui->existing_app->setCurrentIndex(0);
    } else {
        ui->existing_app->setCurrentIndex(1);
        ui->existing_path->setText(which_app);

        QDir app_dir{QFileInfo{which_app}.canonicalFilePath()};
        app_dir.cdUp();

        //TODO: This will need adapting for Windows (powershell script ...)
        uninstall = QStandardPaths::findExecutable("app_uninstall", QStringList() << app_dir.path());
        if ( uninstall.isEmpty() ) {
            ui->existingCheckBox->hide();
        } else {
            ui->existingCheckBox->setChecked(true);
            ui->existingCheckBox->show();
        }
    }
    */
}

void Installer::page_2()
{
    /* TODO-- If a previous installation is to be uninstalled, do it now (if possible)
    if ( !uninstall.isEmpty() && ui->existingCheckBox->isChecked() ) {
        QProcess::execute(uninstall); // must wait for completion before returning!
    }
    */
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
    if ( !dir.isEmpty() )
        setInstallPath(dir);
}

void Installer::setInstallPath(QString ipath)
{
    uninstall_exe.clear();
    ui->removeExisting->hide();
    if ( ipath.isEmpty() ) {
        // Reentering setInstallpath, use existing path
        ipath = dst_dir.absolutePath();
    } else {
        ui->installPath->setText(ipath);
        dst_dir = ipath;
    }
    QString app_dstdir;
    if ( ui->versioned->isChecked() ) // toggling the check-box should call refreshView
        app_dstdir = dst_dir.absoluteFilePath(appinfo.APPNAME + "-" + appinfo.APPVERSION);
    else
        app_dstdir = dst_dir.absoluteFilePath(appinfo.APPNAME);
    if ( QFileInfo::exists(app_dstdir) ) {
        if ( !QFileInfo{app_dstdir}.isDir() ) {
            addBoldLine(ui->check_destination, tr("Destination not a folder: %1").arg(app_dstdir));
            return;
        }
    } else {
        dst_dir.mkpath(app_dstdir);
    }
    ui->setDefaultPath->setEnabled(ipath != defaultInstallationPath);

    // Check destination
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->check_destination->clear();
    // Destination writable? (not reliable on Windows?)
    if ( !QFileInfo{app_dstdir}.isWritable() ) {
        addBoldLine(ui->check_destination, tr("Destination not writable: %1").arg(app_dstdir));
        return;
    }
    // Check that the installation directory is empty
    app_dir = app_dstdir;
    if ( !app_dir.isEmpty() ) {
        addBoldLine(ui->check_destination, tr("Destination not empty: %1").arg(app_dir.path()));
        // Check for app uninstaller.
        //TODO: This may need tweaking for Windows.
        uninstall_exe = QStandardPaths::findExecutable(
            "app_uninstall",
            QStringList() << app_dir.filePath(appinfo.EXECDIR));
        if ( !uninstall_exe.isEmpty() ) {
            ui->removeExisting->show();
            addBoldLine(ui->check_destination, tr("To use the destination, uninstall the existing app."));
        }
        return;
    }
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
}

//TODO: Check all occurrences of dst_dir ... some should be app_dir!!!

void Installer::uninstallExisting()
{
    // Try to remove the installation in app_dir
    QProcess::execute(uninstall_exe);
    setInstallPath();
}

void Installer::refreshView()
{
    // Use this after manual changes to destination folder.
    setInstallPath();
}

void Installer::page_3()
{
    ui->installProgress->setMinimum(0);
    ui->installProgress->setValue(0);
    // Disable the checkboxes and OK button until the copying has finished
    ui->buttonBox_3->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->launch->setEnabled(false);

    ui->stackedWidget->setCurrentIndex(3);

    installationPartial = true;

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

    connect(copyWorker, &CopyWorker::dir_exists, this, &Installer::handleDirExists);
    connect(copyWorker, &CopyWorker::dir_written, this, &Installer::handleDirWritten);
    connect(copyWorker, &CopyWorker::dir_failed_write, this, &Installer::handleDirWriteFailed);

    connect(copyWorker, &CopyWorker::file_copied, this, &Installer::handleFileCopied);
    connect(copyWorker, &CopyWorker::failed_copy, this, &Installer::handleCopyFailed);
    connect(copyWorker, &CopyWorker::link_copied, this, &Installer::handleLinkCopied);
    connect(copyWorker, &CopyWorker::failed_link, this, &Installer::handleLinkFailed);

    connect(copyWorker, &CopyWorker::copying_done, this, &Installer::handleCopyingFinished);

    workerThread.start();

    // Start copying
    copyErrors.clear();
    emit doCopy(src_dir, app_dir, installFiles);
}

// In the slots below, the filepath arguments are all relative to the destination base

void Installer::handleNumberOfFiles(int n)
{
    ui->installProgress->setMaximum(n);
}

void Installer::handleDirWritten(QString filepath)
{
    progressOne();
    print_line("+ " + app_dir.filePath(filepath) + "/");
}

void Installer::handleDirExists(QString filepath)
{
    copyErrors.append(tr("ERROR, directory exists already: %1").arg(app_dir.filePath(filepath)));
}

void Installer::handleDirWriteFailed(QString filepath)
{
    copyErrors.append(tr("ERROR, could not create directory: %1").arg(app_dir.filePath(filepath)));
}

void Installer::print_line(QString line, bool bold)
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
        print_line("\nBUG: progress > 100%", true);
        bugflag = true; // suppress further reports
    }
}

void Installer::handleFileCopied(QString filepath)
{
    progressOne();
    print_line("+ " + app_dir.filePath(filepath));
}

void Installer::handleCopyFailed(QString filepath)
{
    copyErrors.append(tr("ERROR, could not copy file to: %1").arg(app_dir.filePath(filepath)));
}

void Installer::handleLinkCopied(QPair<QString, QString> filepaths)
{
    progressOne();
    print_line("+ " + app_dir.filePath(filepaths.second));
}

void Installer::handleLinkFailed(QPair<QString, QString> filepaths)
{
    copyErrors.append(
        tr("ERROR, could not link '%1' to: %2")
            .arg(app_dir.filePath(filepaths.first),
                app_dir.filePath(filepaths.second)));
}

void Installer::handleCopyingFinished()
{
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    print_line("");
    if ( copyErrors.isEmpty() ) {
        if ( registerApp() ) {
            installationPartial = false; // for page_4 handler => don't show page 4
            print_line(tr("Installation successful!"));
            ui->launch->setEnabled(true);
        } else {
            print_line(tr("App registration failed."), true);
            for ( const auto& f : std:: as_const(localfiles) ) {
                if ( !QFile::remove(f) ) {
                    print_line(tr("(Recovery:) Removal failed: %1").arg(f), true);
                }
            }
            ui->launch->setChecked(false);
        }
    } else {
        for ( const auto& e : std::as_const(copyErrors) ) {
            print_line(e, true);
        }
        print_line("");
        print_line("–––––>>>", true);
        print_line(tr("%1 installation errors").arg(copyErrors.length()), true);
        ui->launch->setChecked(false);
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
        if ( app_dir.removeRecursively() ) {
            ui->uninstall->appendPlainText(tr("Partial installation removed successfully"));
        } else {
            ui->uninstall->appendPlainText(
                tr("Partial installation could not be fully removed: '%1'").arg(app_dir.absolutePath()));
        }

        QApplication::restoreOverrideCursor();
        ui->buttonBox_4->button(QDialogButtonBox::Ok)->setEnabled(true);
        installationPartial = false;
    } else {
        // Installation complete, don't switch to the additional page
        if ( ui->launch->isChecked() ) {
            QProcess runapp;
            runapp.setProgram(app_dir.filePath(appinfo.EXECDIR + appinfo.APPEXEC));
            runapp.startDetached();
        }
        qApp->quit();
    }
}
