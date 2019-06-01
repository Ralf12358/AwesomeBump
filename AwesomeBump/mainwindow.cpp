#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QLabel>
#include <QtDebug>
#include <QDateTime>
#include <QMessageBox>

#include <Property.h>
#include <PropertySet.h>

#include <iostream>

#include "opengl3dimagewidget.h"
#include "opengl2dimagewidget.h"
#include "image.h"
#include "imagewidget.h"
#include "formmaterialindicesmanager.h"
#include "formsettingscontainer.h"
#include "dockwidget3dsettings.h"
#include "properties/Dialog3DGeneralSettings.h"
#include "dialoglogger.h"
#include "dialogshortcuts.h"

#define INIT_PROGRESS(p,m) \
    emit initProgress(p); \
    emit initMessage(m);

// Compressed texture type.
enum CompressedFromTypes
{
    H_TO_D_AND_S_TO_N = 0,
    S_TO_D_AND_H_TO_N = 1
};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    bSaveCheckedImages(false),
    bSaveCompressedFormImages(false)
{
    ui->setupUi(this);

    abSettings = new QtnPropertySetAwesomeBump(this);
    statusLabel = new QLabel("GPU memory status: n/a");

    ImageWidget::recentDir = &recentDir;

#ifdef Q_OS_MAC
    if(!statusLabel->testAttribute(Qt::WA_MacNormalSize))
        statusLabel->setAttribute(Qt::WA_MacSmallSize);
#endif

    openGL2DImageWidget = new OpenGL2DImageWidget(this);
    //glWidget = new OpenGLWidget(this,glImage);
    openGL3DImageWidget = new OpenGL3DImageWidget(this);
}

void MainWindow::initialiseWindow()
{
    //qInstallMessageHandler(customMessageHandler);

    qDebug() << "Starting application:";
    qDebug() << "Application dir:" << QApplication::applicationDirPath();

    qDebug() << "Initialization: Build image properties";
    INIT_PROGRESS(10, "Build image properties");

    diffuseImageWidget   = new ImageWidget(this, openGL2DImageWidget, DIFFUSE_TEXTURE);
    normalImageWidget    = new ImageWidget(this, openGL2DImageWidget, NORMAL_TEXTURE);
    specularImageWidget  = new ImageWidget(this, openGL2DImageWidget, SPECULAR_TEXTURE);
    heightImageWidget    = new ImageWidget(this, openGL2DImageWidget, HEIGHT_TEXTURE);
    occlusionImageWidget = new ImageWidget(this, openGL2DImageWidget, OCCLUSION_TEXTURE);
    roughnessImageWidget = new ImageWidget(this, openGL2DImageWidget, ROUGHNESS_TEXTURE);
    metallicImageWidget  = new ImageWidget(this, openGL2DImageWidget, METALLIC_TEXTURE);
    grungeImageWidget    = new ImageWidget(this, openGL2DImageWidget, GRUNGE_TEXTURE);

    materialManager    = new FormMaterialIndicesManager(this, openGL2DImageWidget);

    qDebug() << "Initialization: Setup image properties";
    INIT_PROGRESS(20, "Setup image properties");

    // Set pointers to images
    materialManager->imagesPointers[0] = diffuseImageWidget;
    materialManager->imagesPointers[1] = normalImageWidget;
    materialManager->imagesPointers[2] = specularImageWidget;
    materialManager->imagesPointers[3] = heightImageWidget;
    materialManager->imagesPointers[4] = occlusionImageWidget;
    materialManager->imagesPointers[5] = roughnessImageWidget;
    materialManager->imagesPointers[6] = metallicImageWidget;

    // Set pointers to 3D view (used to bindTextures).
    openGL3DImageWidget->setImage(diffuseImageWidget  ->getImage(), DIFFUSE_TEXTURE);
    openGL3DImageWidget->setImage(normalImageWidget   ->getImage(), NORMAL_TEXTURE);
    openGL3DImageWidget->setImage(specularImageWidget ->getImage(), SPECULAR_TEXTURE);
    openGL3DImageWidget->setImage(heightImageWidget   ->getImage(), HEIGHT_TEXTURE);
    openGL3DImageWidget->setImage(occlusionImageWidget->getImage(), OCCLUSION_TEXTURE);
    openGL3DImageWidget->setImage(roughnessImageWidget->getImage(), ROUGHNESS_TEXTURE);
    openGL3DImageWidget->setImage(metallicImageWidget ->getImage(), METALLIC_TEXTURE);
    openGL3DImageWidget->setImage(materialManager     ->getImage(), MATERIAL_TEXTURE);

    openGL2DImageWidget->targetImageDiffuse   = diffuseImageWidget  ->getImage();
    openGL2DImageWidget->targetImageNormal    = normalImageWidget   ->getImage();
    openGL2DImageWidget->targetImageSpecular  = specularImageWidget ->getImage();
    openGL2DImageWidget->targetImageHeight    = heightImageWidget   ->getImage();
    openGL2DImageWidget->targetImageOcclusion = occlusionImageWidget->getImage();
    openGL2DImageWidget->targetImageRoughness = roughnessImageWidget->getImage();
    openGL2DImageWidget->targetImageMetallic  = metallicImageWidget ->getImage();
    openGL2DImageWidget->targetImageGrunge    = grungeImageWidget   ->getImage();
    openGL2DImageWidget->targetImageMaterial  = materialManager     ->getImage();

    // Setup GUI
    qDebug() << "Initialization: GUI setup";
    INIT_PROGRESS(30, "GUI setup");

    ui->statusbar->addWidget(statusLabel);

    // Settings container
    settingsContainer = new FormSettingsContainer;
    ui->verticalLayout2DImage->addWidget(settingsContainer);
    settingsContainer->hide();
    connect(settingsContainer, SIGNAL (reloadConfigFile()), this, SLOT (loadSettings()));
    connect(settingsContainer, SIGNAL (emitLoadAndConvert()), this, SLOT (convertFromBase()));
    connect(settingsContainer, SIGNAL (forceSaveCurrentConfig()), this, SLOT (saveSettings()));
    connect(ui->pushButtonProjectManager, SIGNAL (toggled(bool)), settingsContainer, SLOT (setVisible(bool)));

    // 3D settings widget
    dock3Dsettings = new DockWidget3DSettings(this, openGL3DImageWidget);

    ui->verticalLayout3DImage->addWidget(dock3Dsettings);
    setDockNestingEnabled(true);
    connect(dock3Dsettings, SIGNAL (signalSelectedShadingModel(int)), this, SLOT (selectShadingModel(int)));
    // show hide 3D settings
    connect(ui->pushButton3DSettings, SIGNAL (toggled(bool)), dock3Dsettings, SLOT (setVisible(bool)));

    //dialog3dGeneralSettings = new Dialog3DGeneralSettings(this);
    //connect(ui->pushButton3DGeneralSettings, SIGNAL (released()), dialog3dGeneralSettings, SLOT (show()));
    connect(ui->pushButton3DGeneralSettings, SIGNAL (released()), openGL3DImageWidget, SLOT (show3DGeneralSettingsDialog()));
    //connect(dialog3dGeneralSettings, SIGNAL (signalPropertyChanged()), openGL3DImageWidget, SLOT (repaint()));
    //connect(dialog3dGeneralSettings, SIGNAL (signalRecompileCustomShader()), openGL3DImageWidget, SLOT (recompileRenderShader()));

    ui->verticalLayout3DImage->addWidget(openGL3DImageWidget);
    ui->verticalLayout2DImage->addWidget(openGL2DImageWidget);

    qDebug() << "Initialization: Adding widgets.";
    INIT_PROGRESS(40, "Adding widgets.");

    ui->verticalLayoutDiffuseImage        ->addWidget(diffuseImageWidget);
    ui->verticalLayoutNormalImage         ->addWidget(normalImageWidget);
    ui->verticalLayoutSpecularImage       ->addWidget(specularImageWidget);
    ui->verticalLayoutHeightImage         ->addWidget(heightImageWidget);
    ui->verticalLayoutOcclusionImage      ->addWidget(occlusionImageWidget);
    ui->verticalLayoutRoughnessImage      ->addWidget(roughnessImageWidget);
    ui->verticalLayoutMetallicImage       ->addWidget(metallicImageWidget);
    ui->verticalLayoutGrungeImage         ->addWidget(grungeImageWidget);
    ui->verticalLayoutMaterialIndicesImage->addWidget(materialManager);

    ui->tabWidget->setCurrentIndex(TAB_SETTINGS);
    
    connect(ui->tabWidget, SIGNAL (currentChanged(int)), this, SLOT (updateImage(int)));
    connect(ui->tabWidget, SIGNAL (tabBarClicked(int)), this, SLOT (updateImage(int)));

    connect(diffuseImageWidget,   SIGNAL (imageChanged()), this, SLOT (checkWarnings()));
    connect(occlusionImageWidget, SIGNAL (imageChanged()), this, SLOT (checkWarnings()));
    connect(grungeImageWidget,    SIGNAL (imageChanged()), this, SLOT (checkWarnings()));

    connect(diffuseImageWidget,   SIGNAL(imageChanged()), openGL2DImageWidget, SLOT (imageChanged()));
    connect(roughnessImageWidget, SIGNAL(imageChanged()), openGL2DImageWidget, SLOT (imageChanged()));
    connect(metallicImageWidget,  SIGNAL(imageChanged()), openGL2DImageWidget, SLOT (imageChanged()));

    connect(diffuseImageWidget,   SIGNAL (imageChanged()), this, SLOT (updateDiffuseImage()));
    connect(normalImageWidget,    SIGNAL (imageChanged()), this, SLOT (updateNormalImage()));
    connect(specularImageWidget,  SIGNAL (imageChanged()), this, SLOT (updateSpecularImage()));
    connect(heightImageWidget,    SIGNAL (imageChanged()), this, SLOT (updateHeightImage()));
    connect(occlusionImageWidget, SIGNAL (imageChanged()), this, SLOT (updateOcclusionImage()));
    connect(roughnessImageWidget, SIGNAL (imageChanged()), this, SLOT (updateRoughnessImage()));
    connect(metallicImageWidget,  SIGNAL (imageChanged()), this, SLOT (updateMetallicImage()));
    connect(grungeImageWidget,    SIGNAL (imageChanged()), this, SLOT (updateGrungeImage()));

    qDebug() << "Initialization: Connections and actions.";
    INIT_PROGRESS(50, "Connections and actions.");

    // Connect grunge slots.
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), diffuseImageWidget  , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), normalImageWidget   , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), specularImageWidget , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), heightImageWidget   , SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), occlusionImageWidget, SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), roughnessImageWidget, SLOT (toggleGrungeImageSettingsGroup(bool)));
    connect(grungeImageWidget,    SIGNAL(toggleGrungeSettings(bool)), metallicImageWidget , SLOT (toggleGrungeImageSettingsGroup(bool)));

    // Connect material manager slots.
    connect(materialManager, SIGNAL (materialChanged()), this, SLOT (replotAllImages()));
    connect(materialManager, SIGNAL (materialsToggled(bool)), ui->tabTilling, SLOT (setDisabled(bool)));
    // Disable conversion tool
    connect(materialManager, SIGNAL (materialsToggled(bool)), this, SLOT (materialsToggled(bool)));
    connect(openGL3DImageWidget, SIGNAL (materialColorPicked(QColor)), materialManager, SLOT (chooseMaterialByColor(QColor)));

    connect(diffuseImageWidget,  SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(normalImageWidget,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(specularImageWidget, SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(heightImageWidget,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(occlusionImageWidget,SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(roughnessImageWidget,SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(metallicImageWidget, SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    //connect(grungeImageProp,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));
    connect(materialManager,   SIGNAL (imageLoaded(int,int)), this, SLOT (applyResizeImage(int,int)));

    // Connect image reload settings signal.
    connect(diffuseImageWidget,   SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(normalImageWidget,    SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(specularImageWidget,  SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(heightImageWidget,    SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(occlusionImageWidget, SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(roughnessImageWidget, SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(metallicImageWidget,  SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));
    connect(grungeImageWidget,    SIGNAL (reloadSettingsFromConfigFile(TextureType)), this, SLOT (loadImageSettings(TextureType)));

    // Connect conversion signals.
    connect(diffuseImageWidget,   SIGNAL (conversionBaseConversionApplied()), this, SLOT (convertFromBase()));
    connect(normalImageWidget,    SIGNAL (conversionHeightToNormalApplied()), this, SLOT (convertFromHtoN()));
    connect(heightImageWidget,    SIGNAL (conversionNormalToHeightApplied()), this, SLOT (convertFromNtoH()));
    connect(occlusionImageWidget, SIGNAL (conversionHeightNormalToOcclusionApplied()), this, SLOT (convertFromHNtoOcc()));

    // Save signals.
    connect(ui->pushButtonSaveAll, SIGNAL (released()), this, SLOT (saveImages()));
    connect(ui->pushButtonSaveChecked, SIGNAL (released()), this, SLOT (saveCheckedImages()));
    connect(ui->pushButtonSaveAs, SIGNAL (released()), this, SLOT (saveCompressedForm()));

    // Image properties signals.
    connect(ui->comboBoxResizeWidth,  SIGNAL(currentIndexChanged(int)), this, SLOT (changeWidth(int)));
    connect(ui->comboBoxResizeHeight, SIGNAL(currentIndexChanged(int)), this, SLOT (changeHeight(int)));

    connect(ui->doubleSpinBoxRescaleWidth,  SIGNAL(valueChanged(double)), this, SLOT (scaleWidth(double)));
    connect(ui->doubleSpinBoxRescaleHeight, SIGNAL(valueChanged(double)), this, SLOT (scaleHeight(double)));

    connect(ui->pushButtonResizeApply,         SIGNAL(released()), this, SLOT (applyResizeImage()));
    connect(ui->pushButtonRescaleApply,        SIGNAL(released()), this, SLOT (applyScaleImage()));
    connect(ui->pushButtonReplotAll,           SIGNAL(released()), this, SLOT(replotAllImages()));
    connect(ui->pushButtonResetCameraPosition, SIGNAL(released()), openGL3DImageWidget, SLOT (resetCameraPosition()));
    connect(ui->pushButtonChangeCamPosition,   SIGNAL(toggled(bool)), openGL3DImageWidget,SLOT (toggleChangeCamPosition(bool)));

    connect(openGL3DImageWidget, SIGNAL (changeCamPositionApplied(bool)), ui->pushButtonChangeCamPosition, SLOT (setChecked(bool)));

    connect(ui->pushButtonToggleDiffuse,   SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleDiffuseView(bool)));
    connect(ui->pushButtonToggleNormal,    SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleNormalView(bool)));
    connect(ui->pushButtonToggleSpecular,  SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleSpecularView(bool)));
    connect(ui->pushButtonToggleHeight,    SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleHeightView(bool)));
    connect(ui->pushButtonToggleOcclusion, SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleOcclusionView(bool)));
    connect(ui->pushButtonToggleRoughness, SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleRoughnessView(bool)));
    connect(ui->pushButtonToggleMetallic,  SIGNAL(toggled(bool)), openGL3DImageWidget, SLOT (toggleMetallicView(bool)));

    connect(ui->pushButtonSaveCurrentSettings, SIGNAL(released()), this, SLOT(saveSettings()));
    connect(ui->comboBoxImageOutputFormat, SIGNAL(activated(int)), this, SLOT(setOutputFormat(int)));

    ui->progressBar->setValue(0);

    connect(ui->actionReplot,              SIGNAL(triggered()), this, SLOT (replotAllImages()));
    connect(ui->actionShowDiffuseImage,    SIGNAL(triggered()), this, SLOT (selectDiffuseTab()));
    connect(ui->actionShowNormalImage,     SIGNAL(triggered()), this, SLOT (selectNormalTab()));
    connect(ui->actionShowSpecularImage,   SIGNAL(triggered()), this, SLOT (selectSpecularTab()));
    connect(ui->actionShowHeightImage,     SIGNAL(triggered()), this, SLOT (selectHeightTab()));
    connect(ui->actionShowOcclusiontImage, SIGNAL(triggered()), this, SLOT (selectOcclusionTab()));
    connect(ui->actionShowRoughnessImage,  SIGNAL(triggered()), this, SLOT (selectRoughnessTab()));
    connect(ui->actionShowMetallicImage,   SIGNAL(triggered()), this, SLOT (selectMetallicTab()));
    connect(ui->actionShowGrungeTexture,   SIGNAL(triggered()), this, SLOT (selectGrungeTab()));
    connect(ui->actionShowMaterialsImage,  SIGNAL(triggered()), this, SLOT (selectMaterialsTab()));

    connect(ui->checkBoxSaveDiffuse,   SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveNormal,    SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveSpecular,  SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveHeight,    SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveOcclusion, SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveRoughness, SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));
    connect(ui->checkBoxSaveMetallic,  SIGNAL(toggled(bool)), this, SLOT (showHideTextureTypes(bool)));

    connect(ui->actionShowSettingsImage, SIGNAL (triggered()), this, SLOT (selectGeneralSettingsTab()));
    connect(ui->actionShowUVsTab, SIGNAL (triggered()), this, SLOT (selectUVsTab()));
    connect(ui->actionFitToScreen, SIGNAL (triggered()), this, SLOT (fitImage()));

    qDebug() << "Initialization: Perspective tool connections.";
    INIT_PROGRESS(60, "Perspective tool connections.");

    // Connect perspective tool signals.
    connect(ui->pushButtonResetTransform, SIGNAL (released()), this, SLOT (resetTransform()));
    connect(ui->comboBoxPerspectiveTransformMethod, SIGNAL (activated(int)), openGL2DImageWidget, SLOT (selectPerspectiveTransformMethod(int)));
    connect(ui->comboBoxSeamlessMode, SIGNAL (activated(int)), this, SLOT (selectSeamlessMode(int)));
    connect(ui->comboBoxSeamlessContrastInputImage, SIGNAL (activated(int)), this, SLOT (selectContrastInputImage(int)));

    qDebug() << "Initialization: UV seamless connections.";
    INIT_PROGRESS(70, "UV seamless connections.");

    // Connect uv seamless algorithms.
    connect(ui->checkBoxUVTranslationsFirst, SIGNAL (clicked()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderMakeSeamlessRadius, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderMakeSeamlessRadius, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderSeamlessContrastStrenght, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderSeamlessContrastStrenght, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderSeamlessContrastPower, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderSeamlessContrastPower, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));

    QButtonGroup *groupSimpleDirectionMode = new QButtonGroup( this );
    groupSimpleDirectionMode->addButton( ui->radioButtonSeamlessSimpleDirXY);
    groupSimpleDirectionMode->addButton( ui->radioButtonSeamlessSimpleDirX);
    groupSimpleDirectionMode->addButton( ui->radioButtonSeamlessSimpleDirY);
    connect(ui->radioButtonSeamlessSimpleDirXY, SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonSeamlessSimpleDirX,  SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonSeamlessSimpleDirY,  SIGNAL (released()), this, SLOT (updateSliders()));

    QButtonGroup *groupMirroMode = new QButtonGroup( this );
    groupMirroMode->addButton( ui->radioButtonMirrorModeX);
    groupMirroMode->addButton( ui->radioButtonMirrorModeY);
    groupMirroMode->addButton( ui->radioButtonMirrorModeXY);
    connect(ui->radioButtonMirrorModeX,  SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonMirrorModeY,  SIGNAL (released()), this, SLOT (updateSliders()));
    connect(ui->radioButtonMirrorModeXY, SIGNAL (released()), this, SLOT (updateSliders()));

    // Connect random mode signals.
    connect(ui->pushButtonRandomPatchesRandomize, SIGNAL (released()), this, SLOT (randomizeAngles()));
    connect(ui->pushButtonRandomPatchesReset, SIGNAL (released()), this, SLOT (resetRandomPatches()));
    connect(ui->horizontalSliderRandomPatchesRotate,      SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderRandomPatchesInnerRadius, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderRandomPatchesOuterRadius, SIGNAL (sliderReleased()), this, SLOT (updateSliders()));
    connect(ui->horizontalSliderRandomPatchesRotate,      SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderRandomPatchesInnerRadius, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));
    connect(ui->horizontalSliderRandomPatchesOuterRadius, SIGNAL (valueChanged(int)), this, SLOT (updateSpinBoxes(int)));

    // Apply UVs tranformations
    connect(ui->pushButtonApplyUVtransformations, SIGNAL (released()), this, SLOT (applyCurrentUVsTransformations()));
    ui->groupBoxSimpleSeamlessMode->hide();
    ui->groupBoxMirrorMode->hide();
    ui->groupBoxRandomPatchesMode->hide();

    // Color picking signals.
    connect(diffuseImageWidget,   SIGNAL (pickImageColor(QtnPropertyABColor*)), openGL2DImageWidget, SLOT (pickImageColor( QtnPropertyABColor*)));
    connect(roughnessImageWidget, SIGNAL (pickImageColor(QtnPropertyABColor*)), openGL2DImageWidget, SLOT (pickImageColor( QtnPropertyABColor*)));
    connect(metallicImageWidget,  SIGNAL (pickImageColor(QtnPropertyABColor*)), openGL2DImageWidget, SLOT (pickImageColor( QtnPropertyABColor*)));

    // 2D imate tool box settings.
    QActionGroup *group = new QActionGroup( this );
    group->addAction(ui->actionTranslateUV);
    group->addAction(ui->actionGrabCorners);
    group->addAction(ui->actionScaleXY);
    ui->actionTranslateUV->setChecked(true);
    connect(ui->actionTranslateUV, SIGNAL (triggered()), this, SLOT (setUVManipulationMethod()));
    connect(ui->actionGrabCorners, SIGNAL (triggered()), this, SLOT (setUVManipulationMethod()));
    connect(ui->actionScaleXY,     SIGNAL (triggered()), this, SLOT (setUVManipulationMethod()));

    // Other settings.
    connect(ui->spinBoxMouseSensitivity, SIGNAL (valueChanged(int)), openGL3DImageWidget, SLOT (setCameraMouseSensitivity(int)));
    connect(ui->spinBoxFontSize, SIGNAL (valueChanged(int)), this, SLOT (changeGUIFontSize(int)));
    connect(ui->checkBoxToggleMouseLoop, SIGNAL (toggled(bool)), openGL3DImageWidget, SLOT (toggleMouseWrap(bool)));
    connect(ui->checkBoxToggleMouseLoop, SIGNAL (toggled(bool)), openGL2DImageWidget, SLOT (toggleMouseWrap(bool)));

    // Batch settings
    connect(ui->pushButtonImageBatchSource, SIGNAL (pressed()), this, SLOT (selectSourceImages()));
    connect(ui->pushButtonImageBatchOutput, SIGNAL (pressed()), this, SLOT (selectOutputPath()));
    connect(ui->pushButtonImageBatchRun, SIGNAL (pressed()), this, SLOT (runBatch()));

#ifdef Q_OS_MAC
    if(ui->statusbar && !ui->statusbar->testAttribute(Qt::WA_MacNormalSize))
        ui->statusbar->setAttribute(Qt::WA_MacSmallSize);
#endif

    // Checking for GUI styles
    QStringList guiStyleList = QStyleFactory::keys();
    qDebug() << "Supported GUI styles: " << guiStyleList.join(", ");
    ui->comboBoxGUIStyle->addItems(guiStyleList);
    ui->labelFontSize->setVisible(false);
    ui->spinBoxFontSize->setVisible(false);

    // Load settings.
    qDebug() << "Loading settings:" ;
    loadSettings();

    qDebug() << "Initialization: Loading default (initial) textures.";
    INIT_PROGRESS(80, "Loading default (initial) textures.");

    // Load initial textures.
    diffuseImageWidget   ->setImage(QImage(QString(":/resources/logo/logo_D.png")));
    normalImageWidget    ->setImage(QImage(QString(":/resources/logo/logo_N.png")));
    specularImageWidget  ->setImage(QImage(QString(":/resources/logo/logo_D.png")));
    heightImageWidget    ->setImage(QImage(QString(":/resources/logo/logo_H.png")));
    occlusionImageWidget ->setImage(QImage(QString(":/resources/logo/logo_O.png")));
    roughnessImageWidget ->setImage(QImage(QString(":/resources/logo/logo_R.png")));
    metallicImageWidget  ->setImage(QImage(QString(":/resources/logo/logo_M.png")));
    grungeImageWidget    ->setImage(QImage(QString(":/resources/logo/logo_R.png")));
    materialManager    ->setImage(QImage(QString(":/resources/logo/logo_R.png")));

    // Set the active image
    openGL2DImageWidget->setActiveImage(diffuseImageWidget->getImage());

    INIT_PROGRESS(90, "Updating main menu items.");

    aboutAction = new QAction(QIcon(":/resources/icons/cube.png"), tr("&About %1").arg(qApp->applicationName()), this);
    aboutAction->setToolTip(tr("Show information about AwesomeBump"));
    aboutAction->setMenuRole(QAction::AboutQtRole);
    aboutAction->setMenuRole(QAction::AboutRole);

    shortcutsAction = new QAction(QString("Shortcuts"),this);

    aboutQtAction = new QAction(QIcon(":/resources/icons/Qt.png"), tr("About &Qt"), this);
    aboutQtAction->setToolTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);

    logAction = new QAction("Show log file",this);
    dialogLogger    = new DialogLogger(this, AB_LOG);
    dialogShortcuts = new DialogShortcuts(this);
    //dialogLogger->setModal(true);
    dialogShortcuts->setModal(true);

    connect(aboutAction, SIGNAL (triggered()), this, SLOT (about()));
    connect(aboutQtAction, SIGNAL (triggered()), this, SLOT (aboutQt()));
    connect(logAction, SIGNAL (triggered()), dialogLogger, SLOT (showLog()));
    connect(shortcutsAction, SIGNAL (triggered()), dialogShortcuts, SLOT (show()));

    QMenu *help = menuBar()->addMenu(tr("&Help"));
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
    help->addAction(logAction);
    help->addAction(shortcutsAction);

    QAction *action = ui->toolBar->toggleViewAction();
    ui->menubar->addAction(action);

    selectDiffuseTab();

    // Hide warning icons.
    ui->pushButtonMaterialWarning  ->setVisible(false);
    ui->pushButtonConversionWarning->setVisible(false);
    ui->pushButtonGrungeWarning    ->setVisible(false);
    ui->pushButtonUVWarning        ->setVisible(false);
    ui->pushButtonOccWarning       ->setVisible(false);

    qDebug() << "Initialization: Done - UI ready.";
    INIT_PROGRESS(100, tr("Done - UI ready."));

    setWindowTitle(AWESOME_BUMP_VERSION);
    resize(sizeHint());
}

MainWindow::~MainWindow()
{
    delete dialogLogger;
    delete dialogShortcuts;
    delete materialManager;
    delete settingsContainer;
    delete dock3Dsettings;
    delete diffuseImageWidget;
    delete normalImageWidget;
    delete specularImageWidget;
    delete heightImageWidget;
    delete occlusionImageWidget;
    delete roughnessImageWidget;
    delete grungeImageWidget;
    delete metallicImageWidget;
    delete statusLabel;
    delete openGL2DImageWidget;
    delete openGL3DImageWidget;
    delete abSettings;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent( event );
    settingsContainer->close();
    openGL3DImageWidget->close();
    openGL2DImageWidget->close();
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent( event );
    replotAllImages();
}

void MainWindow::showEvent(QShowEvent* event)
{
    QWidget::showEvent( event );
    replotAllImages();
}

void MainWindow::replotAllImages()
{
    Image* lastActive = openGL2DImageWidget->getActiveImage();
    openGL2DImageWidget->enableShadowRender(true);

    // Skip grunge map if conversion is enabled
    if(openGL2DImageWidget->getConversionType() != CONVERT_FROM_D_TO_O)
    {
        updateImage(GRUNGE_TEXTURE);
    }

    updateImage(DIFFUSE_TEXTURE);
    updateImage(ROUGHNESS_TEXTURE);
    updateImage(METALLIC_TEXTURE);
    updateImage(HEIGHT_TEXTURE);
    // Recalulate normal.
    updateImage(NORMAL_TEXTURE);
    // Recalculate the ambient occlusion.
    updateImage(OCCLUSION_TEXTURE);
    updateImage(SPECULAR_TEXTURE);
    updateImage(MATERIAL_TEXTURE);

    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveImage(lastActive);
    openGL3DImageWidget->update();
}

void MainWindow::materialsToggled(bool toggle)
{
    static bool bLastValue;
    ui->pushButtonMaterialWarning->setVisible(toggle);
    ui->pushButtonUVWarning->setVisible(Image::seamlessMode != SEAMLESS_NONE);
    if(toggle)
    {
        bLastValue = diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion;
        diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion = false;
        ui->pushButtonUVWarning->setVisible(false);
        if(bLastValue)
            replotAllImages();
    }
    else
    {
        diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.EnableConversion = bLastValue;
    }
    diffuseImageWidget->getImage()->getProperties()->BaseMapToOthers.switchState(QtnPropertyStateInvisible,toggle);
}

void MainWindow::checkWarnings()
{
    ui->pushButtonConversionWarning->setVisible(Image::bConversionBaseMap);
    ui->pushButtonGrungeWarning->setVisible(grungeImageWidget->getImage()->getProperties()->Grunge.OverallWeight.value() > 0);
    ui->pushButtonUVWarning->setVisible(Image::seamlessMode != SEAMLESS_NONE);

    bool bOccTest = (occlusionImageWidget->getImage()->getInputImageType() == INPUT_FROM_HO_NO) ||
            (occlusionImageWidget->getImage()->getInputImageType() == INPUT_FROM_HI_NI);
    ui->pushButtonOccWarning->setVisible(bOccTest);
}

void MainWindow::selectDiffuseTab()
{
    ui->tabWidget->setCurrentIndex(0);
    updateImage(DIFFUSE_TEXTURE);
}

void MainWindow::selectNormalTab()
{
    ui->tabWidget->setCurrentIndex(1);
    updateImage(NORMAL_TEXTURE);
}

void MainWindow::selectSpecularTab()
{
    ui->tabWidget->setCurrentIndex(2);
    updateImage(SPECULAR_TEXTURE);
}

void MainWindow::selectHeightTab()
{
    ui->tabWidget->setCurrentIndex(3);
    updateImage(HEIGHT_TEXTURE);
}

void MainWindow::selectOcclusionTab()
{
    ui->tabWidget->setCurrentIndex(4);
    updateImage(OCCLUSION_TEXTURE);
}

void MainWindow::selectRoughnessTab()
{
    ui->tabWidget->setCurrentIndex(5);
    updateImage(ROUGHNESS_TEXTURE);
}

void MainWindow::selectMetallicTab()
{
    ui->tabWidget->setCurrentIndex(6);
    updateImage(METALLIC_TEXTURE);
}

void MainWindow::selectMaterialsTab()
{
    ui->tabWidget->setCurrentIndex(7);
    updateImage(MATERIAL_TEXTURE);
}

void MainWindow::selectGrungeTab()
{
    ui->tabWidget->setCurrentIndex(8);
    updateImage(GRUNGE_TEXTURE);
}

void MainWindow::selectGeneralSettingsTab()
{
    ui->tabWidget->setCurrentIndex(TAB_SETTINGS);
}

void MainWindow::selectUVsTab()
{
    ui->tabWidget->setCurrentIndex(TAB_SETTINGS+1);
}

void MainWindow::fitImage()
{
    openGL2DImageWidget->resetView();
    openGL2DImageWidget->repaint();
}

void MainWindow::showHideTextureTypes(bool)
{
    //qDebug() << "Toggle processing images";

    bool value = ui->checkBoxSaveDiffuse->isChecked();
    diffuseImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(DIFFUSE_TEXTURE, value);
    ui->pushButtonToggleDiffuse->setVisible(value);
    ui->pushButtonToggleDiffuse->setChecked(value);
    ui->actionShowDiffuseImage->setVisible(value);

    value = ui->checkBoxSaveNormal->isChecked();
    normalImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(NORMAL_TEXTURE, value);
    ui->pushButtonToggleNormal->setVisible(value);
    ui->pushButtonToggleNormal->setChecked(value);
    ui->actionShowNormalImage->setVisible(value);

    value = ui->checkBoxSaveHeight->isChecked();
    occlusionImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(OCCLUSION_TEXTURE, value);
    ui->pushButtonToggleOcclusion->setVisible(value);
    ui->pushButtonToggleOcclusion->setChecked(value);
    ui->actionShowOcclusiontImage->setVisible(value);

    value = ui->checkBoxSaveOcclusion->isChecked();
    heightImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(HEIGHT_TEXTURE, value);
    ui->pushButtonToggleHeight->setVisible(value);
    ui->pushButtonToggleHeight->setChecked(value);
    ui->actionShowHeightImage->setVisible(value);

    value = ui->checkBoxSaveSpecular->isChecked();
    specularImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(SPECULAR_TEXTURE, value);
    ui->pushButtonToggleSpecular->setVisible(value);
    ui->pushButtonToggleSpecular->setChecked(value);
    ui->actionShowSpecularImage->setVisible(value);

    value = ui->checkBoxSaveRoughness->isChecked();
    roughnessImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(ROUGHNESS_TEXTURE, value);
    ui->pushButtonToggleRoughness->setVisible(value);
    ui->pushButtonToggleRoughness->setChecked(value);
    ui->actionShowRoughnessImage->setVisible(value);

    value = ui->checkBoxSaveMetallic->isChecked();
    metallicImageWidget->getImage()->setSkipProcessing(!value);
    ui->tabWidget->setTabEnabled(METALLIC_TEXTURE, value);
    ui->pushButtonToggleMetallic->setVisible(value);
    ui->pushButtonToggleMetallic->setChecked(value);
    ui->actionShowMetallicImage->setVisible(value);
}

void MainWindow::saveImages()
{
    QString dir = QFileDialog::getExistingDirectory(
                this, tr("Choose Directory"),
                recentDir.absolutePath(),
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if( dir != "" ) saveAllImages(dir);
}

bool MainWindow::saveAllImages(const QString &dir)
{
    QFileInfo fileInfo(dir);
    if (!fileInfo.exists())
    {
        QMessageBox::information(
                    this, QGuiApplication::applicationDisplayName(),
                    tr("Cannot save to %1.").arg(QDir::toNativeSeparators(dir)));
        return false;
    }

    qDebug() << Q_FUNC_INFO << "Saving to dir:" << fileInfo.absoluteFilePath();

    diffuseImageWidget   ->setImageName(ui->lineEditOutputName->text());
    normalImageWidget    ->setImageName(ui->lineEditOutputName->text());
    heightImageWidget    ->setImageName(ui->lineEditOutputName->text());
    specularImageWidget  ->setImageName(ui->lineEditOutputName->text());
    occlusionImageWidget ->setImageName(ui->lineEditOutputName->text());
    roughnessImageWidget ->setImageName(ui->lineEditOutputName->text());
    metallicImageWidget  ->setImageName(ui->lineEditOutputName->text());

    replotAllImages();
    setCursor(Qt::WaitCursor);
    QCoreApplication::processEvents();
    ui->progressBar->setValue(0);

    if(!bSaveCompressedFormImages)
    {
        ui->labelProgressInfo->setText("Saving diffuse image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveDiffuse->isChecked())
            diffuseImageWidget ->saveFileToDir(dir);
        ui->progressBar->setValue(15);
        ui->labelProgressInfo->setText("Saving normal image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveNormal->isChecked())
            normalImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(30);
        ui->labelProgressInfo->setText("Saving specular image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveSpecular->isChecked())
            specularImageWidget->saveFileToDir(dir);
        ui->progressBar->setValue(45);
        ui->labelProgressInfo->setText("Saving height image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveHeight->isChecked())
            occlusionImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(60);
        ui->labelProgressInfo->setText("Saving occlusion image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveOcclusion->isChecked())
            heightImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(75);
        ui->labelProgressInfo->setText("Saving roughness image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveRoughness->isChecked())
            roughnessImageWidget  ->saveFileToDir(dir);
        ui->progressBar->setValue(90);
        ui->labelProgressInfo->setText("Saving metallic image...");
        if(!bSaveCheckedImages || ui->checkBoxSaveMetallic->isChecked())
            metallicImageWidget ->saveFileToDir(dir);
        ui->progressBar->setValue(100);
    }
    else // Save as compressed format
    {
        qDebug() << "Cannot save compressed format!";
        // This needs to be moved to OpenGL2DImageWidget.
        /*
        QCoreApplication::processEvents();
        openGL2DImageWidget->makeCurrent();

        QOpenGLFramebufferObject *diffuseFBOImage  = diffuseImageWidget->getImage()->getFBO();
        QOpenGLFramebufferObject *normalFBOImage   = normalImageWidget->getImage()->getFBO();
        QOpenGLFramebufferObject *specularFBOImage = specularImageWidget->getImage()->getFBO();
        QOpenGLFramebufferObject *heightFBOImage   = heightImageWidget->getImage()->getFBO();

        QImage diffuseImage = diffuseFBOImage->toImage() ;
        QImage normalImage  = normalFBOImage->toImage();
        QImage heightImage  = specularFBOImage->toImage();
        QImage specularImage= heightFBOImage->toImage();

        ui->progressBar->setValue(20);
        ui->labelProgressInfo->setText("Preparing images...");

        QCoreApplication::processEvents();

        QImage newDiffuseImage = QImage(diffuseImage.width(), diffuseImage.height(), QImage::Format_ARGB32);
        QImage newNormalImage  = QImage(diffuseImage.width(), diffuseImage.height(), QImage::Format_ARGB32);

        unsigned char* newDiffuseBuffer  = newDiffuseImage.bits();
        unsigned char* newNormalBuffer   = newNormalImage.bits();
        unsigned char* srcDiffuseBuffer  = diffuseImage.bits();
        unsigned char* srcNormalBuffer   = normalImage.bits();
        unsigned char* srcSpecularBuffer = specularImage.bits();
        unsigned char* srcHeightBuffer   = heightImage.bits();

        int w = diffuseImage.width();
        int h = diffuseImage.height();

        // Put height or specular color to alpha channel according to compression type
        switch(ui->comboBoxSaveAsOptions->currentIndex())
        {
        case(H_TO_D_AND_S_TO_N):
            for(int i = 0; i <h; i++)
            {
                for(int j = 0; j < w; j++)
                {
                    newDiffuseBuffer[4 * (i * w + j) + 0] = srcDiffuseBuffer[4 * (i * w + j)+0  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 1] = srcDiffuseBuffer[4 * (i * w + j)+1  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 2] = srcDiffuseBuffer[4 * (i * w + j)+2  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 3] = srcHeightBuffer[4 * (i * w + j)+0  ] ;

                    newNormalBuffer[4 * (i * w + j) + 0] = srcNormalBuffer[4 * (i * w + j)+0  ] ;
                    newNormalBuffer[4 * (i * w + j) + 1] = srcNormalBuffer[4 * (i * w + j)+1  ] ;
                    newNormalBuffer[4 * (i * w + j) + 2] = srcNormalBuffer[4 * (i * w + j)+2  ] ;
                    newNormalBuffer[4 * (i * w + j) + 3] = srcSpecularBuffer[4 * (i * w + j)+0  ] ;
                }
            }
            break;
        case(S_TO_D_AND_H_TO_N):
            for(int i = 0; i <h; i++)
            {
                for(int j = 0; j < w; j++)
                {
                    newDiffuseBuffer[4 * (i * w + j) + 0] = srcDiffuseBuffer[4 * (i * w + j)+0  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 1] = srcDiffuseBuffer[4 * (i * w + j)+1  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 2] = srcDiffuseBuffer[4 * (i * w + j)+2  ] ;
                    newDiffuseBuffer[4 * (i * w + j) + 3] = srcSpecularBuffer[4 * (i * w + j)+0  ] ;

                    newNormalBuffer[4 * (i * w + j) + 0] = srcNormalBuffer[4 * (i * w + j)+0  ] ;
                    newNormalBuffer[4 * (i * w + j) + 1] = srcNormalBuffer[4 * (i * w + j)+1  ] ;
                    newNormalBuffer[4 * (i * w + j) + 2] = srcNormalBuffer[4 * (i * w + j)+2  ] ;
                    newNormalBuffer[4 * (i * w + j) + 3] = srcHeightBuffer[4 * (i * w + j)+0  ] ;
                }
            }
            break;
        }

        ui->progressBar->setValue(50);
        ui->labelProgressInfo->setText("Saving diffuse image...");

        diffuseImageWidget->saveImageToDir(dir,newDiffuseImage);

        ui->progressBar->setValue(80);
        ui->labelProgressInfo->setText("Saving diffuse image...");
        normalImageWidget->saveImageToDir(dir,newNormalImage);
        */
    } // End of save as compressed formats.

    QCoreApplication::processEvents();
    ui->progressBar->setValue(100);
    ui->labelProgressInfo->setText("Done!");
    setCursor(Qt::ArrowCursor);

    return true;
}

void MainWindow::saveCheckedImages()
{
    bSaveCheckedImages = true;
    saveImages();
    bSaveCheckedImages = false;
}

void MainWindow::saveCompressedForm()
{
    bSaveCompressedFormImages = true;
    saveImages();
    bSaveCompressedFormImages = false;
}

void MainWindow::updateDiffuseImage()
{
    ui->lineEditOutputName->setText(diffuseImageWidget->getImageName());
    updateImageInformation();
    openGL2DImageWidget->repaint();

    // Replot normal if height was changed in attached mode.
    if(specularImageWidget->getImage()->getInputImageType() == INPUT_FROM_DIFFUSE_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(SPECULAR_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(DIFFUSE_TEXTURE);
    }

    // Replot normal if height was changed in attached mode.
    if(roughnessImageWidget->getImage()->getInputImageType() == INPUT_FROM_DIFFUSE_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(ROUGHNESS_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(DIFFUSE_TEXTURE);
    }

    // Replot normal if height was changed in attached mode.
    if(metallicImageWidget->getImage()->getInputImageType() == INPUT_FROM_DIFFUSE_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(METALLIC_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(DIFFUSE_TEXTURE);
    }

    openGL3DImageWidget->repaint();
}

void MainWindow::updateNormalImage()
{
    ui->lineEditOutputName->setText(normalImageWidget->getImageName());
    openGL2DImageWidget->repaint();

    // Replot normal if was changed in attached mode.
    if(occlusionImageWidget->getImage()->getInputImageType() == INPUT_FROM_HO_NO)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(OCCLUSION_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(NORMAL_TEXTURE);
    }

    openGL3DImageWidget->repaint();
}

void MainWindow::updateSpecularImage()
{
    ui->lineEditOutputName->setText(specularImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateHeightImage()
{
    ui->lineEditOutputName->setText(heightImageWidget->getImageName());
    openGL2DImageWidget->repaint();

    // Replot normal if height was changed in attached mode.
    if(normalImageWidget->getImage()->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(NORMAL_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode.
    if(specularImageWidget->getImage()->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(SPECULAR_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode.
    if(occlusionImageWidget->getImage()->getInputImageType() == INPUT_FROM_HI_NI||
            occlusionImageWidget->getImage()->getInputImageType() == INPUT_FROM_HO_NO)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(OCCLUSION_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode.
    if(roughnessImageWidget->getImage()->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(ROUGHNESS_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }

    // Replot normal if was changed in attached mode
    if(metallicImageWidget->getImage()->getInputImageType() == INPUT_FROM_HEIGHT_OUTPUT)
    {
        openGL2DImageWidget->enableShadowRender(true);
        updateImage(METALLIC_TEXTURE);
        //glImage->updateGL();
        openGL2DImageWidget->enableShadowRender(false);
        // set height tab back again
        updateImage(HEIGHT_TEXTURE);
    }
    openGL3DImageWidget->repaint();
}

void MainWindow::updateOcclusionImage()
{
    ui->lineEditOutputName->setText(occlusionImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateRoughnessImage()
{
    ui->lineEditOutputName->setText(roughnessImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateMetallicImage()
{
    ui->lineEditOutputName->setText(metallicImageWidget->getImageName());
    openGL2DImageWidget->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::updateGrungeImage()
{
    bool test = (grungeImageWidget->getImage()->getProperties()->Grunge.ReplotAll == true);

    // If replot enabled and grunge weight > 0 then replot all textures.
    if(test)
    {
        replotAllImages();
    }
    else
    {
        // Otherwise replot only the grunge map.
        openGL2DImageWidget->repaint();
        openGL3DImageWidget->repaint();
    }
}

void MainWindow::updateImageInformation()
{
    ui->labelCurrentImageWidth ->setNum(diffuseImageWidget->getImage()->width());
    ui->labelCurrentImageHeight->setNum(diffuseImageWidget->getImage()->height());
}

void MainWindow::initializeGL()
{
    static bool one_time = false;
    // Context is vallid at this moment
    if (!one_time)
    {
        one_time = true;
        qDebug() << "calling" << Q_FUNC_INFO;

        // Loading default (initial) textures
        diffuseImageWidget  ->setImage(QImage(QString(":/resources/logo/logo_D.png")));
        normalImageWidget   ->setImage(QImage(QString(":/resources/logo/logo_N.png")));
        specularImageWidget ->setImage(QImage(QString(":/resources/logo/logo_D.png")));
        heightImageWidget   ->setImage(QImage(QString(":/resources/logo/logo_H.png")));
        occlusionImageWidget->setImage(QImage(QString(":/resources/logo/logo_O.png")));
        roughnessImageWidget->setImage(QImage(QString(":/resources/logo/logo_R.png")));
        metallicImageWidget ->setImage(QImage(QString(":/resources/logo/logo_M.png")));
        grungeImageWidget   ->setImage(QImage(QString(":/resources/logo/logo_R.png")));

        diffuseImageWidget  ->setImageName(ui->lineEditOutputName->text());
        normalImageWidget   ->setImageName(ui->lineEditOutputName->text());
        heightImageWidget   ->setImageName(ui->lineEditOutputName->text());
        specularImageWidget ->setImageName(ui->lineEditOutputName->text());
        occlusionImageWidget->setImageName(ui->lineEditOutputName->text());
        roughnessImageWidget->setImageName(ui->lineEditOutputName->text());
        metallicImageWidget ->setImageName(ui->lineEditOutputName->text());
        grungeImageWidget   ->setImageName(ui->lineEditOutputName->text());

        // Set the active image.
        openGL2DImageWidget->setActiveImage(diffuseImageWidget->getImage());
    }
}

void MainWindow::initializeImages()
{
    qDebug() << "MainWindow::Initialization";
    QCoreApplication::processEvents();

    replotAllImages();
    // SSAO recalculation
    Image* lastActive = openGL2DImageWidget->getActiveImage();

    updateImage(OCCLUSION_TEXTURE);
    //glImage->update();
    openGL2DImageWidget->setActiveImage(lastActive);
}

void MainWindow::updateImage(int tType)
{
    switch(tType)
    {
    case DIFFUSE_TEXTURE:
        openGL2DImageWidget->setActiveImage(diffuseImageWidget->getImage());
        break;
    case NORMAL_TEXTURE:
        openGL2DImageWidget->setActiveImage(normalImageWidget->getImage());
        break;
    case SPECULAR_TEXTURE:
        openGL2DImageWidget->setActiveImage(specularImageWidget->getImage());
        break;
    case HEIGHT_TEXTURE:
        openGL2DImageWidget->setActiveImage(heightImageWidget->getImage());
        break;
    case OCCLUSION_TEXTURE:
        openGL2DImageWidget->setActiveImage(occlusionImageWidget->getImage());
        break;
    case ROUGHNESS_TEXTURE:
        openGL2DImageWidget->setActiveImage(roughnessImageWidget->getImage());
        break;
    case METALLIC_TEXTURE:
        openGL2DImageWidget->setActiveImage(metallicImageWidget->getImage());
        break;
    case MATERIAL_TEXTURE:
        openGL2DImageWidget->setActiveImage(materialManager->getImage());
        break;
    case GRUNGE_TEXTURE:
        openGL2DImageWidget->setActiveImage(grungeImageWidget->getImage());
        break;
    default:
        return;
    }
    openGL3DImageWidget->update();
}

void MainWindow::changeWidth(int)
{
    if(ui->pushButtonResizePropTo->isChecked())
        ui->comboBoxResizeHeight->setCurrentText(ui->comboBoxResizeWidth->currentText());
}

void MainWindow::changeHeight(int)
{
    if(ui->pushButtonResizePropTo->isChecked())
        ui->comboBoxResizeWidth->setCurrentText(ui->comboBoxResizeHeight->currentText());
}

void MainWindow::applyResizeImage()
{
    QCoreApplication::processEvents();
    int width  = ui->comboBoxResizeWidth->currentText().toInt();
    int height = ui->comboBoxResizeHeight->currentText().toInt();
    qDebug() << "Image resize applied. Current image size is (" << width << "," << height << ")" ;

    int materiaIndex = Image::currentMaterialIndex;
    materialManager->disableMaterials();

    Image* lastActive = openGL2DImageWidget->getActiveImage();
    openGL2DImageWidget->enableShadowRender(true);
    for(int i = 0 ; i < MAX_TEXTURES_TYPE ; i++)
    {
        if( i != GRUNGE_TEXTURE)
        {
            // Grunge map does not scale like other images.
            openGL2DImageWidget->resizeFBO(width,height);
            updateImage(i);
        }
    }
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveImage(lastActive);
    replotAllImages();
    updateImageInformation();
    openGL3DImageWidget->repaint();

    // Replot all material group after image resize.
    Image::currentMaterialIndex = materiaIndex;
    if(materialManager->isEnabled())
    {
        materialManager->toggleMaterials(true);
    }
}

void MainWindow::applyResizeImage(int width, int height)
{
    QCoreApplication::processEvents();

    qDebug() << "Image resize applied. Current image size is (" << width << "," << height << ")" ;
    int materiaIndex = Image::currentMaterialIndex;
    materialManager->disableMaterials();
    Image* lastActive = openGL2DImageWidget->getActiveImage();
    openGL2DImageWidget->enableShadowRender(true);
    for(int i = 0 ; i < MAX_TEXTURES_TYPE ; i++)
    {
        if( i != GRUNGE_TEXTURE)
        {
            openGL2DImageWidget->resizeFBO(width,height);
            updateImage(i);
        }
    }
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveImage(lastActive);
    replotAllImages();
    updateImageInformation();
    openGL3DImageWidget->repaint();

    // Replot all material group after image resize.
    Image::currentMaterialIndex = materiaIndex;
    if(materialManager->isEnabled())
    {
        materialManager->toggleMaterials(true);
    }
}

void MainWindow::scaleWidth(double)
{
    if(ui->pushButtonRescalePropTo->isChecked())
    {
        ui->doubleSpinBoxRescaleHeight->setValue(ui->doubleSpinBoxRescaleWidth->value());
    }
}

void MainWindow::scaleHeight(double)
{
    if(ui->pushButtonRescalePropTo->isChecked())
    {
        ui->doubleSpinBoxRescaleWidth->setValue(ui->doubleSpinBoxRescaleHeight->value());
    }
}

void MainWindow::applyScaleImage()
{
    QCoreApplication::processEvents();
    float scale_width   = ui->doubleSpinBoxRescaleWidth ->value();
    float scale_height  = ui->doubleSpinBoxRescaleHeight->value();
    int width  = diffuseImageWidget->getImage()->width() * scale_width;
    int height = diffuseImageWidget->getImage()->height() * scale_height;

    qDebug() << "Image rescale applied. Current image size is (" << width << "," << height << ")" ;
    int materiaIndex = Image::currentMaterialIndex;
    materialManager->disableMaterials();
    Image* lastActive = openGL2DImageWidget->getActiveImage();
    openGL2DImageWidget->enableShadowRender(true);
    for(int i = 0 ; i < MAX_TEXTURES_TYPE ; i++)
    {
        openGL2DImageWidget->resizeFBO(width,height);
        updateImage(i);
    }
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setActiveImage(lastActive);
    replotAllImages();
    updateImageInformation();
    openGL3DImageWidget->repaint();

    // Replot all material group after image resize.
    Image::currentMaterialIndex = materiaIndex;
    if(materialManager->isEnabled())
    {
        materialManager->toggleMaterials(true);
    }
}

void MainWindow::applyCurrentUVsTransformations()
{
    // Get current diffuse image (with applied UVs transformations).
    QImage diffuseImage = diffuseImageWidget->getImage()->getFBOImage();
    // Reset all the transformations
    ui->comboBoxSeamlessMode->setCurrentIndex(0);
    selectSeamlessMode(0);
    resetTransform();
    // Set as default.
    diffuseImageWidget->setImage(diffuseImage);
    // Generate all textures based on new image.
    bool bConvValue = diffuseImageWidget->getImage()->bConversionBaseMap;
    diffuseImageWidget->getImage()->bConversionBaseMap = true;
    convertFromBase();
    diffuseImageWidget->getImage()->bConversionBaseMap = bConvValue;
}

void MainWindow::selectSeamlessMode(int mode)
{
    // some gui interaction -> hide and show
    ui->groupBoxSimpleSeamlessMode->hide();
    ui->groupBoxMirrorMode->hide();
    ui->groupBoxRandomPatchesMode->hide();
    ui->groupBoxUVContrastSettings->setDisabled(false);
    ui->horizontalSliderSeamlessContrastPower->setEnabled(true);
    ui->comboBoxSeamlessContrastInputImage->setEnabled(true);
    ui->doubleSpinBoxSeamlessContrastPower->setEnabled(true);
    switch(mode)
    {
    case SEAMLESS_NONE:
        break;
    case SEAMLESS_SIMPLE:
        ui->groupBoxSimpleSeamlessMode->show();
        ui->labelContrastStrenght->setText("Contrast strenght");
        break;
    case SEAMLESS_MIRROR:
        ui->groupBoxMirrorMode->show();
        ui->groupBoxUVContrastSettings->setDisabled(true);
        break;
    case SEAMLESS_RANDOM:
        ui->groupBoxRandomPatchesMode->show();
        ui->labelContrastStrenght->setText("Radius");
        ui->doubleSpinBoxSeamlessContrastPower->setEnabled(false);
        ui->horizontalSliderSeamlessContrastPower->setEnabled(false);
        ui->comboBoxSeamlessContrastInputImage->setEnabled(false);
        break;
    default:
        break;
    }
    openGL2DImageWidget->selectSeamlessMode((SeamlessMode)mode);
    checkWarnings();
    replotAllImages();
}

void MainWindow::selectContrastInputImage(int mode)
{
    switch(mode)
    {
    case(0):
        Image::seamlessContrastImageType = INPUT_FROM_HEIGHT_INPUT;
        break;
    case(1):
        Image::seamlessContrastImageType = INPUT_FROM_DIFFUSE_INPUT;
        break;
    case(2):
        Image::seamlessContrastImageType = INPUT_FROM_NORMAL_INPUT;
        break;
    case(3):
        Image::seamlessContrastImageType = INPUT_FROM_OCCLUSION_INPUT;
        break;
    default:
        break;
    }
    replotAllImages();
}

void MainWindow::selectSourceImages()
{
    QString startPath;
    if(recentDir.exists())
        startPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    else
        startPath = recentDir.absolutePath();

    QString source = QFileDialog::getExistingDirectory(
                this, tr("Select source directory"),
                startPath,
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    QDir dir(source);
    qDebug() << "Selecting source folder for batch processing: " << source;

    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.bmp" << "*.tga";
    QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files | QDir::NoDotAndDotDot);

    ui->listWidgetImageBatch->clear();
    foreach (QFileInfo fileInfo, fileInfoList)
    {
        qDebug() << "Found:" << fileInfo.absoluteFilePath();
        ui->listWidgetImageBatch->addItem(fileInfo.fileName());
    }
    ui->lineEditImageBatchSource->setText(source);
}

void MainWindow::selectOutputPath()
{
    QString startPath;
    if(recentDir.exists())
        startPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    else
        startPath = recentDir.absolutePath();

    QString path = QFileDialog::getExistingDirectory(
                this, tr("Select source directory"),
                startPath,
                QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    ui->lineEditImageBatchOutput->setText(path);
}

void MainWindow::runBatch()
{
    QString sourceFolder = ui->lineEditImageBatchSource->text();
    QString outputFolder = ui->lineEditImageBatchOutput->text();

    // Check if output path exists.
    if(!QDir(outputFolder).exists() || outputFolder == "")
    {
        QMessageBox msgBox;
        msgBox.setText("Info");
        msgBox.setInformativeText("Output path is not provided");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    qDebug() << "Starting batch mode: this may take some time";
    while(ui->listWidgetImageBatch->count() > 0)
    {
        QListWidgetItem* item = ui->listWidgetImageBatch->takeItem(0);
        ui->labelBatchProgress->setText("Images left: " + QString::number(ui->listWidgetImageBatch->count()+1));
        ui->labelBatchProgress->repaint();
        QCoreApplication::processEvents();

        QString imageName = item->text();
        ui->lineEditOutputName->setText(imageName);
        QString imagePath = sourceFolder + "/" + imageName;

        qDebug() << "Processing image: " << imagePath;
        diffuseImageWidget->loadFile(imagePath);
        convertFromBase();
        saveAllImages(outputFolder);

        delete item;
        ui->listWidgetImageBatch->repaint();
        QCoreApplication::processEvents();
    }

    ui->labelBatchProgress->setText("Done.");
}


void MainWindow::randomizeAngles()
{
    Image::randomize();
    replotAllImages();
}

void MainWindow::resetRandomPatches()
{
    Image::randomReset();
    ui->horizontalSliderRandomPatchesRotate     ->setValue(Image::randomCommonPhase);
    ui->horizontalSliderRandomPatchesInnerRadius->setValue(Image::randomInnerRadius * 100.0);
    ui->horizontalSliderRandomPatchesOuterRadius->setValue(Image::randomOuterRadius * 100.0);
    updateSpinBoxes(0);
    replotAllImages();
}

void MainWindow::updateSpinBoxes(int)
{
    ui->doubleSpinBoxMakeSeamless->setValue(ui->horizontalSliderMakeSeamlessRadius->value()/100.0);

    // Random tilling mode.
    ui->doubleSpinBoxRandomPatchesAngle      ->setValue(ui->horizontalSliderRandomPatchesRotate     ->value());
    ui->doubleSpinBoxRandomPatchesInnerRadius->setValue(ui->horizontalSliderRandomPatchesInnerRadius->value()/100.0);
    ui->doubleSpinBoxRandomPatchesOuterRadius->setValue(ui->horizontalSliderRandomPatchesOuterRadius->value()/100.0);
    // Seamless strength.
    ui->doubleSpinBoxSeamlessContrastStrenght->setValue(ui->horizontalSliderSeamlessContrastStrenght->value()/100.0);
    ui->doubleSpinBoxSeamlessContrastPower->setValue(ui->horizontalSliderSeamlessContrastPower->value()/100.0);
}

void MainWindow::selectShadingModel(int i)
{
    if(i == 0) ui->tabWidget->setTabText(5,"Rgnss");
    if(i == 1) ui->tabWidget->setTabText(5,"Gloss");
}

void MainWindow::convertFromHtoN()
{
    openGL2DImageWidget->setConversionType(CONVERT_FROM_H_TO_N);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveImage(heightImageWidget->getImage());
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setConversionType(CONVERT_FROM_H_TO_N);
    openGL2DImageWidget->setActiveImage(normalImageWidget->getImage());
    openGL2DImageWidget->enableShadowRender(false);
    openGL2DImageWidget->setConversionType(CONVERT_NONE);

    replotAllImages();

    qDebug() << "Conversion from height to normal applied";
}

void MainWindow::convertFromNtoH()
{
    openGL2DImageWidget->setConversionType(CONVERT_FROM_H_TO_N);// fake conversion
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveImage(heightImageWidget->getImage());
    openGL2DImageWidget->setConversionType(CONVERT_FROM_N_TO_H);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveImage(normalImageWidget->getImage());
    openGL2DImageWidget->setConversionType(CONVERT_FROM_N_TO_H);
    openGL2DImageWidget->enableShadowRender(true);
    openGL2DImageWidget->setActiveImage(heightImageWidget->getImage());
    openGL2DImageWidget->enableShadowRender(false);
    replotAllImages();

    qDebug() << "Conversion from normal to height applied";
}

void MainWindow::convertFromBase()
{
    Image* lastActive = openGL2DImageWidget->getActiveImage();
    openGL2DImageWidget->setActiveImage(diffuseImageWidget->getImage());
    qDebug() << "Conversion from Base to others started";
    normalImageWidget   ->setImageName(diffuseImageWidget->getImageName());
    heightImageWidget   ->setImageName(diffuseImageWidget->getImageName());
    specularImageWidget ->setImageName(diffuseImageWidget->getImageName());
    occlusionImageWidget->setImageName(diffuseImageWidget->getImageName());
    roughnessImageWidget->setImageName(diffuseImageWidget->getImageName());
    metallicImageWidget ->setImageName(diffuseImageWidget->getImageName());
    openGL2DImageWidget->setConversionType(CONVERT_FROM_D_TO_O);
    openGL2DImageWidget->update();
    openGL2DImageWidget->setConversionType(CONVERT_FROM_D_TO_O);
    replotAllImages();

    openGL2DImageWidget->setActiveImage(lastActive);
    openGL3DImageWidget->update();
    qDebug() << "Conversion from Base to others applied";
}

void MainWindow::convertFromHNtoOcc()
{
    openGL2DImageWidget->setConversionType(CONVERT_FROM_HN_TO_OC);
    openGL2DImageWidget->enableShadowRender(true);

    openGL2DImageWidget->setActiveImage(heightImageWidget->getImage());
    openGL2DImageWidget->setConversionType(CONVERT_FROM_HN_TO_OC);
    openGL2DImageWidget->enableShadowRender(true);

    openGL2DImageWidget->setActiveImage(normalImageWidget->getImage());
    openGL2DImageWidget->setConversionType(CONVERT_FROM_HN_TO_OC);
    openGL2DImageWidget->enableShadowRender(true);

    openGL2DImageWidget->setActiveImage(occlusionImageWidget->getImage());
    openGL2DImageWidget->enableShadowRender(false);

    replotAllImages();

    qDebug() << "Conversion from Height and Normal to Occlusion applied";
}

void MainWindow::updateSliders()
{
    updateSpinBoxes(0);
    Image::seamlessSimpleModeRadius          = ui->doubleSpinBoxMakeSeamless->value();
    Image::seamlessContrastStrength          = ui->doubleSpinBoxSeamlessContrastStrenght->value();
    Image::seamlessContrastPower             = ui->doubleSpinBoxSeamlessContrastPower->value();
    Image::randomCommonPhase = ui->doubleSpinBoxRandomPatchesAngle->value()/180.0*3.1415926;
    Image::randomInnerRadius = ui->doubleSpinBoxRandomPatchesInnerRadius->value();
    Image::randomOuterRadius = ui->doubleSpinBoxRandomPatchesOuterRadius->value();

    Image::bSeamlessTranslationsFirst = ui->checkBoxUVTranslationsFirst->isChecked();
    // Choose the proper mirror mode.
    if(ui->radioButtonMirrorModeXY->isChecked()) Image::seamlessMirroModeType = 0;
    if(ui->radioButtonMirrorModeX ->isChecked()) Image::seamlessMirroModeType = 1;
    if(ui->radioButtonMirrorModeY ->isChecked()) Image::seamlessMirroModeType = 2;

    // Choose the proper simple mode direction.
    if(ui->radioButtonSeamlessSimpleDirXY->isChecked()) Image::seamlessSimpleModeDirection = 0;
    if(ui->radioButtonSeamlessSimpleDirX ->isChecked()) Image::seamlessSimpleModeDirection = 1;
    if(ui->radioButtonSeamlessSimpleDirY ->isChecked()) Image::seamlessSimpleModeDirection = 2;

    openGL2DImageWidget ->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::resetTransform()
{
    QVector2D corner(0,0);
    openGL2DImageWidget->updateCornersPosition(corner,corner,corner,corner);
    openGL2DImageWidget->updateCornersWeights(0,0,0,0);
    replotAllImages();
}

void MainWindow::setUVManipulationMethod()
{
    if(ui->actionTranslateUV->isChecked()) openGL2DImageWidget->selectUVManipulationMethod(UV_TRANSLATE);
    if(ui->actionGrabCorners->isChecked()) openGL2DImageWidget->selectUVManipulationMethod(UV_GRAB_CORNERS);
    if(ui->actionScaleXY->isChecked())     openGL2DImageWidget->selectUVManipulationMethod(UV_SCALE_XY);
}

QSize MainWindow::sizeHint() const
{
    return QSize(abSettings->d_win_w,abSettings->d_win_h);
}

void MainWindow::loadImageSettings(TextureType type)
{
    switch(type)
    {
    case DIFFUSE_TEXTURE:
        diffuseImageWidget    ->getImage()->getProperties()->copyValues(&abSettings->Diffuse);
        break;
    case NORMAL_TEXTURE:
        normalImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Normal);
        break;
    case SPECULAR_TEXTURE:
        specularImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Specular);
        break;
    case HEIGHT_TEXTURE:
        heightImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Height);
        break;
    case OCCLUSION_TEXTURE:
        occlusionImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Occlusion);
        break;
    case ROUGHNESS_TEXTURE:
        roughnessImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Roughness);
        break;
    case METALLIC_TEXTURE:
        metallicImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Metallic);
        break;
    case GRUNGE_TEXTURE:
        grungeImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Grunge);
        break;
    default:
        qWarning() << "Trying to load non supported image! Given textureType:" << type;
    }
    openGL2DImageWidget ->repaint();
    openGL3DImageWidget->repaint();
}

void MainWindow::showSettingsManager()
{
    settingsContainer->show();
}

void MainWindow::saveSettings()
{
    qDebug() << "Calling" << Q_FUNC_INFO << "Saving to :"<< QString(AB_INI);

    abSettings->d_win_w =  this->width();
    abSettings->d_win_h =  this->height();
    abSettings->tab_win_w = ui->tabWidget->width();
    abSettings->tab_win_h = ui->tabWidget->height();
    abSettings->recent_dir      = recentDir.absolutePath();
    abSettings->recent_mesh_dir = recentMeshDir.absolutePath();

    Image::diffuseName   = ui->lineEditPostfixDiffuse->text();
    Image::normalName    = ui->lineEditPostfixNormal->text();
    Image::specularName  = ui->lineEditPostfixSpecular->text();
    Image::heightName    = ui->lineEditPostfixHeight->text();
    Image::occlusionName = ui->lineEditPostfixOcclusion->text();
    Image::roughnessName = ui->lineEditPostfixRoughness->text();
    Image::metallicName  = ui->lineEditPostfixMetallic->text();

    abSettings->d_postfix=ui->lineEditPostfixDiffuse->text();
    abSettings->d_postfix=ui->lineEditPostfixDiffuse->text();
    abSettings->n_postfix=ui->lineEditPostfixNormal->text();
    abSettings->s_postfix=ui->lineEditPostfixSpecular->text();
    abSettings->h_postfix=ui->lineEditPostfixHeight->text();
    abSettings->o_postfix=ui->lineEditPostfixOcclusion->text();
    abSettings->r_postfix=ui->lineEditPostfixRoughness->text();
    abSettings->m_postfix=ui->lineEditPostfixMetallic->text();

    abSettings->gui_style=ui->comboBoxGUIStyle->currentText();

    // UV settings.
    abSettings->uv_tiling_type=ui->comboBoxSeamlessMode->currentIndex();
    abSettings->uv_tiling_radius=ui->horizontalSliderMakeSeamlessRadius->value();
    abSettings->uv_tiling_mirror_x=ui->radioButtonMirrorModeX->isChecked();
    abSettings->uv_tiling_mirror_y=ui->radioButtonMirrorModeY->isChecked();
    abSettings->uv_tiling_mirror_xy=ui->radioButtonMirrorModeXY->isChecked();
    abSettings->uv_tiling_random_inner_radius=ui->horizontalSliderRandomPatchesInnerRadius->value();
    abSettings->uv_tiling_random_outer_radius=ui->horizontalSliderRandomPatchesOuterRadius->value();
    abSettings->uv_tiling_random_rotate=ui->horizontalSliderRandomPatchesRotate->value();

    // UV contrast etc.
    abSettings->uv_translations_first=ui->checkBoxUVTranslationsFirst->isChecked();
    abSettings->uv_contrast_strength=ui->doubleSpinBoxSeamlessContrastStrenght->value();
    abSettings->uv_contrast_power=ui->doubleSpinBoxSeamlessContrastPower->value();
    abSettings->uv_contrast_input_image=ui->comboBoxSeamlessContrastInputImage->currentIndex();
    abSettings->uv_tiling_simple_dir_xy=ui->radioButtonSeamlessSimpleDirXY->isChecked();
    abSettings->uv_tiling_simple_dir_x=ui->radioButtonSeamlessSimpleDirX->isChecked();
    abSettings->uv_tiling_simple_dir_y=ui->radioButtonSeamlessSimpleDirY->isChecked();

    // Other parameters.
    abSettings->use_texture_interpolation=ui->checkBoxUseLinearTextureInterpolation->isChecked();
    abSettings->mouse_sensitivity=ui->spinBoxMouseSensitivity->value();
    abSettings->font_size=ui->spinBoxFontSize->value();
    abSettings->mouse_loop=ui->checkBoxToggleMouseLoop->isChecked();

    dock3Dsettings->saveSettings(abSettings);

    abSettings->Diffuse  .copyValues(diffuseImageWidget   ->getImage()->getProperties());
    abSettings->Specular .copyValues(specularImageWidget  ->getImage()->getProperties());
    abSettings->Normal   .copyValues(normalImageWidget    ->getImage()->getProperties());
    abSettings->Occlusion.copyValues(occlusionImageWidget ->getImage()->getProperties());
    abSettings->Height   .copyValues(heightImageWidget    ->getImage()->getProperties());
    abSettings->Metallic .copyValues(metallicImageWidget  ->getImage()->getProperties());
    abSettings->Roughness.copyValues(roughnessImageWidget ->getImage()->getProperties());
    abSettings->Grunge   .copyValues(grungeImageWidget    ->getImage()->getProperties());

    // Disable possibility to save conversion status
    //    abSettings->Diffuse.BaseMapToOthers.EnableConversion.setValue(false);

    QFile file( QString(AB_INI) );
    if( !file.open( QIODevice::WriteOnly ) )
        return;
    QTextStream stream(&file);
    QString data;
    abSettings->toStr(data);
    stream << data;
}

void MainWindow::changeGUIFontSize(int value)
{
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPixelSize(value);
    QApplication::setFont(font);
}

void MainWindow::setOutputFormat(int)
{
    Image::outputFormat = ui->comboBoxImageOutputFormat->currentText();
}

void MainWindow::loadSettings()
{
    static bool bFirstTime = true;

    qDebug() << "Calling" << Q_FUNC_INFO << " loading from " << QString(AB_INI);

    ImageWidget::loadingImages = true;

    QFile file( QString(AB_INI) );
    if( !file.open( QIODevice::ReadOnly ) )
        return;

    QTextStream stream(&file);
    QString data;

    //Skip one line
    stream.readLine();
    data = stream.readAll();
    abSettings->fromStr(data);

    QString name = abSettings->settings_name.value();
    ui->pushButtonProjectManager->setText("Project manager (" + name + ")");

    diffuseImageWidget    ->getImage()->getProperties()->copyValues(&abSettings->Diffuse);
    specularImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Specular);
    normalImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Normal);
    occlusionImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Occlusion);
    heightImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Height);
    metallicImageWidget   ->getImage()->getProperties()->copyValues(&abSettings->Metallic);
    roughnessImageWidget  ->getImage()->getProperties()->copyValues(&abSettings->Roughness);
    grungeImageWidget     ->getImage()->getProperties()->copyValues(&abSettings->Grunge);

    // Update general settings.
    if(bFirstTime)
    {
        this->resize(abSettings->d_win_w,abSettings->d_win_h);
        ui->tabWidget->resize(abSettings->tab_win_w,abSettings->tab_win_h);
    }

    Image::diffuseName   = abSettings->d_postfix;
    Image::normalName    = abSettings->n_postfix;
    Image::specularName  = abSettings->s_postfix;
    Image::heightName    = abSettings->h_postfix;
    Image::occlusionName = abSettings->o_postfix;
    Image::roughnessName = abSettings->m_postfix;
    Image::metallicName  = abSettings->r_postfix;

    showHideTextureTypes(true);

    ui->lineEditPostfixDiffuse  ->setText(Image::diffuseName);
    ui->lineEditPostfixNormal   ->setText(Image::normalName);
    ui->lineEditPostfixSpecular ->setText(Image::specularName);
    ui->lineEditPostfixHeight   ->setText(Image::heightName);
    ui->lineEditPostfixOcclusion->setText(Image::occlusionName);
    ui->lineEditPostfixRoughness->setText(Image::roughnessName);
    ui->lineEditPostfixMetallic ->setText(Image::metallicName);

    recentDir     = abSettings->recent_dir;
    recentMeshDir = abSettings->recent_mesh_dir;

    ui->checkBoxUseLinearTextureInterpolation->setChecked(abSettings->use_texture_interpolation);
    Image::bUseLinearInterpolation = ui->checkBoxUseLinearTextureInterpolation->isChecked();
    ui->comboBoxGUIStyle->setCurrentText(abSettings->gui_style);

    // UV Settings.
    ui->comboBoxSeamlessMode->setCurrentIndex(abSettings->uv_tiling_type);
    selectSeamlessMode(ui->comboBoxSeamlessMode->currentIndex());
    ui->horizontalSliderMakeSeamlessRadius->setValue(abSettings->uv_tiling_radius);
    ui->radioButtonMirrorModeX->setChecked(abSettings->uv_tiling_mirror_x);
    ui->radioButtonMirrorModeY->setChecked(abSettings->uv_tiling_mirror_y);
    ui->radioButtonMirrorModeXY->setChecked(abSettings->uv_tiling_mirror_xy);
    ui->horizontalSliderRandomPatchesInnerRadius->setValue(abSettings->uv_tiling_random_inner_radius);
    ui->horizontalSliderRandomPatchesOuterRadius->setValue(abSettings->uv_tiling_random_outer_radius);
    ui->horizontalSliderRandomPatchesRotate->setValue(abSettings->uv_tiling_random_rotate);

    ui->radioButtonSeamlessSimpleDirXY->setChecked(abSettings->uv_tiling_simple_dir_xy);
    ui->radioButtonSeamlessSimpleDirX->setChecked(abSettings->uv_tiling_simple_dir_x);
    ui->radioButtonSeamlessSimpleDirY->setChecked(abSettings->uv_tiling_simple_dir_y);

    ui->checkBoxUVTranslationsFirst->setChecked(abSettings->uv_translations_first);
    ui->horizontalSliderSeamlessContrastStrenght->setValue(abSettings->uv_contrast_strength*100);
    ui->horizontalSliderSeamlessContrastPower->setValue(abSettings->uv_contrast_power*100);
    ui->comboBoxSeamlessContrastInputImage->setCurrentIndex(abSettings->uv_contrast_input_image);

    // Other settings.
    ui->spinBoxMouseSensitivity->setValue(abSettings->mouse_sensitivity);
    ui->spinBoxFontSize->setValue(abSettings->font_size);
    ui->checkBoxToggleMouseLoop->setChecked(abSettings->mouse_loop);

    updateSliders();

    dock3Dsettings->loadSettings(abSettings);

    heightImageWidget->reloadSettings();

    ImageWidget::loadingImages = false;

    replotAllImages();

    openGL2DImageWidget ->repaint();
    openGL3DImageWidget->repaint();
    bFirstTime = false;
}

void MainWindow::about()
{
    QMessageBox::about(this, tr(AWESOME_BUMP_VERSION),
                       tr("AwesomeBump is an open source program designed to generate normal, "
                          "height, specular or ambient occlusion, roughness and metallic textures from a single image. "
                          "Since the image processing is done in 99% on GPU  the program runs very fast "
                          "and all the parameters can be changed in real time.\n "
                          "Program written by: \n Krzysztof Kolasinski and Pawel Piecuch (Copyright 2015-2016) with collaboration \n"
                          "with other people! See project collaborators list on github. "));
}

void MainWindow::aboutQt()
{
    QMessageBox::aboutQt(this, tr(AWESOME_BUMP_VERSION));
}

/*
if(!checkOpenGL()){
    AllAboutDialog msgBox;
    msgBox.setPixmap(":/resources/icons/icon-off.png");
    msgBox.setText("Fatal Error!");
    msgBox.setInformativeText(QString("Sorry but it seems that your graphics card does not support openGL %1.%2.\n"
                                      "Program will not run :(\n"
                                      "See " AB_LOG " file for more info.").arg(GL_MAJOR).arg(GL_MINOR));
    msgBox.setVersionText(AWESOME_BUMP_VERSION);
    msgBox.show();

    return app.exec();
}
*/

/*
void displayOpenGLInformation(bool includeExtensions)
{
    qDebug() << Q_FUNC_INFO;

    qDebug() << QString("OpenGL version: %1.%2")
                .arg(context()->format().majorVersion())
                .arg(context()->format().minorVersion());

    qDebug() << "GL version:"
             << (const char*)glGetString(GL_VERSION);
    qDebug() << "GL vendor:"
             << (const char*)glGetString(GL_VENDOR);
    qDebug() << "GL renderer:"
             << (const char*)glGetString(GL_RENDERER);
    qDebug() << "GLSL version:"
             << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    if (!includeExtensions) return;

    GLint numberOfExentsions = 0;
    GLCHK( glGetIntegerv(GL_NUM_EXTENSIONS, &numberOfExentsions) );
    qDebug() << numberOfExentsions << " extensions:";
    for (int i = 0; i < numberOfExentsions; i++)
    {
        qDebug() << (const char*)glGetStringi(GL_EXTENSIONS, i);
    }
}
*/

void customMessageHandler(
        QtMsgType type,
        const QMessageLogContext& context,
        const QString& msg)
{
    Q_UNUSED(context);

    QString dateTimeString = QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm:ss");
    QString errorMessage = QString("[%1] ").arg(dateTimeString);

    switch (type)
    {
    case QtDebugMsg:
        errorMessage += QString("{Debug} \t\t %1").arg(msg);
        break;
    case QtWarningMsg:
        errorMessage += QString("{Warning} \t %1").arg(msg);
        break;
    case QtCriticalMsg:
        errorMessage += QString("{Critical} \t %1").arg(msg);
        break;
    case QtFatalMsg:
        errorMessage += QString("{Fatal} \t\t %1").arg(msg);
        abort();
        break;
    case QtInfoMsg:
        break;
    }

    // Avoid recursive calling.
    QFile logFile(AB_LOG);
    if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        // Try any safe location.
        QString glob = QString("%1/%2")
                .arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
                .arg(QFileInfo(AB_LOG).fileName());
        logFile.setFileName(glob);
        if (!logFile.open(QIODevice::WriteOnly | QIODevice::Append))
            return;
    }

    QTextStream logFileStream(&logFile);
    logFileStream << errorMessage << endl;
}
