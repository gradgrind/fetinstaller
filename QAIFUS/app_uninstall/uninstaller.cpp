#include "uninstaller.h"
#include "ui_uninstaller.h"
#include "checkinuse.h"

#include <QFile>
#include <QPushButton>
#include <QTimer>
#include <QProcess>

static const char* UNINSTALL_INTRO = QT_TRANSLATE_NOOP("Uninstaller", R"(
The folder above will be deleted.
Also any associated items which were installed with this package,
such as "start-menu" entries and file-ending associations,
will be removed.
)");

static const char* UNINSTALL_BLOCKED = QT_TRANSLATE_NOOP("Uninstaller", R"(
BLOCKED: Files in the folder above are in use by an active process.
Before uninstalling this package, the process with ID %1 must be terminated.
)");

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
    setWindowIcon(QIcon(":/icons/uninstaller.svg"));

    base_dir = app_info.basedir;

    // Set application name in GUI
    ui->label_title->setText(ui->label_title->text().arg(app_info.basename));
    ui->label_qaifus_version->setText(ui->label_qaifus_version->text().arg(app_info.QAIFUS_VERSION));
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
    auto pid = dirInUse(base_dir, QCoreApplication::applicationPid());
    if ( pid != 0 ) {
        ui->text_1->setPlainText(tr(UNINSTALL_BLOCKED).arg(pid));
        ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);
    } else {
        ui->text_1->setPlainText(tr(UNINSTALL_INTRO));
    }
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
             base_dir.path(),
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
    emit deleteFiles(base_dir.path());
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
        if ( base_dir.exists() ) {
            print_line("");
            print_line(tr(UNINSTALL_INCOMPLETE).arg(base_dir.path()));
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
