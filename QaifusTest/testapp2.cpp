#include "testapp2.h"
#include "ui_testapp2.h"

TestApp2::TestApp2(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TestApp2)
{
    ui->setupUi(this);
}

TestApp2::~TestApp2()
{
    delete ui;
}
