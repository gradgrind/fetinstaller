#include "uninstaller.h"
#include "./ui_uninstaller.h"

#include <QFile>
#include <QMessageBox>

static const QString INSTALLED_FILES{"share/fet/installed_files"};

static const char *FATAL_ERROR = QT_TRANSLATE_NOOP("Uninstaller", "Fatal Error");
static const char *WARNING = QT_TRANSLATE_NOOP("Uninstaller", "Warning");
static const char *ERROR1 = QT_TRANSLATE_NOOP("Uninstaller", R"(
%1 files not within installation base directory (lines starting with "!!!")
%2 files not found (lines starting with "***")

Continue, deleting the other %3 files?
)");


void fatalError(QString msg)
{
    QMessageBox::critical(
        nullptr,
        QCoreApplication::translate("Uninstaller", FATAL_ERROR),
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
    basedir.cdUp(); // base directory of installation

    ui->fetinstall_path->setText(basedir.path());

}

Uninstaller::~Uninstaller()
{
    delete ui;
}

void Uninstaller::page_2()
{
    ui->stackedWidget->setCurrentIndex(1);

    // Read the list of installed files
    QString filespath{basedir.absoluteFilePath(INSTALLED_FILES)};
    QFile textFile{filespath};
    if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fatalError(tr("Couldn't read file list at: ") + filespath);
        return;
    }
    QStringList filesList;
    QSet<QString> dirsSet; // collect directories
    QTextStream textStream(&textFile);
    int errorCount1{0}; // file not within installation directory
    int errorCount2{0}; // file not founs
    // For checking that files are within the installation directory:
    QString basepath{basedir.path() + "/"};
    while (true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;    // Report files not removed
        QString fpath{line.trimmed()};
        if (fpath.isEmpty()) {
            continue;
        } else if (fpath.startsWith(basepath)) {
            QFileInfo finfo{fpath};
            if (!finfo.exists()) {
                errorCount2++;
                ui->output->appendPlainText("*** " + fpath);
                continue;
            }
            filesList.append(fpath);
            dirsSet.insert(finfo.dir().absolutePath());
        } else {
            // File not within installation directory
            errorCount1++;
            ui->output->appendPlainText("!!! " + fpath);
        }
    }
    if (errorCount1 != 0 || errorCount2 != 0) {
        if (QMessageBox::warning(
                this,
                tr(WARNING),
                tr(ERROR1).arg(errorCount1).arg(errorCount2).arg(filesList.length()),
                QMessageBox::Ok|QMessageBox::Cancel) != QMessageBox::Ok) {

            return;
        }
    }

    // Add uninstaller files
    filesList.append(basedir.absoluteFilePath("bin/fet_uninstall"));
    filesList.append(basedir.absoluteFilePath(INSTALLED_FILES));

    // Add directories within the installation directory which haven't yet been added
    // because they contain only directories
    for (const auto& d : dirsSet) {
        QFileInfo dinfo{d};
        while (true) {
            QString p{dinfo.path()};
            if (p.startsWith(basepath)) {
                dirsSet.insert(p);
                dinfo = QFileInfo{p};
            } else {
                break;
            }
        }
    }

//TODO: background thread for deletions

    // Delete the files in filesList
    int fileCount{0};
    QStringList errorFiles;

    for (const auto& fpath : filesList) {
        if (QFile::remove(fpath)) {
            fileCount++;
            ui->output->appendPlainText(" - " + fpath);
        } else {
            errorFiles.append(fpath);
        }
    }

    // Report files not removed
    for (const auto& fpath : errorFiles) {
        ui->output->appendPlainText("??? " + fpath);
    }
    if (!errorFiles.isEmpty()) {
        QMessageBox::warning(
            this,
            tr(WARNING),
            tr("%1 files could not be deleted (lines starting with \"???\")").arg(errorFiles.length()));
    }

    // Remove empty directories
    QStringList dirsList{dirsSet.values()};
    dirsList.sort();
    int dirCount{0};
    for (auto it = dirsList.rbegin(); it != dirsList.rend(); ++it) {
        // Use reverse iteration to get the deepest directories first
        if (basedir.rmdir(*it)) {
            dirCount++;
            ui->output->appendPlainText(" --- " + *it + '/');
        }
    }
    if (basedir.rmdir(basedir.path())) {
        dirCount++;
        ui->output->appendPlainText(" --- " + basedir.path());
    }

    ui->output->appendPlainText("");
    ui->output->appendPlainText(tr("%1 files deleted").arg(fileCount));
    ui->output->appendPlainText(tr("%1 directories removed").arg(dirCount));
}