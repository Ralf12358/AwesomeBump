// Keeps the settings for 3D visualization.

#ifndef DOCKWIDGET3DSETTINGS_H
#define DOCKWIDGET3DSETTINGS_H

#include <QDockWidget>
#include "openglwidget.h"
#include "properties/ImageProperties.peg.h"

namespace Ui
{
class DockWidget3DSettings;
}

class DockWidget3DSettings : public QDockWidget
{
    Q_OBJECT

public:
    explicit DockWidget3DSettings(QWidget *parent, OpenGLWidget *ptr_glWidget);
    ~DockWidget3DSettings();

    Display3DSettings settings;

signals:
    // Change Tab name in MainWindow.
    void signalSelectedShadingModel(int i);
    void signalSettingsChanged(Display3DSettings settings);

public slots:
    void updateSettings(int i = 0);
    // Sends signal to change the names of the tabs in MainWindow.
    void selectShadingModel(int i);
    // Save currents states to config file.
    void saveSettings(QtnPropertySetAwesomeBump *settings);
    // Load currents states from config file
    void loadSettings(QtnPropertySetAwesomeBump *settings);

private:
    OpenGLWidget *ptr_glWidget;
    Ui::DockWidget3DSettings *ui;
    QSize sizeHint() const;
};

#endif // DOCKWIDGET3DSETTINGS_H
