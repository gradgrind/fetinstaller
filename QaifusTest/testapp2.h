#ifndef TESTAPP2_H
#define TESTAPP2_H

#include <QWidget>

namespace Ui {
class TestApp2;
}

class TestApp2 : public QWidget
{
    Q_OBJECT

public:
    explicit TestApp2(QWidget *parent = nullptr);
    ~TestApp2();

private:
    Ui::TestApp2 *ui;
};

#endif // TESTAPP2_H
