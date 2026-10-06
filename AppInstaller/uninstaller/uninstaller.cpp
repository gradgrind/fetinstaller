#include "uninstaller.h"
#include "ui_uninstaller.h"

#include <QFile>
#include <QPushButton>
#include <QTimer>
#include <QProcess>

static const char* UNINSTALL_INCOMPLETE = QT_TRANSLATE_NOOP("Uninstaller", R"(
The installation folder could not be deleted:
  %1

Please check its contents and delete manually.
A renewed installation will only be possible if the folder does not exist.
)");

Uninstaller::Uninstaller(AppInfo& app_info, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Uninstaller)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    basedir = app_info.basedir;

    // Set application name in GUI
    ui->label_title->setText(ui->label_title->text().arg(app_info.basename));
    ui->label_page_1->setText(ui->label_page_1->text().arg(app_info.basename));

    // Connect signals
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Uninstaller::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &QApplication::quit);

    ui->appinstall_path->setText(app_info.basedir.path());

    //NOTE: If the time is too short, a blank window might get shown at first ...
    QTimer::singleShot(100, this, &Uninstaller::page_1);
}

Uninstaller::~Uninstaller() {
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void Uninstaller::print_line(QString line, bool bold)
{
    if ( !bugflag ) {
        if ( bold )
            ui->text_2->appendHtml("<b>" + line + "</b");
        else
            ui->text_2->appendPlainText(line);
    }
}

void Uninstaller::page_1()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->text_1->clear();
}

void Uninstaller::page_2()
{
    ui->stackedWidget->setCurrentIndex(1);

    // Disable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(false);

    unregisterApp();

    // Initialize progress bar

    // Loop through the directory contents
    int count{0};
    for ( const auto &dirEntry : QDirListing(
             basedir.path(),
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
    print_line(tr("*** Remove installation files ***"));
    print_line("");
    emit deleteFiles(basedir.path());
}

void Uninstaller::file_deleted(QString fpath, bool ok)
{
    if ( ok ) {
        print_line(" - " + fpath);
    } else {
        print_line("!X! " + fpath, true);
    }
    progressOne();
}

void Uninstaller::dir_removed(QString fpath, bool ok)
{
    if ( ok ) {
        print_line(" -- " + fpath + "/");
    } else {
        print_line("!X/! " + fpath + "/", true);
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
    if ( ok ) {
        print_line("");
        // Test if the app installation directory still exists.
        if ( basedir.exists() ) {
            print_line("");
            print_line(tr(UNINSTALL_INCOMPLETE).arg(basedir.path()));
        } else {
            if ( unregister_failed.isEmpty() )
                print_line(tr("App successfully uninstalled."));
            else {
                print_line(tr("System files not uninstalled:"), true);
                for ( const auto& f : std::as_const(unregister_failed) ) {
                    print_line(tr(" - Couldn't remove %1").arg(f), true);
                }
            }
        }
    }
    // Enable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
}
