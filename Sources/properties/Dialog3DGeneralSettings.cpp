#include "Dialog3DGeneralSettings.h"
#include "ui_Dialog3DGeneralSettings.h"

#include <QMessageBox>

QtnPropertySetFilters3D *Dialog3DGeneralSettings::filters3DProperties = NULL;
GLSLShaderParser *Dialog3DGeneralSettings::currentShaderParser = NULL;
GLSLParsedShaderContainer *Dialog3DGeneralSettings::parsedShaderContainer = NULL;

Dialog3DGeneralSettings::Dialog3DGeneralSettings(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::PropertyDialog)
{
    ui->setupUi(this);
    hide();
    ui->widget->setParts(QtnPropertyWidgetPartsDescriptionPanel);

    QtnPropertySetFilters3D *settings = new QtnPropertySetFilters3D(this);

    filters3DProperties = settings;

    ui->widget->setPropertySet(settings);

    // Read settings from file
    QFile file("Configs/settings3D.dat");
    file.open(QIODevice::ReadOnly);
    if(file.isOpen())
    {
        QString property;
        QTextStream outstream(&file);
        property = outstream.readAll();
        filters3DProperties->fromStr(property);
    }

    connect(ui->pushButtonCancel,SIGNAL(pressed()),this,SLOT(cancelSettings()));
    connect(ui->pushButtonOK    ,SIGNAL(pressed()),this,SLOT(acceptSettings()));
    connect(ui->pushButtonRecompileShaders,SIGNAL(released()),this,SLOT(recompileCustomShader()));
    connect(settings,SIGNAL(propertyDidChange(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)),
            this,SLOT(propertyChanged(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)));

    // Parse and create list of avaiable shaders in Render folder.
    parsedShaderContainer = new GLSLParsedShaderContainer;

    // Create list of available shaders.
    QStringList shaderList;
    for(int i = 0 ; i < parsedShaderContainer->glslParsedShaders.size(); i++)
    {
        shaderList << parsedShaderContainer->glslParsedShaders.at(i)->shaderBaseName;
    }
    ui->comboBoxShadersList->addItems(shaderList);

    // Setup pointer and comboBox
    int lastIndex = settings->ParsedShader.LastShaderIndex.value();
    ui->comboBoxShadersList->setCurrentIndex(lastIndex);
    currentShaderParser  = parsedShaderContainer->glslParsedShaders[lastIndex];

    connect(ui->comboBoxShadersList,SIGNAL(currentIndexChanged(int)),this,SLOT(shaderChanged(int)));
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

void Dialog3DGeneralSettings::shaderChanged(int index)
{
    currentShaderParser  = parsedShaderContainer->glslParsedShaders[index];
    updateParsedShaders();
    emit signalPropertyChanged();
}

// Each time button ok is pressed the settings are saved
// to local file. This file is used during the application
// initialization to load last settings.
void Dialog3DGeneralSettings::closeEvent(QCloseEvent *)
{
    cancelSettings();
}

void Dialog3DGeneralSettings::show()
{
    QtnPropertySet* properties = ui->widget->propertySet();
    // copy of properties
    cpyPropertySet = properties->createCopy(this);
    Q_ASSERT(cpyPropertySet);
    this->showNormal();
}

void Dialog3DGeneralSettings::cancelSettings()
{
    this->reject();
    QtnPropertySet* properties = ui->widget->propertySet();
    properties->copyValues(cpyPropertySet, QtnPropertyStateInvisible);
}

void Dialog3DGeneralSettings::acceptSettings()
{
    this->accept();
    saveSettings();
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

void Dialog3DGeneralSettings::updateParsedShaders()
{
    GLSLShaderParser* parsedShader = currentShaderParser;
    int maxParams      = filters3DProperties->ParsedShader.MaxParams;
    int noParsedParams = parsedShader->uniforms.size();

    // If parsed number uniform is greater than supported number of params display warning message.
    if(noParsedParams > maxParams)
    {
        QMessageBox msgBox;
        msgBox.setText("Error!");
        msgBox.setInformativeText("Custom shader with name:"+parsedShader->shaderBaseName+
                                  " has more than maxiumum allowed number of user-defined uniforms. \n"+
                                  "Current number of parameters:"+QString::number(noParsedParams)+".\n"+
                                  "Supported number:"+QString::number(maxParams)+".\n"+
                                  "Custom shader has been linked but may not work properly.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    // Hide all by default.
    for(int i = 0 ; i < maxParams ; i++)
    {
        QtnPropertyFloat* p = (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i+1));
        p->switchState(QtnPropertyStateInvisible,true);
    }

    // Update property set based on parsed fragment shader.
    for(int i = 0 ; i < qMin(noParsedParams,maxParams) ; i++)
    {
        QtnPropertyFloat* p = (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i+1));
        UniformData& uniform = parsedShader->uniforms[i];
        p->switchState(QtnPropertyStateInvisible,false);
        p->setDescription(uniform.description);
        p->setDisplayName(uniform.name);
        p->setValue(uniform.value);
        p->setMaxValue(uniform.max);
        p->setMinValue(uniform.min);
        p->setStepValue(uniform.step);
    }
}

void Dialog3DGeneralSettings::setUniforms()
{
    GLSLShaderParser* parsedShader = currentShaderParser;
    int maxParams      = filters3DProperties->ParsedShader.MaxParams;
    int noParsedParams = parsedShader->uniforms.size();

    // Update property set based on parsed fragment shader.
    for(int i = 0 ; i < qMin(noParsedParams,maxParams) ; i++)
    {
        QtnPropertyFloat* p = (QtnPropertyFloat*)(filters3DProperties->ParsedShader.findChildProperty(i+1));
        UniformData& uniform = parsedShader->uniforms[i];
        uniform.value = (float)p->value();
    }
    parsedShader->setParsedUniforms();
}

Dialog3DGeneralSettings::~Dialog3DGeneralSettings()
{
    qDebug() << "calling" << Q_FUNC_INFO;
    filters3DProperties = NULL;
    delete parsedShaderContainer;
    parsedShaderContainer = NULL;
    delete ui;
}
