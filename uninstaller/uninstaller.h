#ifndef UNINSTALLER_H
#define UNINSTALLER_H

#include <QWidget>
#include <QDir>
#include <QStringList>
#include <QSet>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui {
class Uninstaller;
}
QT_END_NAMESPACE

class Uninstaller : public QWidget
{
    Q_OBJECT
    QThread workerThread;

public:
    explicit Uninstaller(QWidget *parent = nullptr);
    ~Uninstaller() override;

private slots:
    void threadedWarning(QString msg);
    void progressOne();
    void done();

private:
    Ui::Uninstaller *ui;
    QDir basedir;
    void page_2();
    void warning(QString msg);
    void fatalError(QString msg);

signals:
    void deleteFiles(QDir basedir, QStringList filesList, QSet<QString> dirsSet);
};

extern void fatalError(QString msg);

#endif // UNINSTALLER_H
