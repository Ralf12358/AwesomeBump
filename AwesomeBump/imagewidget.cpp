#include "imagewidget.h"
#include "ui_imagewidget.h"

#include <QDebug>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>

#include "opengl2dimagewidget.h"

bool ImageWidget::loadingImages = false;
QDir* ImageWidget::recentDir;

ImageWidget::ImageWidget(QWidget* parent, OpenGL2DImageWidget* openGL2DImageWidget, TextureType textureType) :
    QWidget(parent), openGL2DImageWidget(openGL2DImageWidget), textureType(textureType), inputImageType(INPUT_NONE),
    ui(new Ui::ImageWidget)
{
    ui->setupUi(this);
    ui->widgetProperty->setParts(QtnPropertyWidgetPartsDescriptionPanel);
    ui->widgetProperty->setPropertySet(&properties);
    ui->widgetProperty->layout()->setMargin(0);
    ui->widgetProperty->layout()->setSpacing(0);

    connect(&properties,
            SIGNAL (propertyDidChange(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)), this,
            SLOT (propertyChanged(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)));

    connect(&properties.BaseMapToOthers.Convert,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (applyBaseConversion(const QtnPropertyButton*)));

    connect(&properties.NormalsMixer.PasteFromClipboard,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (pasteNormalFromClipBoard(const QtnPropertyButton*)));


    connect(&properties.BaseMapToOthers.MinColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));

    connect(&properties.BaseMapToOthers.MaxColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));


    connect(&properties.RMFilter.ColorFilter.PickColor,
                     SIGNAL(click( QtnPropertyABColor*)), this,
                     SLOT(pickColorFromImage( QtnPropertyABColor*)));

    ui->groupBoxConvertToHeightSettings->hide();

    connect(ui->pushButtonOpenImage,          SIGNAL (released()), this, SLOT (open()));
    connect(ui->pushButtonSaveImage,          SIGNAL (released()), this, SLOT (save()));
    connect(ui->pushButtonCopyToClipboard,    SIGNAL (released()), this, SLOT (copyToClipboard()));
    connect(ui->pushButtonPasteFromClipboard, SIGNAL (released()), this, SLOT (pasteFromClipboard()));
    connect(ui->pushButtonRestoreSettings,    SIGNAL (released()), this, SLOT(reloadImageSettings()));

    // Height conversion buttons.
    connect(ui->horizontalSliderConversionHNDepth, SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->pushButtonConverToNormal,      SIGNAL (released()), this, SLOT (applyHeightToNormalConversion()));
    connect(ui->pushButtonShowDepthCalculator, SIGNAL (released()), this, SLOT (showHeightCalculatorDialog()));

    // Normal convertion buttons and sliders.
    connect(ui->horizontalSliderNormalToHeightNoiseLevel,     SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->horizontalSliderNormalToHeightItersHuge,      SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->horizontalSliderNormalToHeightItersVeryLarge, SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->horizontalSliderNormalToHeightItersLarge,     SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->horizontalSliderNormalToHeightItersMedium,    SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->horizontalSliderNormalToHeightItersSmall,     SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));
    connect(ui->horizontalSliderNormalToHeightItersVerySmall, SIGNAL (sliderReleased()), this, SLOT (updateSlidersOnRelease()));

    connect(ui->pushButtonConvertToHeight,        SIGNAL(released()), this, SLOT(applyNormalToHeightConversion()));
    connect(ui->pushButtonConvertOcclusionFromHN, SIGNAL(released()), this, SLOT(applyHeightNormalToOcclusionConversion()));

    // Input image boxes.
    connect(ui->comboBoxNormalInputImage,    SIGNAL (activated(int)), this, SLOT (updateComboBoxes(int)));
    connect(ui->comboBoxSpecularInputImage,  SIGNAL (activated(int)), this, SLOT (updateComboBoxes(int)));
    connect(ui->comboBoxOcclusionInputImage, SIGNAL (activated(int)), this, SLOT (updateComboBoxes(int)));
    connect(ui->comboBoxRoughnessInputImage, SIGNAL (activated(int)), this, SLOT (updateComboBoxes(int)));

    heightCalculator = new DialogHeightCalculator;

    setAcceptDrops(true);
    setMouseTracking(true);
    setFocus();
    setFocusPolicy(Qt::ClickFocus);

    setupPropertiesGUI();
    setImageName("image");
}

ImageWidget::~ImageWidget()
{
    qDebug() << "calling" << Q_FUNC_INFO;
    delete heightCalculator;
    delete ui;
}

Image* ImageWidget::getImage()
{
    return &image;
}

QtnPropertySetFormImageProp* ImageWidget::getProperties()
{
    return &properties;
}

ImageType ImageWidget::getInputImageType()
{
    return inputImageType;
}

QString ImageWidget::getImageName()
{
    return imageName;
}

void ImageWidget::setImageName(const QString& name)
{
    imageName = name;
}

void ImageWidget::setupPropertiesGUI()
{
    properties.ImageType.setValue(textureType);

    //    ui->groupBoxConvertToHeightSettings->setVisible(false);
    //    ui->groupBoxHN                 ->setVisible(false);
    ui->groupBoxNormalInputImage   ->setVisible(false);
    ui->groupBoxOcclusionInputImage->setVisible(false);
    ui->groupBoxRoughnessInputImage->setVisible(false);
    ui->groupBoxSpecularInputImage ->setVisible(false);
    ui->groupBoxNtoHConversion     ->setVisible(false);

    switch(textureType)
    {
    case(DIFFUSE_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        properties.RemoveShading.switchState(QtnPropertyStateInvisible,false);
        properties.EnableRemoveShading.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.BaseMapToOthers.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_d");
        break;
    case(NORMAL_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.NormalsStep.switchState(QtnPropertyStateInvisible,false);
        properties.GrungeOnImage.BlendingMode.switchState(QtnPropertyStateInvisible,true);
        properties.GrungeOnImage.ImageWeight.switchState(QtnPropertyStateInvisible,false);
        properties.NormalsMixer.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_n");
        ui->groupBoxNormalInputImage->setVisible(true);
        break;
    case(SPECULAR_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_s");
        ui->groupBoxSpecularInputImage->setVisible(true);
        break;
    case(HEIGHT_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_h");
        ui->groupBoxNtoHConversion->setVisible(true);
        break;
    case(OCCLUSION_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.AO.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_o");
        ui->groupBoxOcclusionInputImage->setVisible(true);
        break;
    case(ROUGHNESS_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        properties.RMFilter.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_r");
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(METALLIC_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        properties.RMFilter.switchState(QtnPropertyStateInvisible,false);
        properties.suffix.setValue("_m");
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(MATERIAL_TEXTURE):
        break;
    case(GRUNGE_TEXTURE):
        properties.Basic.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        properties.Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        properties.ColorLevels.switchState(QtnPropertyStateInvisible,false);
        properties.SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        properties.Grunge.switchState(QtnPropertyStateInvisible,false);
        break;
    default:
        break;
    }
}

void ImageWidget::reloadSettings()
{
    loadingImages = true;

    if(textureType == HEIGHT_TEXTURE)
    {
        ui->horizontalSliderNormalToHeightNoiseLevel    ->setValue(properties.NormalHeightConv.NoiseLevel);
        ui->horizontalSliderNormalToHeightItersHuge     ->setValue(properties.NormalHeightConv.Huge);
        ui->horizontalSliderNormalToHeightItersVeryLarge->setValue(properties.NormalHeightConv.VeryLarge);
        ui->horizontalSliderNormalToHeightItersLarge    ->setValue(properties.NormalHeightConv.Large);
        ui->horizontalSliderNormalToHeightItersMedium   ->setValue(properties.NormalHeightConv.Medium);
        ui->horizontalSliderNormalToHeightItersVerySmall->setValue(properties.NormalHeightConv.Small);
        ui->horizontalSliderNormalToHeightItersSmall    ->setValue(properties.NormalHeightConv.VerySmall);
    }
    // Input image case study
    switch(textureType)
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);
        switch(inputImageType)
        {
        case(INPUT_FROM_NORMAL_INPUT):
            ui->comboBoxNormalInputImage->setCurrentIndex(0);
            ui->pushButtonConverToNormal->setEnabled(true);
            break;
        case(INPUT_FROM_HEIGHT_INPUT):
            ui->comboBoxNormalInputImage->setCurrentIndex(1);
            break;
        case(INPUT_FROM_HEIGHT_OUTPUT):
            ui->comboBoxNormalInputImage->setCurrentIndex(2);
            break;
        default:
            break;
        }
        break;
        // End case NORMAL_TEXTURE.

    case(SPECULAR_TEXTURE):
        // Select propper input image for specular.
        switch(inputImageType)
        {
        case(INPUT_FROM_SPECULAR_INPUT):
            ui->comboBoxSpecularInputImage->setCurrentIndex(0);
            break;
        case(INPUT_FROM_DIFFUSE_INPUT):
            ui->comboBoxSpecularInputImage->setCurrentIndex(0);
            break;
        case(INPUT_FROM_DIFFUSE_OUTPUT):
            ui->comboBoxSpecularInputImage->setCurrentIndex(1);
            break;
        case(INPUT_FROM_HEIGHT_INPUT):
            ui->comboBoxSpecularInputImage->setCurrentIndex(2);
            break;
        case(INPUT_FROM_HEIGHT_OUTPUT):
            ui->comboBoxSpecularInputImage->setCurrentIndex(3);
            break;
        default:
            break;
        }
        break;
        // End case SPECULAR_TEXTURE.

    case(OCCLUSION_TEXTURE):
        // Select propper input image for occlusion.
        ui->pushButtonConvertOcclusionFromHN->setEnabled(false);
        switch(inputImageType)
        {
        case(INPUT_FROM_OCCLUSION_INPUT):
            ui->comboBoxOcclusionInputImage->setCurrentIndex(0);
            ui->pushButtonConvertOcclusionFromHN->setEnabled(true);
            break;
        case(INPUT_FROM_HI_NI):
            ui->comboBoxOcclusionInputImage->setCurrentIndex(1);
            break;
        case(INPUT_FROM_HO_NO):
            ui->comboBoxOcclusionInputImage->setCurrentIndex(2);
            break;
        default:
            break;
        }
        break;
        // End case OCCLUSION_TEXTURE.

    case(ROUGHNESS_TEXTURE):
        // Select propper input image for roughness.
        switch(inputImageType)
        {
        case(INPUT_FROM_ROUGHNESS_INPUT):
            ui->comboBoxRoughnessInputImage->setCurrentIndex(0);
            break;
        case(INPUT_FROM_DIFFUSE_INPUT):
            ui->comboBoxRoughnessInputImage->setCurrentIndex(1);
            break;
        case(INPUT_FROM_DIFFUSE_OUTPUT):
            ui->comboBoxRoughnessInputImage->setCurrentIndex(2);
            break;
        default:
            break;
        }
        break;
        // End case ROUGHNESS_TEXTURE.

    case(METALLIC_TEXTURE):
        // Select propper input image for roughness
        switch(inputImageType)
        {
        case(INPUT_FROM_METALLIC_INPUT):
            ui->comboBoxRoughnessInputImage->setCurrentIndex(0);
            break;
        case(INPUT_FROM_DIFFUSE_INPUT):
            ui->comboBoxRoughnessInputImage->setCurrentIndex(1);
            break;
        case(INPUT_FROM_DIFFUSE_OUTPUT):
            ui->comboBoxRoughnessInputImage->setCurrentIndex(2);
            break;
        default:
            break;
        }
        break;
        // End case METALLIC_TEXTURE.

    case(HEIGHT_TEXTURE):
        break;
        // End case HEIGHT_TEXTURE.

    default:
        break;
    };

    loadingImages = false;
}

bool ImageWidget::loadFile(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QImage qImage;
    QImageReader loadedImage(fileName);
    qImage = loadedImage.read();

    if (qImage.isNull())
    {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1.").arg(QDir::toNativeSeparators(fileName)));
        return false;
    }
    if(properties.NormalsMixer.EnableMixer)
    {
        qDebug() << "<FormImageProp> Open normal mixer image:" << fileName;
        openGL2DImageWidget->setNormalMixerInputTexture(qImage);

        emit imageChanged();
    }
    else
    {
        qDebug() << "<FormImageProp> Open image:" << fileName;

        imageName = fileInfo.baseName();
        (*recentDir).setPath(fileName);
        openGL2DImageWidget->setTextureImage(textureType, qImage);

        emit imageLoaded(qImage.width(), qImage.height());
        if(textureType == GRUNGE_TEXTURE)
            emit imageChanged();
    }
    return true;
}

void ImageWidget::saveFileToDir(const QString &dir)
{
    QString fullFileName = dir + "/" +
            imageName +
            properties.suffix +
            Image::outputFormat;
    saveFile(fullFileName);
}

void ImageWidget::saveImageToDir(const QString& dir, const QImage& qImage)
{
    QString fullFileName = dir + "/" +
            imageName +
            properties.suffix +
            Image::outputFormat;

    qDebug() << "<FormImageProp> save image:" << fullFileName;
    QFileInfo fileInfo(fullFileName);
    (*recentDir).setPath(fileInfo.absolutePath());

    qImage.save(fullFileName);
}

void ImageWidget::reloadImageSettings()
{
    emit reloadSettingsFromConfigFile(textureType);
}

void ImageWidget::open()
{
    QStringList picturesLocations;
    if(recentDir == NULL)
        picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    else
        picturesLocations << recentDir->absolutePath();

    QFileDialog dialog(
                this,
                tr("Open File"),
                picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.first(),
                tr("All Images (*.png *.jpg  *.tga *.jpeg *.bmp  *.tif);;"
                          "Images (*.png);;"
                          "Images (*.jpg);;"
                          "Images (*.tga);;"
                          "Images (*.jpeg);;"
                          "Images (*.bmp);;"
                          "Images (*.tif);;"
                          "All files (*.*)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted &&
           !loadFile(dialog.selectedFiles().first()))
    {}
}

void ImageWidget::save()
{
    QStringList picturesLocations;
    if(recentDir == NULL)
    {
        picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    }
    else
    {
        QFileInfo fileInfo(recentDir->absolutePath());
        QString fullFileName = fileInfo.absolutePath() + "/" +
                imageName +
                properties.suffix +
                Image::outputFormat;
        picturesLocations << fullFileName;
        qDebug() << "ImageWidget: Saving image to file:" << fullFileName;
    }

    QFileDialog dialog(
                this,
                tr("Save current image to file"),
                picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.first(),
                tr("All images (*.png *.jpg  *.tga *.jpeg *.bmp *.tif);;All files (*.*)"));
    dialog.setDirectory(recentDir->absolutePath());
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted &&
           !saveFile(dialog.selectedFiles().first()))
    {}
}

void ImageWidget::copyToClipboard()
{
    QApplication::processEvents();
    QImage qImage = openGL2DImageWidget->getTextureFBOImage(textureType);
    QApplication::clipboard()->setImage(qImage, QClipboard::Clipboard);
}

void ImageWidget::pasteFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage qImage = pixmap.toImage();
        pasteImageFromClipboard(qImage);
    }
}

void ImageWidget::pasteNormalFromClipBoard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage qImage = pixmap.toImage();

        openGL2DImageWidget->setNormalMixerInputTexture(qImage);

        emit imageChanged();
    }
}

void ImageWidget::pasteNormalFromClipBoard(const QtnPropertyButton*)
{
    emit pasteNormalFromClipBoard();
}

void ImageWidget::updateComboBoxes(int)
{
    // Input image case study.
    switch(textureType)
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);

        switch(ui->comboBoxNormalInputImage->currentIndex())
        {
        case(0):
            inputImageType = INPUT_FROM_NORMAL_INPUT;
            ui->pushButtonConverToNormal->setEnabled(true);
            break;
        case(1):
            inputImageType = INPUT_FROM_HEIGHT_INPUT;
            break;
        case(2):
            inputImageType = INPUT_FROM_HEIGHT_OUTPUT;
            break;
        }
        break;
        // End case NORMAL_TEXTURE.

    case(SPECULAR_TEXTURE):
        // Select propper input image for specular.
        switch(ui->comboBoxSpecularInputImage->currentIndex())
        {
        case(0):
            inputImageType = INPUT_FROM_SPECULAR_INPUT;
            break;
        case(1):
            inputImageType = INPUT_FROM_DIFFUSE_INPUT;
            break;
        case(2):
            inputImageType = INPUT_FROM_DIFFUSE_OUTPUT;
            break;
        case(3):
            inputImageType = INPUT_FROM_HEIGHT_INPUT;
            break;
        case(4):
            inputImageType = INPUT_FROM_HEIGHT_OUTPUT;
            break;
        }
        break;
        // End case SPECULAR_TEXTURE.

    case(OCCLUSION_TEXTURE):
        // Select propper input image for occlusion.
        ui->pushButtonConvertOcclusionFromHN->setEnabled(false);
        switch(ui->comboBoxOcclusionInputImage->currentIndex())
        {
        case(0):
            inputImageType = INPUT_FROM_OCCLUSION_INPUT;
            ui->pushButtonConvertOcclusionFromHN->setEnabled(true);
            break;
        case(1):
            inputImageType = INPUT_FROM_HI_NI;
            break;
        case(2):
            inputImageType = INPUT_FROM_HO_NO;
            break;
        }
        break;
        // End case OCCLUSION_TEXTURE.

    case(ROUGHNESS_TEXTURE):
        // Select propper input image for roughness.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            inputImageType = INPUT_FROM_ROUGHNESS_INPUT;
            break;
        case(1):
            inputImageType = INPUT_FROM_DIFFUSE_INPUT;
            break;
        case(2):
            inputImageType = INPUT_FROM_DIFFUSE_OUTPUT;
            break;
        }
        break;
        // End case ROUGHNESS_TEXTURE.

    case(METALLIC_TEXTURE):
        // Select propper input image for metallic.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            inputImageType = INPUT_FROM_METALLIC_INPUT;
            break;
        case(1):
            inputImageType = INPUT_FROM_DIFFUSE_INPUT;
            break;
        case(2):
            inputImageType = INPUT_FROM_DIFFUSE_OUTPUT;
            break;
        }
        break;
        // End case METALLIC_TEXTURE.

    case(HEIGHT_TEXTURE):
        break;
        // End case HEIGHT_TEXTURE.

    default:
        break;
    };
    emit imageChanged();
}

void ImageWidget::updateGuiSpinBoxesAndLabes(int)
{
    if(loadingImages == true) return;

    ui->doubleSpinBoxConversionHNDepth->setValue(ui->horizontalSliderConversionHNDepth->value()/5.0);

    openGL2DImageWidget->setConversionHNDepth(ui->doubleSpinBoxConversionHNDepth->value());

    properties.NormalHeightConv.NoiseLevel = ui->horizontalSliderNormalToHeightNoiseLevel     ->value();
    properties.NormalHeightConv.Huge       = ui->horizontalSliderNormalToHeightItersHuge      ->value();
    properties.NormalHeightConv.VeryLarge  = ui->horizontalSliderNormalToHeightItersVeryLarge ->value();
    properties.NormalHeightConv.Large      = ui->horizontalSliderNormalToHeightItersLarge     ->value();
    properties.NormalHeightConv.Medium     = ui->horizontalSliderNormalToHeightItersMedium    ->value();
    properties.NormalHeightConv.Small      = ui->horizontalSliderNormalToHeightItersSmall     ->value();
    properties.NormalHeightConv.VerySmall  = ui->horizontalSliderNormalToHeightItersVerySmall ->value();
}

void ImageWidget::updateSlidersOnRelease()
{
    if(loadingImages == true) return;
    updateGuiSpinBoxesAndLabes(0);
    emit imageChanged();
}

void ImageWidget::propertyChanged(const QtnPropertyBase* changedProperty, const QtnPropertyBase*,
                                  QtnPropertyChangeReason reason)
{
    if (loadingImages) return;
    if (reason & QtnPropertyChangeReasonValue)
    {
        // Grunge Load predefined pattern
        if(dynamic_cast<const QtnPropertyQString*>(changedProperty)
                == &properties.Grunge.Patterns)
        {
            loadPredefinedGrunge(properties.Grunge.Patterns.value());
        }
        // Enable Grunge.
        if(properties.Grunge.OverallWeight.value() == 0)
        {
            emit  toggleGrungeSettings(false);
        }
        else
        {
            emit  toggleGrungeSettings(true);
        }
        // Open Normal mixer.
        if(dynamic_cast<const QtnPropertyQString*>(changedProperty)
                == &properties.NormalsMixer.NormalImage)
        {
            loadFile(properties.NormalsMixer.NormalImage);
        }
        // Bool activated, enum changed.
        if(dynamic_cast<const QtnPropertyBool*>(changedProperty)
                || dynamic_cast<const QtnPropertyEnum*>(changedProperty))
        {
            // Enable BaseMapToOthers Conversion Tool Height Preview.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &properties.BaseMapToOthers.EnableHeightPreview)
            {
                Image::bConversionBaseMapShowHeightTexture = properties.BaseMapToOthers.EnableHeightPreview;
            }
            emit imageChanged();
        }

        // Pick min or max color from diffuse image
        //if(dynamic_cast<const QtnPropertyQString*>(changedProperty) == &imageProp.getProperties()->BaseMapToOthers.MaxColor
        //|| dynamic_cast<const QtnPropertyQString*>(changedProperty) == &imageProp.getProperties()->BaseMapToOthers.MinColor ){
        //            pickColorFromImage(changedProperty);
        //        }

        emit imageChanged();
    } // End of if reason value
}

void ImageWidget::propertyFinishedEditing()
{
    emit imageChanged();
}

void ImageWidget::applyBaseConversion(const QtnPropertyButton*)
{
    emit conversionBaseConversionApplied();
}

void ImageWidget::applyHeightToNormalConversion()
{
    emit conversionHeightToNormalApplied();
}

void ImageWidget::applyNormalToHeightConversion()
{
    emit conversionNormalToHeightApplied();
}

void ImageWidget::applyBaseConversionConversion()
{
    emit conversionBaseConversionApplied();
}

void ImageWidget::applyHeightNormalToOcclusionConversion()
{
    emit conversionHeightNormalToOcclusionApplied();
}

void ImageWidget::showHeightCalculatorDialog()
{
    heightCalculator->setImageSize(openGL2DImageWidget->getTextureWidth(textureType),
                                   openGL2DImageWidget->getTextureHeight(textureType));
    unsigned int result = heightCalculator->exec();
    if(result == QDialog::Accepted)
    {
        ui->horizontalSliderConversionHNDepth->setValue(heightCalculator->getDepthInPixels()*5);
        updateSlidersOnRelease();
        qDebug() << "Height map::Depth calculated:" << heightCalculator->getDepthInPixels();
    }
}

void ImageWidget::pickColorFromImage( QtnPropertyABColor* property)
{
    // Some customizations here
    emit pickImageColor(property);
}

void ImageWidget::toggleGrungeImageSettingsGroup(bool toggle)
{
    properties.GrungeOnImage.switchState(QtnPropertyStateInvisible,!toggle);
}

void ImageWidget::loadPredefinedGrunge(QString image)
{
    loadFile(":/resources/grunge/" + image);
}

bool ImageWidget::saveFile(const QString &fileName)
{
    qDebug() << Q_FUNC_INFO << "image:" << fileName;

    QFileInfo fileInfo(fileName);
    (*recentDir).setPath(fileInfo.absolutePath());
    QImage qImage = openGL2DImageWidget->getTextureFBOImage(textureType);

    qImage.save(fileName);

    return true;
}

void ImageWidget::pasteImageFromClipboard(const QImage& qImage)
{
    imageName = "clipboard_image";
    openGL2DImageWidget->setTextureImage(textureType, qImage);
    emit imageLoaded(qImage.width(), qImage.height());
    if(textureType == GRUNGE_TEXTURE)
        emit imageChanged();
}
