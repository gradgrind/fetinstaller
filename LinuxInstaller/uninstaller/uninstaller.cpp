#include "uninstaller.h"
#include <ui_uninstaller.h>
#include "deleteworker.h"

#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QProcess>

static const char* INSTALLED_FILES = "share/fet/installed_files";

//TODO: Remove some of these ...

static const char* FATAL_ERROR = QT_TRANSLATE_NOOP("Uninstaller", "Fatal Error");

static const char* WARNING = QT_TRANSLATE_NOOP("Uninstaller", "Warning");

static const char* ERROR1 = QT_TRANSLATE_NOOP("Uninstaller", R"(
%1 files not within installation base directory (lines starting with "!!!")
%2 files not found (lines starting with "***")

Continue, deleting the other %3 files?
)");

static const char* CORRUPT_INSTALLATION = QT_TRANSLATE_NOOP("Uninstaller", R"(
It looks like the installation has been corrupted.

Continuing to uninstall might not produce the desired results. Consider carefully
whether you want to proceed.
)");

void Uninstaller::fatalError(QString msg)
{
    QMessageBox::critical(
        this,
        QCoreApplication::translate("Uninstaller", FATAL_ERROR),
        msg);
}

void Uninstaller::warning(QString msg)
{
    QMessageBox::warning(
        this,
        QCoreApplication::translate("Uninstaller", WARNING),
        msg);
}

Uninstaller::Uninstaller(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Uninstaller)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // Connect signals
    connect(ui->buttonBox_1, &QDialogButtonBox::accepted, this, &Uninstaller::page_2);
    connect(ui->buttonBox_1, &QDialogButtonBox::rejected, qApp, &QApplication::quit);
    connect(ui->buttonBox_2, &QDialogButtonBox::accepted, this, &QApplication::quit);

    basedir.setPath(QCoreApplication::applicationDirPath());
    basedir.makeAbsolute();
    basedir.cdUp(); // base directory of installationProcess

    ui->fetinstall_path->setText(basedir.path());

    //NOTE: If the time is too short, a blank window might get shown at first ...
    QTimer::singleShot(100, this, &Uninstaller::page_1);
}

Uninstaller::~Uninstaller() {
    workerThread.quit();
    workerThread.wait();
    delete ui;
}

void Uninstaller::page_1()
{
    ui->stackedWidget->setCurrentIndex(0);
    ui->buttonBox_1->button(QDialogButtonBox::Ok)->setEnabled(false);
    ui->filesReport->clear();

    // Read the list of installed files
    QString filespath{basedir.filePath(INSTALLED_FILES)};
    QFile textFile{filespath};
    ui->filesReport->appendPlainText(tr("Reading file list from: %s").arg(filespath));
    ui->filesReport->appendPlainText("");
    if ( !textFile.open(QIODevice::ReadOnly | QIODevice::Text) ) {
        ui->filesReport->appendPlainText(tr("*** CRITICAL ERROR: couldn't read file list ***"));
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
        if ( rpath.startsWith("/") || rpath.startsWith("..") ) {
            // Only allow relative paths within the installer package.
            ui->filesReport->appendPlainText(tr("Invalid line in file list: %1").arg(rpath));
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
            ui->filesReport->appendPlainText(tr("File not found: %1").arg(fpath));
            errors++;
        }
    }
    if ( errors == 0 ) {
        ui->filesReport->appendPlainText(tr("Press OK to uninstall."));
    } else {
        ui->filesReport->appendPlainText(tr(CORRUPT_INSTALLATION));
    }
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
    DeleteWorker* worker = new DeleteWorker;
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
        ui->output->appendPlainText(" - " + fpath);
    } else {
        failed_files.append(fpath);
    }
    progressOne();
}

void Uninstaller::dir_removed(QString fpath, bool ok)
{
    if ( ok ) {
        ui->output->appendPlainText(" -/ " + fpath);
    } else {
        failed_files.append(fpath);
    }
    progressOne();
}

void Uninstaller::progressOne()
{
    int p = ui->uninstallProgress->value();
    int max = ui->uninstallProgress->maximum();
    if (p == max) {
        //TODO: Do I still need the fatal error popup? What about a report in the output window?
        fatalError("BUG: progress > 100%");
        qApp->exit(2);
        // Also ensure it only happens once!
    } else {
        ui->uninstallProgress->setValue(p + 1);
    }
}

void Uninstaller::done()
{
    int fileCount = filesList.length() + linksList.length() - failed_files.length();
    int dirCount = dirsList.length() - failed_dirs.length();
    ui->output->appendPlainText("");
    ui->output->appendPlainText(tr("%1 files deleted").arg(fileCount));
    ui->output->appendPlainText(tr("%1 directories removed").arg(dirCount));

    QDir home_dir{QDir::home()};
    if (basedir.path() == home_dir.absoluteFilePath(".local")) {
        ui->output->appendPlainText("");
        ui->output->appendPlainText(tr("Run %1 and %2").arg("update-mime-database", "update-desktop-database"));
        // Update file-type associations
        QProcess::execute("update-mime-database",
                          QStringList() << basedir.absoluteFilePath("share/mime"));
        QProcess::execute("update-desktop-database",
                          QStringList() << basedir.absoluteFilePath("share/applications"));
    }

    //TODO: remove the root directory if empty?

    // Enable ok button
    ui->buttonBox_2->button(QDialogButtonBox::Ok)->setEnabled(true);
}
