#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QDir>

#include "opengl3dimagewidget.h"
#include "opengl2dimagewidget.h"
#include "postfixnames.h"
#include "properties/ImageProperties.peg.h"

#ifdef Q_OS_MAC
# define AB_INI "AwesomeBump.ini"
# define AB_LOG "AwesomeBump.log"
#else
# define AB_INI "config.ini"
# define AB_LOG "log.txt"
#endif

#ifdef USE_OPENGL_330
#define AWESOME_BUMP_VERSION "AwesomeBump " VERSION_STRING " (2016) (openGL 330 release)"
#else
#define AWESOME_BUMP_VERSION "AwesomeBump " VERSION_STRING " (2016)"
#endif

#define TAB_SETTINGS 9

class QAction;
class QLabel;
class ImageWidget;
class FormMaterialIndicesManager;
class FormSettingsContainer;
class DockWidget3DSettings;
class DialogLogger;
class DialogShortcuts;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    QSize sizeHint() const;
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
    void resizeEvent(QResizeEvent* event);
    void showEvent(QShowEvent* event);

signals:
    void initProgress(int perc);
    void initMessage(const QString &msg);

public slots:
    void aboutQt();
    void about();
    void initialiseWindow();
    void initializeGL();
    void initializeImages();
    void saveImages();
    void saveCheckedImages();
    void saveCompressedForm();
    void saveSettings();
    void changeGUIFontSize(int);
    // Load the application setting from ini file.
    void loadSettings();
    void loadImageSettings(TextureType type);
    void showSettingsManager();
    void setOutputFormat(int);
    void replotAllImages();
    void materialsToggled(bool toggle);
    void checkWarnings();
    // Repaint views after selecting tab.
    void selectDiffuseTab();
    void selectNormalTab();
    void selectSpecularTab();
    void selectHeightTab();
    void selectOcclusionTab();
    void selectRoughnessTab();
    void selectMetallicTab();
    void selectMaterialsTab();
    void selectGrungeTab();
    void selectGeneralSettingsTab();
    void selectUVsTab();
    // Resize 2D image.
    void fitImage();
    // Disable textures.
    void showHideTextureTypes(bool);
    // Repaint views after changes.
    void updateDiffuseImage();
    void updateNormalImage();
    void updateSpecularImage();
    void updateHeightImage();
    void updateOcclusionImage();
    void updateRoughnessImage();
    void updateMetallicImage();
    void updateGrungeImage();
    // Repaint selected tab.
    void updateImage(int tab);
    void updateImageInformation();
    // Change the combobox index
    void changeWidth(int);
    void changeHeight(int);
    void scaleWidth(double);
    void scaleHeight(double);
    void applyResizeImage();
    void applyResizeImage(int width, int height);
    void applyScaleImage();
    // Copy current image to diffuse and convert to others.
    void applyCurrentUVsTransformations();
    // Set the global parameters
    void updateSpinBoxes(int);
    void selectShadingModel(int i);
    // Conversion functions
    void convertFromHtoN();
    void convertFromNtoH();
    void convertFromBase();
    void convertFromHNtoOcc();
    // UV tools
    void updateSliders();
    // Perspective tool
    void resetTransform();
    void setUVManipulationMethod();
    void selectSeamlessMode(int mode);
    // In random mode
    void randomizeAngles();
    void resetRandomPatches();
    void selectContrastInputImage(int mode);
    // Batch tool
    void selectSourceImages();
    void selectOutputPath();
    void runBatch();

private:    
    // Save all textures to given directory.
    bool saveAllImages(const QString &dir);

    Ui::MainWindow *ui;
    OpenGL3DImageWidget *openGL3DImageWidget;
    OpenGL2DImageWidget *openGL2DImageWidget;
    
    bool bSaveCheckedImages;
    bool bSaveCompressedFormImages;

    QDir recentDir;
    // Path to last loaded OBJ Mesh folder
    QDir recentMeshDir;

    ImageWidget *diffuseImageWidget;
    ImageWidget *normalImageWidget;
    ImageWidget *specularImageWidget;
    ImageWidget *heightImageWidget;
    ImageWidget *occlusionImageWidget;
    ImageWidget *roughnessImageWidget;
    ImageWidget *metallicImageWidget;
    ImageWidget *grungeImageWidget;

    // Material manager.
    FormMaterialIndicesManager *materialManager;
    // Settings container.
    FormSettingsContainer *settingsContainer;
    // Use qtn to keep all settings in one place.
    QtnPropertySetAwesomeBump *abSettings;
    // 3D settings manager.
    DockWidget3DSettings *dock3Dsettings;

    QAction *aboutQtAction;
    QAction *aboutAction;
    // Show logger
    QAction *logAction;
    // Show key shortcuts
    QAction *shortcutsAction;

    QLabel *statusLabel;

    DialogLogger *dialogLogger;
    DialogShortcuts *dialogShortcuts;
    QSettings defaults;
};

void displayOpenGLInformation(bool includeExtensions);

// Redirect qDebug() to log file.
void customMessageHandler(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg);

// Find data directory for each platform.
QString getDataDirectory(const QString& resource);

#endif // MAINWINDOW_H
