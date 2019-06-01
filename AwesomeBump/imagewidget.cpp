#include "imagewidget.h"
#include "ui_imagewidget.h"

#include <QDebug>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>

bool ImageWidget::loadingImages = false;
QDir* ImageWidget::recentDir;

ImageWidget::ImageWidget(QWidget *parent, OpenGL2DImageWidget *openGL2DImageWidget, TextureType textureType) :
    QWidget(parent),
    image(openGL2DImageWidget),
    ui(new Ui::ImageWidget)
{
    ui->setupUi(this);
    ui->widgetProperty->setParts(QtnPropertyWidgetPartsDescriptionPanel);
    ui->widgetProperty->setPropertySet(image.getProperties());
    ui->widgetProperty->layout()->setMargin(0);
    ui->widgetProperty->layout()->setSpacing(0);

    connect(image.getProperties(),
            SIGNAL (propertyDidChange(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)), this,
            SLOT (propertyChanged(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)));

    connect(&image.getProperties()->BaseMapToOthers.Convert,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (applyBaseConversion(const QtnPropertyButton*)));

    connect(&image.getProperties()->NormalsMixer.PasteFromClipboard,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (pasteNormalFromClipBoard(const QtnPropertyButton*)));


    connect(&image.getProperties()->BaseMapToOthers.MinColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));

    connect(&image.getProperties()->BaseMapToOthers.MaxColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));


    connect(&image.getProperties()->RMFilter.ColorFilter.PickColor,
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

    image.setTextureType(textureType);
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

void ImageWidget::setImage(const QImage& qImage)
{
    image.setImage(qImage);
}

QString ImageWidget::getImageName()
{
    return image.getImageName();
}

void ImageWidget::setImageName(const QString& name)
{
    image.setImageName(name);
}

void ImageWidget::setupPropertiesGUI()
{
    image.getProperties()->ImageType.setValue((image.getTextureType()));

    //    ui->groupBoxConvertToHeightSettings->setVisible(false);
    //    ui->groupBoxHN                 ->setVisible(false);
    ui->groupBoxNormalInputImage   ->setVisible(false);
    ui->groupBoxOcclusionInputImage->setVisible(false);
    ui->groupBoxRoughnessInputImage->setVisible(false);
    ui->groupBoxSpecularInputImage ->setVisible(false);
    ui->groupBoxNtoHConversion     ->setVisible(false);

    switch(image.getTextureType())
    {
    case(DIFFUSE_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->RemoveShading.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->EnableRemoveShading.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->BaseMapToOthers.switchState(QtnPropertyStateInvisible,false);
        break;
    case(NORMAL_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.NormalsStep.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->GrungeOnImage.BlendingMode.switchState(QtnPropertyStateInvisible,true);
        image.getProperties()->GrungeOnImage.ImageWeight.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->NormalsMixer.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxNormalInputImage->setVisible(true);
        break;
    case(SPECULAR_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxSpecularInputImage->setVisible(true);
        break;
    case(HEIGHT_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxNtoHConversion->setVisible(true);
        break;
    case(OCCLUSION_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->AO.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxOcclusionInputImage->setVisible(true);
        break;
    case(ROUGHNESS_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->RMFilter.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(METALLIC_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->RMFilter.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(MATERIAL_TEXTURE):
        break;
    case(GRUNGE_TEXTURE):
        image.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        image.getProperties()->Grunge.switchState(QtnPropertyStateInvisible,false);
        break;
    default:
        break;
    }
}

void ImageWidget::reloadSettings()
{
    loadingImages = true;

    if(image.getTextureType() == HEIGHT_TEXTURE)
    {
        ui->horizontalSliderNormalToHeightNoiseLevel    ->setValue(image.getProperties()->NormalHeightConv.NoiseLevel);
        ui->horizontalSliderNormalToHeightItersHuge     ->setValue(image.getProperties()->NormalHeightConv.Huge);
        ui->horizontalSliderNormalToHeightItersVeryLarge->setValue(image.getProperties()->NormalHeightConv.VeryLarge);
        ui->horizontalSliderNormalToHeightItersLarge    ->setValue(image.getProperties()->NormalHeightConv.Large);
        ui->horizontalSliderNormalToHeightItersMedium   ->setValue(image.getProperties()->NormalHeightConv.Medium);
        ui->horizontalSliderNormalToHeightItersVerySmall->setValue(image.getProperties()->NormalHeightConv.Small);
        ui->horizontalSliderNormalToHeightItersSmall    ->setValue(image.getProperties()->NormalHeightConv.VerySmall);
    }
    // Input image case study
    switch(image.getTextureType())
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);
        switch(image.getInputImageType())
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
        switch(image.getInputImageType())
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
        switch(image.getInputImageType())
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
        switch(image.getInputImageType())
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
        switch(image.getInputImageType())
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
    if(image.getProperties()->NormalsMixer.EnableMixer)
    {
        qDebug() << "<FormImageProp> Open normal mixer image:" << fileName;
        image.setNormalMixerInputTexture(qImage);

        emit imageChanged();
    }
    else
    {
        qDebug() << "<FormImageProp> Open image:" << fileName;

        image.getImageName() = fileInfo.baseName();
        (*recentDir).setPath(fileName);
        image.setImage(qImage);

        emit imageLoaded(qImage.width(), qImage.height());
        if(image.getTextureType() == GRUNGE_TEXTURE)
            emit imageChanged();
    }
    return true;
}

void ImageWidget::saveFileToDir(const QString &dir)
{
    QString fullFileName = dir + "/" +
            image.getImageName() +
            image.getTextureSuffix() +
            Image::outputFormat;
    saveFile(fullFileName);
}

void ImageWidget::saveImageToDir(const QString& dir, const QImage& qImage)
{
    QString fullFileName = dir + "/" +
            image.getImageName() +
            image.getTextureSuffix() +
            Image::outputFormat;

    qDebug() << "<FormImageProp> save image:" << fullFileName;
    QFileInfo fileInfo(fullFileName);
    (*recentDir).setPath(fileInfo.absolutePath());

    qImage.save(fullFileName);
}

void ImageWidget::reloadImageSettings()
{
    emit reloadSettingsFromConfigFile(image.getTextureType());
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
                image.getImageName() +
                image.getTextureSuffix() +
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
    qDebug() << "<FormImageProp> Image :" +
                image.getTextureName() +
                " copied to clipboard.";

    QApplication::processEvents();
    QImage qImage = image.getFBOImage();
    QApplication::clipboard()->setImage(qImage, QClipboard::Clipboard);
}

void ImageWidget::pasteFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        qDebug() << "<FormImageProp> Image :" +
                    image.getTextureName() +
                    " loaded from clipboard.";
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
        qDebug() << "<FormImageProp> Normal image :" +
                    image.getTextureName() +
                    " loaded from clipboard.";
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage qImage = pixmap.toImage();

        //imageProp.getOpenGLWidget()->makeCurrent();
        //        if(glIsTexture(imageProp.normalMixerInputTexId))
        //            imageProp.glWidget_ptr->deleteTexture(imageProp.normalMixerInputTexId);
        //        imageProp.normalMixerInputTexId = imageProp.glWidget_ptr->bindTexture(_image,GL_TEXTURE_2D);
        image.setNormalMixerInputTexture(qImage);

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
    switch(image.getTextureType())
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);

        switch(ui->comboBoxNormalInputImage->currentIndex())
        {
        case(0):
            image.setInputImageType(INPUT_FROM_NORMAL_INPUT);
            ui->pushButtonConverToNormal->setEnabled(true);
            break;
        case(1):
            image.setInputImageType(INPUT_FROM_HEIGHT_INPUT);
            break;
        case(2):
            image.setInputImageType(INPUT_FROM_HEIGHT_OUTPUT);
            break;
        }
        break;
        // End case NORMAL_TEXTURE.

    case(SPECULAR_TEXTURE):
        // Select propper input image for specular.
        switch(ui->comboBoxSpecularInputImage->currentIndex())
        {
        case(0):
            image.setInputImageType(INPUT_FROM_SPECULAR_INPUT);
            break;
        case(1):
            image.setInputImageType(INPUT_FROM_DIFFUSE_INPUT);
            break;
        case(2):
            image.setInputImageType(INPUT_FROM_DIFFUSE_OUTPUT);
            break;
        case(3):
            image.setInputImageType(INPUT_FROM_HEIGHT_INPUT);
            break;
        case(4):
            image.setInputImageType(INPUT_FROM_HEIGHT_OUTPUT);
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
            image.setInputImageType(INPUT_FROM_OCCLUSION_INPUT);
            ui->pushButtonConvertOcclusionFromHN->setEnabled(true);
            break;
        case(1):
            image.setInputImageType(INPUT_FROM_HI_NI);
            break;
        case(2):
            image.setInputImageType(INPUT_FROM_HO_NO);
            break;
        }
        break;
        // End case OCCLUSION_TEXTURE.

    case(ROUGHNESS_TEXTURE):
        // Select propper input image for roughness.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            image.setInputImageType(INPUT_FROM_ROUGHNESS_INPUT);
            break;
        case(1):
            image.setInputImageType(INPUT_FROM_DIFFUSE_INPUT);
            break;
        case(2):
            image.setInputImageType(INPUT_FROM_DIFFUSE_OUTPUT);
            break;
        }
        break;
        // End case ROUGHNESS_TEXTURE.

    case(METALLIC_TEXTURE):
        // Select propper input image for metallic.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            image.setInputImageType(INPUT_FROM_METALLIC_INPUT);
            break;
        case(1):
            image.setInputImageType(INPUT_FROM_DIFFUSE_INPUT);
            break;
        case(2):
            image.setInputImageType(INPUT_FROM_DIFFUSE_OUTPUT);
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

    image.setConversionHNDepth(ui->doubleSpinBoxConversionHNDepth->value());

    image.getProperties()->NormalHeightConv.NoiseLevel = ui->horizontalSliderNormalToHeightNoiseLevel     ->value();
    image.getProperties()->NormalHeightConv.Huge       = ui->horizontalSliderNormalToHeightItersHuge      ->value();
    image.getProperties()->NormalHeightConv.VeryLarge  = ui->horizontalSliderNormalToHeightItersVeryLarge ->value();
    image.getProperties()->NormalHeightConv.Large      = ui->horizontalSliderNormalToHeightItersLarge     ->value();
    image.getProperties()->NormalHeightConv.Medium     = ui->horizontalSliderNormalToHeightItersMedium    ->value();
    image.getProperties()->NormalHeightConv.Small      = ui->horizontalSliderNormalToHeightItersSmall     ->value();
    image.getProperties()->NormalHeightConv.VerySmall  = ui->horizontalSliderNormalToHeightItersVerySmall ->value();
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
                == &image.getProperties()->Grunge.Patterns)
        {
            loadPredefinedGrunge(image.getProperties()->Grunge.Patterns.value());
        }
        // Enable Grunge.
        if(image.getProperties()->Grunge.OverallWeight.value() == 0)
        {
            emit  toggleGrungeSettings(false);
        }
        else
        {
            emit  toggleGrungeSettings(true);
        }
        // Open Normal mixer.
        if(dynamic_cast<const QtnPropertyQString*>(changedProperty)
                == &image.getProperties()->NormalsMixer.NormalImage)
        {
            loadFile(image.getProperties()->NormalsMixer.NormalImage);
        }
        // Bool activated, enum changed.
        if(dynamic_cast<const QtnPropertyBool*>(changedProperty)
                || dynamic_cast<const QtnPropertyEnum*>(changedProperty))
        {
            // Enable BaseMapToOthers Conversion Tool.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &image.getProperties()->BaseMapToOthers.EnableConversion)
            {
                Image::bConversionBaseMap = image.getProperties()->BaseMapToOthers.EnableConversion;
            }
            // Enable BaseMapToOthers Conversion Tool Height Preview.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &image.getProperties()->BaseMapToOthers.EnableHeightPreview)
            {
                Image::bConversionBaseMapShowHeightTexture = image.getProperties()->BaseMapToOthers.EnableHeightPreview;
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
    heightCalculator->setImageSize(image.width(), image.height());
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
    image.getProperties()->GrungeOnImage.switchState(QtnPropertyStateInvisible,!toggle);
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
    QImage qImage = image.getFBOImage();

    qImage.save(fileName);

    return true;
}

void ImageWidget::pasteImageFromClipboard(const QImage& qImage)
{
    image.setImageName("clipboard_image");
    image.setImage(qImage);
    emit imageLoaded(qImage.width(), qImage.height());
    if(image.getTextureType() == GRUNGE_TEXTURE)
        emit imageChanged();
}
