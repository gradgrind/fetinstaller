#ifndef UNINSTALLER_H
#define UNINSTALLER_H

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Uninstaller;
}
QT_END_NAMESPACE

class Uninstaller : public QWidget
{
    Q_OBJECT

public:
    explicit Uninstaller(QWidget *parent = nullptr);
    ~Uninstaller() override;

private:
    Ui::Uninstaller *ui;
};
#endif // UNINSTALLER_H
