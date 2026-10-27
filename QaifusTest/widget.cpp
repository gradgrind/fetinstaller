#include "widget.h"
#include "ui_widget.h"
#include <QFile>
#include <QTextStream>
#include <QStringList>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/QaifusTest.svg"));

    QStringList args = QCoreApplication::arguments();
    if ( args.length() > 1 ) {
        QString msg;
        // Read from the file passed as argument.
        QString filepath{args.at(1)};
        QFile f{filepath};
        if( !f.open(QIODevice::ReadOnly) ) {
            msg = filepath + ":\n   " + f.errorString();
        } else {
            QTextStream in(&f);
            msg = in.readAll().replace("%f", filepath);
            f.close();
        }
        ui->plainTextEdit->appendPlainText("\n\n" + msg);
    }
}

Widget::~Widget()
{
    delete ui;
}
