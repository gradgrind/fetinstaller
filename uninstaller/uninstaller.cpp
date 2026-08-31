#include "uninstaller.h"
#include "./ui_uninstaller.h"

#include <QFile>
#include <QMessageBox>

static const char *FATAL_ERROR = QT_TRANSLATE_NOOP("Uninstaller", "Fatal Error");
static const char *WARNING = QT_TRANSLATE_NOOP("Uninstaller", "Warning");
static const char *ERROR1 = QT_TRANSLATE_NOOP("Uninstaller", R"(
%1 files not found (lines starting with "***")
%2 files not within installation base directory (lines starting with "!!!")
)");

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
    QString filespath{basedir.absoluteFilePath("share/fet/installed_files")};

    QStringList filesList;
    QFile textFile{filespath};
    if (!textFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr(FATAL_ERROR), tr("Couldn't read file list at: ") + filespath);
        return;
    }
    QTextStream textStream(&textFile);
    int errorCount1{0};
    int errorCount2{0};
    QString basepath{basedir.canonicalPath() + "/"};
    while (true)
    {
        QString line = textStream.readLine();
        if (line.isNull())
            break;

        QFileInfo finfo{line};
        QString fpath{finfo.canonicalFilePath()};
        if (fpath.isEmpty()) {
            errorCount1++;
            ui->output->appendPlainText("*** " + line);
        } else if (fpath.startsWith(basepath)) {
            //ui->output->appendPlainText(" - " + fpath);
            filesList.append(fpath);
        } else {
            errorCount2++;
            ui->output->appendPlainText("!!! " + line);
        }
    }
    if (errorCount1 != 0 || errorCount2 != 0) {
        if (QMessageBox::warning(this, tr(WARNING), tr(ERROR1).arg(errorCount1).arg(errorCount2),
            QMessageBox::Ok|QMessageBox::Cancel) == QMessageBox::Ok) {


            //TODO
            qDebug() << "Continue";
        }
    }
}