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
    currentShaderParser = new GLSLShaderParser;

    connect(ui->comboBoxShadersList, SIGNAL (currentIndexChanged(int)), this, SLOT (shaderChanged(int)));
}

Dialog3DGeneralSettings::~Dialog3DGeneralSettings()
{
    delete filters3DProperties;
    delete currentShaderParser;
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
    GLSLShaderParser *parsedShader = currentShaderParser;
    int maxParams = filters3DProperties->ParsedShader.MaxParams;
    int parsedParamsCount = parsedShader->uniforms.size();

    // If parsed number uniform is greater than supported number of params display warning message.
    if(parsedParamsCount > maxParams)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Custom shader with name:" + parsedShader->shaderBaseName +
                                  " has more than maxiumum allowed number of user-defined uniforms.\n" +
                                  "Current number of parameters: " + QString::number(parsedParamsCount) + ".\n" +
                                  "Supported number of parameters: " + QString::number(maxParams) + ".\n" +
                                  "Custom shader has been linked but may not work properly.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    // Hide all by default.
    for(int i = 0; i < maxParams; i++)
    {
        QtnPropertyFloat *floatProperty =
                (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i + 1));
        floatProperty->switchState(QtnPropertyStateInvisible, true);
    }

    // Update property set based on parsed fragment shader.
    for(int i = 0; i < qMin(parsedParamsCount, maxParams); i++)
    {
        QtnPropertyFloat *floatProperty =
                (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i + 1));
        UniformData& uniform = parsedShader->uniforms[i];
        floatProperty->switchState(QtnPropertyStateInvisible, false);
        floatProperty->setDescription(uniform.description);
        floatProperty->setDisplayName(uniform.name);
        floatProperty->setValue(uniform.value);
        floatProperty->setMaxValue(uniform.max);
        floatProperty->setMinValue(uniform.min);
        floatProperty->setStepValue(uniform.step);
    }
}

void Dialog3DGeneralSettings::setUniforms()
{
    GLSLShaderParser *parsedShader = currentShaderParser;
    int maxParams = filters3DProperties->ParsedShader.MaxParams;
    int parsedParamsCount = parsedShader->uniforms.size();

    // Update property set based on parsed fragment shader.
    for(int i = 0; i < qMin(parsedParamsCount, maxParams) ; i++)
    {
        QtnPropertyFloat* floatProperty = (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i+1));
        UniformData& uniform = parsedShader->uniforms[i];
        uniform.value = (float)floatProperty->value();
    }
    parsedShader->setParsedUniforms();
}
