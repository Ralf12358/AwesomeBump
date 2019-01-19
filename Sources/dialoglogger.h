#ifndef DIALOGLOGGER_H
#define DIALOGLOGGER_H

#include <QDialog>
#include <QFile>

namespace Ui
{
class DialogLogger;
}

class DialogLogger : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLogger(QWidget *parent, const QString& filename);
    ~DialogLogger();

public slots:
    void showLog();

private:
    QFile logFile;
    Ui::DialogLogger *ui;
};

#endif // DIALOGLOGGER_H
