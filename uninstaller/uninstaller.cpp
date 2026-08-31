#include "uninstaller.h"
#include "./ui_uninstaller.h"

Uninstaller::Uninstaller(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Uninstaller)
{
    ui->setupUi(this);
}

Uninstaller::~Uninstaller()
{
    delete ui;
}
