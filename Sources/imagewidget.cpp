#include "imagewidget.h"
#include "ui_imagewidget.h"

#include <QDebug>
#include <QFileInfo>
#include <QImageReader>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>

#include "targaimage.h"

bool ImageWidget::loadingImages = false;

ImageWidget::ImageWidget(QWidget *parent, OpenGLImageEditor *openGLWidget, TextureType textureType) :
    ImageBaseWidget(parent),
    ui(new Ui::ImageWidget)
{
    ui->setupUi(this);
    ui->widgetProperty->setParts(QtnPropertyWidgetPartsDescriptionPanel);
    ui->widgetProperty->setPropertySet(imageProp.properties);
    ui->widgetProperty->layout()->setMargin(0);
    ui->widgetProperty->layout()->setSpacing(0);

    connect(imageProp.properties,
            SIGNAL (propertyDidChange(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)), this,
            SLOT (propertyChanged(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)));
    //connect(imageProp.properties,SIGNAL(propertyDidFinishEditing()),this,SLOT(propertyFinishedEditing()));

    QObject::connect(&imageProp.properties->BaseMapToOthers.Convert,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (applyBaseConversion(const QtnPropertyButton*)));

    QObject::connect(&imageProp.properties->NormalsMixer.PasteFromClipboard,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (pasteNormalFromClipBoard(const QtnPropertyButton*)));


    QObject::connect(&imageProp.properties->BaseMapToOthers.MinColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));

    QObject::connect(&imageProp.properties->BaseMapToOthers.MaxColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));


    QObject::connect(&imageProp.properties->RMFilter.ColorFilter.PickColor,
                     SIGNAL(click( QtnPropertyABColor*)), this,
                     SLOT(pickColorFromImage( QtnPropertyABColor*)));

    ui->groupBoxConvertToHeightSettings->hide();

    imageProp.openGLWidget = openGLWidget;
    
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

    getImage()->textureType = textureType;
    setupPropertiesGUI();
    setImageName("image");
}

ImageWidget::~ImageWidget()
{
    qDebug() << "calling" << Q_FUNC_INFO;
    delete heightCalculator;
    //delete imageProp.properties;
    delete ui;
}

void ImageWidget::setImage(QImage newImage)
{
    image = newImage;
    if (imageProp.openGLWidget->isValid())
        imageProp.init(image);
    else
        qDebug() << Q_FUNC_INFO << "Invalid context.";
}

void ImageWidget::setOpenGLWidget(QOpenGLWidget *openGLWidget)
{
    imageProp.openGLWidget = openGLWidget;
}

void ImageWidget::setupPropertiesGUI()
{
    imageProp.properties->ImageType.setValue((imageProp.textureType));

    //    ui->groupBoxConvertToHeightSettings->setVisible(false);
    //    ui->groupBoxHN                 ->setVisible(false);
    ui->groupBoxNormalInputImage   ->setVisible(false);
    ui->groupBoxOcclusionInputImage->setVisible(false);
    ui->groupBoxRoughnessInputImage->setVisible(false);
    ui->groupBoxSpecularInputImage ->setVisible(false);
    ui->groupBoxNtoHConversion     ->setVisible(false);

    switch(imageProp.textureType)
    {
    case(DIFFUSE_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->RemoveShading.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->EnableRemoveShading.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->BaseMapToOthers.switchState(QtnPropertyStateInvisible,false);
        break;
    case(NORMAL_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.NormalsStep.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->GrungeOnImage.BlendingMode.switchState(QtnPropertyStateInvisible,true);
        imageProp.properties->GrungeOnImage.ImageWeight.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->NormalsMixer.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxNormalInputImage->setVisible(true);
        break;
    case(SPECULAR_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxSpecularInputImage->setVisible(true);
        break;
    case(HEIGHT_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxNtoHConversion->setVisible(true);
        break;
    case(OCCLUSION_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->AO.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxOcclusionInputImage->setVisible(true);
        break;
    case(ROUGHNESS_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->RMFilter.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(METALLIC_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->RMFilter.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(MATERIAL_TEXTURE):
        break;
    case(GRUNGE_TEXTURE):
        imageProp.properties->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        imageProp.properties->Grunge.switchState(QtnPropertyStateInvisible,false);
        break;
    default:
        break;
    }
}

void ImageWidget::reloadSettings()
{
    loadingImages = true;

    if(imageProp.textureType == HEIGHT_TEXTURE)
    {
        ui->horizontalSliderNormalToHeightNoiseLevel    ->setValue(imageProp.properties->NormalHeightConv.NoiseLevel);
        ui->horizontalSliderNormalToHeightItersHuge     ->setValue(imageProp.properties->NormalHeightConv.Huge);
        ui->horizontalSliderNormalToHeightItersVeryLarge->setValue(imageProp.properties->NormalHeightConv.VeryLarge);
        ui->horizontalSliderNormalToHeightItersLarge    ->setValue(imageProp.properties->NormalHeightConv.Large);
        ui->horizontalSliderNormalToHeightItersMedium   ->setValue(imageProp.properties->NormalHeightConv.Medium);
        ui->horizontalSliderNormalToHeightItersVerySmall->setValue(imageProp.properties->NormalHeightConv.Small);
        ui->horizontalSliderNormalToHeightItersSmall    ->setValue(imageProp.properties->NormalHeightConv.VerySmall);
    }
    // Input image case study
    switch(imageProp.textureType)
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);
        switch(imageProp.inputImageType)
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
        switch(imageProp.inputImageType)
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
        switch(imageProp.inputImageType)
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
        switch(imageProp.inputImageType)
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
        switch(imageProp.inputImageType)
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
    QImage _image;
    //    qDebug() << "Opening file: " << fileName;
    // Targa support added
    if(fileInfo.completeSuffix().compare("tga") == 0)
    {
        TargaImage tgaImage;
        _image = tgaImage.read(fileName);
    }
    else
    {
        QImageReader loadedImage(fileName);
        _image = loadedImage.read();
    }

    if (_image.isNull())
    {
        QMessageBox::information(this, QGuiApplication::applicationDisplayName(),
                                 tr("Cannot load %1.").arg(QDir::toNativeSeparators(fileName)));
        return false;
    }
    if(imageProp.properties->NormalsMixer.EnableMixer)
    {
        qDebug() << "<FormImageProp> Open normal mixer image:" << fileName;

        imageProp.openGLWidget->makeCurrent();
        //if(glIsTexture(imageProp.normalMixerInputTexId))
        //    imageProp.glWidget_ptr->deleteTexture(imageProp.normalMixerInputTexId);
        //    imageProp.normalMixerInputTexId = imageProp.glWidget_ptr->bindTexture(_image,GL_TEXTURE_2D);
        if(imageProp.normalMixerInputTexture)
            delete imageProp.normalMixerInputTexture;
        imageProp.normalMixerInputTexture = new QOpenGLTexture(_image);
        imageProp.normalMixerInputTexture->bind();

        emit imageChanged();

    }
    else
    {
        qDebug() << "<FormImageProp> Open image:" << fileName;

        imageName = fileInfo.baseName();
        (*recentDir).setPath(fileName);
        image    = _image;
        imageProp.init(image);

        //emit imageChanged();
        emit imageLoaded(image.width(),image.height());
        if(imageProp.textureType == GRUNGE_TEXTURE)emit imageChanged();
    }
    return true;
}

void ImageWidget::reloadImageSettings()
{
    emit reloadSettingsFromConfigFile(imageProp.textureType);
}

void ImageWidget::copyToClipboard()
{
    qDebug() << "<FormImageProp> Image :" +
                PostfixNames::getTextureName(imageProp.textureType) +
                " copied to clipboard.";

    QApplication::processEvents();
    image = imageProp.getFBOImage();
    QApplication::clipboard()->setImage(image,QClipboard::Clipboard);
}

void ImageWidget::pasteFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        qDebug() << "<FormImageProp> Image :" +
                    PostfixNames::getTextureName(imageProp.textureType)+
                    " loaded from clipboard.";
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage image = pixmap.toImage();
        pasteImageFromClipboard(image);
    }
}

void ImageWidget::pasteNormalFromClipBoard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        qDebug() << "<FormImageProp> Normal image :" +
                    PostfixNames::getTextureName(imageProp.textureType) +
                    " loaded from clipboard.";
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage _image = pixmap.toImage();

        imageProp.openGLWidget->makeCurrent();
        //        if(glIsTexture(imageProp.normalMixerInputTexId))
        //            imageProp.glWidget_ptr->deleteTexture(imageProp.normalMixerInputTexId);
        //        imageProp.normalMixerInputTexId = imageProp.glWidget_ptr->bindTexture(_image,GL_TEXTURE_2D);
        if(imageProp.normalMixerInputTexture)
            delete imageProp.normalMixerInputTexture;
        imageProp.normalMixerInputTexture = new QOpenGLTexture(_image);
        imageProp.normalMixerInputTexture->bind();

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
    switch(imageProp.textureType)
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);

        switch(ui->comboBoxNormalInputImage->currentIndex())
        {
        case(0):
            imageProp.inputImageType = INPUT_FROM_NORMAL_INPUT;
            ui->pushButtonConverToNormal->setEnabled(true);
            break;
        case(1):
            imageProp.inputImageType = INPUT_FROM_HEIGHT_INPUT;
            break;
        case(2):
            imageProp.inputImageType = INPUT_FROM_HEIGHT_OUTPUT;
            break;
        }
        break;
        // End case NORMAL_TEXTURE.

    case(SPECULAR_TEXTURE):
        // Select propper input image for specular.
        switch(ui->comboBoxSpecularInputImage->currentIndex())
        {
        case(0):
            imageProp.inputImageType = INPUT_FROM_SPECULAR_INPUT;
            break;
        case(1):
            imageProp.inputImageType = INPUT_FROM_DIFFUSE_INPUT;
            break;
        case(2):
            imageProp.inputImageType = INPUT_FROM_DIFFUSE_OUTPUT;
            break;
        case(3):
            imageProp.inputImageType = INPUT_FROM_HEIGHT_INPUT;
            break;
        case(4):
            imageProp.inputImageType = INPUT_FROM_HEIGHT_OUTPUT;
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
            imageProp.inputImageType = INPUT_FROM_OCCLUSION_INPUT;
            ui->pushButtonConvertOcclusionFromHN->setEnabled(true);
            break;
        case(1):
            imageProp.inputImageType = INPUT_FROM_HI_NI;
            break;
        case(2):
            imageProp.inputImageType = INPUT_FROM_HO_NO;
            break;
        }
        break;
        // End case OCCLUSION_TEXTURE.

    case(ROUGHNESS_TEXTURE):
        // Select propper input image for roughness.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            imageProp.inputImageType = INPUT_FROM_ROUGHNESS_INPUT;
            break;
        case(1):
            imageProp.inputImageType = INPUT_FROM_DIFFUSE_INPUT;
            break;
        case(2):
            imageProp.inputImageType = INPUT_FROM_DIFFUSE_OUTPUT;
            break;
        }
        break;
        // End case ROUGHNESS_TEXTURE.

    case(METALLIC_TEXTURE):
        // Select propper input image for metallic.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            imageProp.inputImageType = INPUT_FROM_METALLIC_INPUT;
            break;
        case(1):
            imageProp.inputImageType = INPUT_FROM_DIFFUSE_INPUT;
            break;
        case(2):
            imageProp.inputImageType = INPUT_FROM_DIFFUSE_OUTPUT;
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

    imageProp.conversionHNDepth = ui->doubleSpinBoxConversionHNDepth->value();

    imageProp.properties->NormalHeightConv.NoiseLevel = ui->horizontalSliderNormalToHeightNoiseLevel     ->value();
    imageProp.properties->NormalHeightConv.Huge       = ui->horizontalSliderNormalToHeightItersHuge      ->value();
    imageProp.properties->NormalHeightConv.VeryLarge  = ui->horizontalSliderNormalToHeightItersVeryLarge ->value();
    imageProp.properties->NormalHeightConv.Large      = ui->horizontalSliderNormalToHeightItersLarge     ->value();
    imageProp.properties->NormalHeightConv.Medium     = ui->horizontalSliderNormalToHeightItersMedium    ->value();
    imageProp.properties->NormalHeightConv.Small      = ui->horizontalSliderNormalToHeightItersSmall     ->value();
    imageProp.properties->NormalHeightConv.VerySmall  = ui->horizontalSliderNormalToHeightItersVerySmall ->value();
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
                == &imageProp.properties->Grunge.Patterns)
        {
            loadPredefinedGrunge(imageProp.properties->Grunge.Patterns.value());
        }
        // Enable Grunge.
        if(imageProp.properties->Grunge.OverallWeight.value() == 0)
        {
            emit  toggleGrungeSettings(false);
        }
        else
        {
            emit  toggleGrungeSettings(true);
        }
        // Open Normal mixer.
        if(dynamic_cast<const QtnPropertyQString*>(changedProperty)
                == &imageProp.properties->NormalsMixer.NormalImage)
        {
            loadFile(imageProp.properties->NormalsMixer.NormalImage);
        }
        // Bool activated, enum changed.
        if(dynamic_cast<const QtnPropertyBool*>(changedProperty)
                || dynamic_cast<const QtnPropertyEnum*>(changedProperty))
        {
            // Enable BaseMapToOthers Conversion Tool.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &imageProp.properties->BaseMapToOthers.EnableConversion)
            {
                Image::bConversionBaseMap = imageProp.properties->BaseMapToOthers.EnableConversion;
            }
            // Enable BaseMapToOthers Conversion Tool Height Preview.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &imageProp.properties->BaseMapToOthers.EnableHeightPreview)
            {
                Image::bConversionBaseMapShowHeightTexture = imageProp.properties->BaseMapToOthers.EnableHeightPreview;
            }
            emit imageChanged();
        }

        // Pick min or max color from diffuse image
        //if(dynamic_cast<const QtnPropertyQString*>(changedProperty) == &imageProp.properties->BaseMapToOthers.MaxColor
        //|| dynamic_cast<const QtnPropertyQString*>(changedProperty) == &imageProp.properties->BaseMapToOthers.MinColor ){
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
    //heightCalculator->setImageSize(imageProp.ref_fbo->width(),imageProp.ref_fbo->height());
    heightCalculator->setImageSize(imageProp.fbo->width(),imageProp.fbo->height());
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
    imageProp.properties->GrungeOnImage.switchState(QtnPropertyStateInvisible,!toggle);
}

void ImageWidget::loadPredefinedGrunge(QString image)
{
    loadFile(QString(RESOURCE_BASE) + "Core/2D/grunge/" + image);
}

void ImageWidget::pasteImageFromClipboard(QImage& _image)
{
    imageName = "clipboard_image";
    image     = _image;
    imageProp.init(image);
    emit imageLoaded(image.width(),image.height());
    if(imageProp.textureType == GRUNGE_TEXTURE)emit imageChanged();
}
