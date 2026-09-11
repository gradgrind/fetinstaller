#include "uninstaller.h"
#include "ui_uninstaller.h"

#include <QFile>
#include <QPushButton>
#include <QTimer>
#include <QProcess>

static const char* CORRUPT_INSTALLATION = QT_TRANSLATE_NOOP("Uninstaller", R"(
It looks like the installation has been corrupted.

Continuing to uninstall might not produce the desired results. Consider carefully
whether you want to proceed.
)");

//TODO: Consider allowing certain files to be ignored when checking for emptiness.
// This would allow, say, configuration files to be retained when updating an
// installation.
static const char* DIR_NOT_EMPTY = QT_TRANSLATE_NOOP("Uninstaller", R"(
The installation directory is not empty:
  %1

Please check its contents and delete manually, if they are no longer required.
A renewed installation prefers an empty folder.
)");

Uninstaller::Uninstaller(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Uninstaller)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // Set application name in GUI
    ui->label_title->setText(ui->label_title->text().arg(appinfo.APPNAME));
    ui->label_page_1->setText(ui->label_page_1->text().arg(appinfo.APPNAME));

    // Connect signals
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Uninstaller::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &QApplication::quit);

    basedir.setPath(QCoreApplication::applicationDirPath());
    basedir.makeAbsolute();
    basedir.cdUp(); // base directory of installationProcess

    ui->appinstall_path->setText(basedir.path());

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
        ui->filesReport->appendPlainText(line);
}

void Uninstaller::page_1()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->filesReport->clear();

    // Default installation path
#ifdef Q_OS_WIN
    defaultInstallationPath = QDir::home().absoluteFilePath("AppData/Local/Programs/%1").arg(APPNAME);
#else
    defaultInstallationPath = QDir::home().absoluteFilePath(".local");
#endif

    // Read the list of installed files
    QString filespath{basedir.filePath(appinfo.APPFILES + "installed_files")};
    QFile textFile{filespath};
    print_line(tr("Reading file list from: %1").arg(filespath));
    print_line("");
    if ( !textFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        print_line(tr("*** CRITICAL ERROR: couldn't read file list ***"));
        return;
    }
    QTextStream textStream(&textFile);
    filesList.clear();
    dirsList.clear();
    linksList.clear();
    int errors{0};
    while ( true )
    {
        QString line = textStream.readLine();
        if ( line.isNull() )
            break;    // end of file
        QString rpath{line.trimmed()};
        if ( rpath.isEmpty() )
            continue;
        QFileInfo fr{rpath};
        if ( fr.isAbsolute() || rpath.contains("..") ) {
            // Only allow relative paths without "..", i.e. guaranteed to be within the package.
            print_line(tr("Invalid line in file list: %1").arg(rpath));
            errors++;
            continue;
        }
        QString fpath{basedir.absoluteFilePath(rpath)};
        QFileInfo f{fpath};
        if ( f.isSymLink() ) {
            linksList.append(fpath);
        } else if ( f.isDir() ) {
            dirsList.append(fpath);
        } else if ( f.exists() ) {
            filesList.append(fpath);
        } else {
            print_line(tr("File not found: %1").arg(fpath));
            errors++;
        }
    }
    if ( errors == 0 ) {
        print_line(tr("Press OK to uninstall."));
    } else {
        print_line(tr(CORRUPT_INSTALLATION));
        return;
    }
    // Add the root directory, basedir
    dirsList.append(basedir.path());
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void Uninstaller::page_2()
{
    ui->stackedWidget->setCurrentIndex(1);

    // Disable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(false);

    // Initialize progress bar
    ui->uninstallProgress->setMinimum(0);
    ui->uninstallProgress->setMaximum(linksList.length() + filesList.length() + dirsList.size());
    ui->uninstallProgress->setValue(0);

    // Use background thread to perform deletions
    worker = new DeleteWorker;
    worker->moveToThread(&workerThread);

    // Connect signals
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &Uninstaller::deleteFiles, worker, &DeleteWorker::deleteFiles);

    connect(worker, &DeleteWorker::deletedFile, this, &Uninstaller::file_deleted);
    connect(worker, &DeleteWorker::removedDir, this, &Uninstaller::dir_removed);
    connect(worker, &DeleteWorker::finished, this, &Uninstaller::done);

    workerThread.start();

     // Start deleting.
    failed_files.clear();
    failed_dirs.clear();
    ui->output->clear();
    // The directories should already be sorted correctly (longest first), so that
    // leaf directories will come before parent directories.
    emit deleteFiles(linksList, filesList, dirsList);
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

    int fileCount = filesList.length() + linksList.length() - failed_files.length();
    int dirCount = dirsList.length() - failed_dirs.length();
    print_line("");
    print_line(tr("%1 files deleted").arg(fileCount));
    print_line(tr("%1 directories removed").arg(dirCount));

    if ( basedir.path() == defaultInstallationPath ) {
        unregisterApp();
    } else {
        // Seek remaining directories, test if empty.
        if ( basedir.exists() ) {
            print_line("");
            print_line(tr(DIR_NOT_EMPTY).arg(basedir.path()));
        }
    }

    // Enable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
}
