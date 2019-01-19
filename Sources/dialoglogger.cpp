#include "dialoglogger.h"
#include "ui_dialoglogger.h"

#include <QDebug>
#include <QScrollBar>

DialogLogger::DialogLogger(QWidget *parent, const QString& filename) :
    QDialog(parent),
    logFile(filename),
    ui(new Ui::DialogLogger)
{
    ui->setupUi(this);
}

DialogLogger::~DialogLogger()
{
    delete ui;
}

void DialogLogger::showLog(){
    qDebug() << "Show logger.";
    ui->textEdit->clear();
    if(logFile.exists())
    {
        logFile.open(QFile::ReadOnly | QFile::Text);
        QTextStream in(&logFile);
        ui->textEdit->setText(in.readAll());
        ui->textEdit->verticalScrollBar()->setValue(ui->textEdit->verticalScrollBar()->maximum());
    }
    else
    {
        ui->textEdit->setText(logFile.fileName() + QString(" does not exist."));
    }
    show();
}
