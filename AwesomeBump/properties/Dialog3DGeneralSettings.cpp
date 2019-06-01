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
    connect(ui->pushButtonRecompileShaders, SIGNAL (released()), this, SLOT (recompileCustomShader()));
    connect(filters3DProperties,
            SIGNAL (propertyDidChange(const QtnPropertyBase*, const QtnPropertyBase*, QtnPropertyChangeReason)),
            this, SLOT (propertyChanged(const QtnPropertyBase*, const QtnPropertyBase*, QtnPropertyChangeReason)));

    // Create list of available shaders.
    QStringList shaderList;
    shaderList << "AwesomeBump";
    ui->comboBoxShadersList->addItems(shaderList);

    // Setup pointer and comboBox
    int lastIndex = filters3DProperties->ParsedShader.LastShaderIndex.value();
    ui->comboBoxShadersList->setCurrentIndex(lastIndex);

    connect(ui->comboBoxShadersList, SIGNAL (currentIndexChanged(int)), this, SLOT (shaderChanged(int)));
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
    filters3DProperties->ParsedShader.LastShaderIndex.setValue(ui->comboBoxShadersList->currentIndex());
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

void Dialog3DGeneralSettings::recompileCustomShader()
{
    emit signalRecompileCustomShader();
}

void Dialog3DGeneralSettings::shaderChanged(int)
{
    updateParsedShaders();
    emit signalPropertyChanged();
}

void Dialog3DGeneralSettings::updateParsedShaders()
{
    int maxParams = filters3DProperties->ParsedShader.MaxParams;

    // Hide all by default.
    for(int i = 0; i < maxParams; i++)
    {
        QtnPropertyFloat *floatProperty =
                (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i + 1));
        floatProperty->switchState(QtnPropertyStateInvisible, true);
    }
}
