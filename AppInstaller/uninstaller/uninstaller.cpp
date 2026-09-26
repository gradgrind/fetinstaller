#include "uninstaller.h"
#include "ui_uninstaller.h"

#include <QFile>
#include <QPushButton>
#include <QTimer>
#include <QProcess>

static const char* REGISTERED_INSTALLATION = QT_TRANSLATE_NOOP("Uninstaller", R"(
This will unregister the application and remove it from the desktop.
)");

static const char* UNREGISTERED_INSTALLATION = QT_TRANSLATE_NOOP("Uninstaller", R"(
This installation is not registered – only the installation directory will be removed.
)");

static const char* DIR_NOT_EMPTY = QT_TRANSLATE_NOOP("Uninstaller", R"(
The installation directory is not empty:
  %1

Please check its contents and delete manually.
A renewed installation will only be possible if the folder does not exist.
)");

Uninstaller::Uninstaller(AppInfo* app_info, QWidget *parent)
    : appinfo{app_info}
    , QWidget(parent)
    , ui(new Ui::Uninstaller)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // Set application name in GUI
    ui->label_title->setText(ui->label_title->text().arg(appinfo->APPNAME));
    ui->label_page_1->setText(ui->label_page_1->text().arg(appinfo->APPNAME));

    // Connect signals
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Uninstaller::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &QApplication::quit);

    ui->appinstall_path->setText(appinfo->basedir.path());

    //NOTE: If the time is too short, a blank window might get shown at first ...
    QTimer::singleShot(100, this, &Uninstaller::page_1);
}

Uninstaller::~Uninstaller() {
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void Uninstaller::print_line(QString line)
{
    if ( !bugflag )
        ui->text_2->appendPlainText(line);
}

void Uninstaller::page_1()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->text_1->clear();
    if ( appinfo->registered ) {
        ui->text_1->appendPlainText(tr(REGISTERED_INSTALLATION));
    } else {
        ui->text_1->appendPlainText(tr(UNREGISTERED_INSTALLATION));
    }
}

void Uninstaller::page_2()
{
    ui->stackedWidget->setCurrentIndex(1);

    // Disable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Initialize progress bar

    // Loop through the directory contents
    int count{0};
    for ( const auto &dirEntry : QDirListing(
             appinfo->basedir.path(),
             QDirListing::IteratorFlag::IncludeHidden | QDirListing::IteratorFlag::Recursive) ) {
        count++;
    }


    ui->uninstallProgress->setMinimum(0);
    ui->uninstallProgress->setMaximum(count + 1); // include base directory
    ui->uninstallProgress->setValue(0);

    // Use background thread to perform deletions
    worker = new DeleteWorker;
    worker->moveToThread(&workerThread);

    // Connect signals
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Uninstaller::deleteFiles, worker, &DeleteWorker::deleteFiles);

    connect(worker, &DeleteWorker::deletedFile, this, &Uninstaller::file_deleted);
    connect(worker, &DeleteWorker::removedDir, this, &Uninstaller::dir_removed);
    connect(worker, &DeleteWorker::done, this, &Uninstaller::done);

    workerThread.start();

     // Start deleting.
    failed_files.clear();
    failed_dirs.clear();
    ui->text_2->clear();
    // The directories should already be sorted correctly (longest first), so that
    // leaf directories will come before parent directories.
    emit deleteFiles(appinfo->basedir.path());
}

void Uninstaller::file_deleted(QString fpath, bool ok)
{
    if ( ok ) {
        print_line(" - " + fpath);
    } else {
        failed_files.append(fpath);
    }
    progressOne();
}

void Uninstaller::dir_removed(QString fpath, bool ok)
{
    if ( ok ) {
        print_line(" -/ " + fpath);
    } else {
        failed_files.append(fpath);
    }
    progressOne();
}

void Uninstaller::progressOne()
{
    int p = ui->uninstallProgress->value();
    int max = ui->uninstallProgress->maximum();
    if ( p < max ) {
        ui->uninstallProgress->setValue(p + 1);
    } else if ( !bugflag ) {
        print_line("\nBUG: progress > 100%");
        bugflag = true; // suppress further reports
    }
}

void Uninstaller::done(bool ok)
{
    if ( !ok ) {
        // Enable ok button
        ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
        return;
    }

    print_line("");
    print_line(tr("%1 files could not be deleted").arg(failed_files.length()));
    print_line(tr("%1 directories removed").arg(failed_dirs.length()));

    if ( appinfo->registered ) {
        unregisterApp();
    }
    // Seek remaining directories, test if empty.
    if ( appinfo->basedir.exists() ) {
        print_line("");
        print_line(tr(DIR_NOT_EMPTY).arg(appinfo->basedir.path()));
    }

    // Enable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
}
