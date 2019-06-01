#include "Dialog3DGeneralSettings.h"
#include "ui_Dialog3DGeneralSettings.h"

#include <QMessageBox>

Dialog3DGeneralSettings::Dialog3DGeneralSettings(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PropertyDialog)
{
    ui->setupUi(this);
    hide();
    ui->widget->setParts(QtnPropertyWidgetPartsDescriptionPanel);

    filters3DProperties =  new QtnPropertySetFilters3D(this);
    ui->widget->setPropertySet(filters3DProperties);

    // Read settings from file.
    QFile file("Configs/settings3D.dat");
    file.open(QIODevice::ReadOnly);
    if(file.isOpen())
    {
        QString property;
        QTextStream outstream(&file);
        property = outstream.readAll();
        filters3DProperties->fromStr(property);
    }

    connect(ui->pushButtonCancel, SIGNAL (pressed()), this, SLOT (cancelSettings()));
    connect(ui->pushButtonOK, SIGNAL (pressed()), this, SLOT (acceptSettings()));
    connect(filters3DProperties,
            SIGNAL (propertyDidChange(const QtnPropertyBase*, const QtnPropertyBase*, QtnPropertyChangeReason)),
            this, SLOT (propertyChanged(const QtnPropertyBase*, const QtnPropertyBase*, QtnPropertyChangeReason)));
}

Dialog3DGeneralSettings::~Dialog3DGeneralSettings()
{
    delete filters3DProperties;
    delete ui;
}

void Dialog3DGeneralSettings::closeEvent(QCloseEvent *)
{
    cancelSettings();
}

void Dialog3DGeneralSettings::saveSettings()
{
    QFile file("Configs/settings3D.dat");
    file.open(QIODevice::WriteOnly);
    QString property;
    filters3DProperties->toStr(property);
    QTextStream outstream(&file);
    outstream << property;
}

void Dialog3DGeneralSettings::show()
{
    QtnPropertySet *properties = ui->widget->propertySet();
    // Store copy of properties.
    propertySetCopy = properties->createCopy(this);
    Q_ASSERT(propertySetCopy);
    this->showNormal();
}

void Dialog3DGeneralSettings::cancelSettings()
{
    this->reject();
    QtnPropertySet *properties = ui->widget->propertySet();
    properties->copyValues(propertySetCopy, QtnPropertyStateInvisible);
}

void Dialog3DGeneralSettings::acceptSettings()
{
    this->accept();
    saveSettings();
}

void Dialog3DGeneralSettings::propertyChanged(const QtnPropertyBase *,
                                              const QtnPropertyBase *,
                                              QtnPropertyChangeReason reason)
{
    if (reason & QtnPropertyChangeReasonValue)
    {
        emit signalPropertyChanged();
    }
}
