#ifndef MYDIALOG_H
#define MYDIALOG_H

#include <QDialog>
#include <QDebug>
#include <QFile>
#include <QtnProperty/Core/Property.h>

#include "Filters3D.peg.h"
#include "utils/glslshaderparser.h"
#include "utils/glslparsedshadercontainer.h"

namespace Ui
{
class PropertyDialog;
}

class Dialog3DGeneralSettings : public QDialog
{
    Q_OBJECT

public:
    Dialog3DGeneralSettings(QWidget* parent);
    ~Dialog3DGeneralSettings();
    void closeEvent(QCloseEvent*);

    // Save current properties to file.
    void saveSettings();

    static QtnPropertySetFilters3D* filters3DProperties;
    static GLSLShaderParser* currentShaderParser;
    static GLSLParsedShaderContainer* parsedShaderContainer;

signals:
    void signalPropertyChanged();
    void signalRecompileCustomShader();

public slots:
    // Opens settings window.
    void show();
    // Restore last settings when window is cancelled.
    void cancelSettings();
    // Save current settings to file when OK button is pressed.
    void acceptSettings();
    void propertyChanged(const QtnPropertyBase *, const QtnPropertyBase *, QtnPropertyChangeReason reason);
    void recompileCustomShader();
    void shaderChanged(int index);
    static void updateParsedShaders();
    static void setUniforms();

private:

    Ui::PropertyDialog *ui;
    QtnPropertyDelegateInfo delegate;
    // Stores last settings before window was opened.
    QtnPropertySet *cpyPropertySet;
};

#endif // MYDIALOG_H
