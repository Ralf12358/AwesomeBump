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
    ui->widgetProperty->setPropertySet(imageProp.getProperties());
    ui->widgetProperty->layout()->setMargin(0);
    ui->widgetProperty->layout()->setSpacing(0);

    connect(imageProp.getProperties(),
            SIGNAL (propertyDidChange(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)), this,
            SLOT (propertyChanged(const QtnPropertyBase*,const QtnPropertyBase*,QtnPropertyChangeReason)));
    //connect(imageProp.getProperties(),SIGNAL(propertyDidFinishEditing()),this,SLOT(propertyFinishedEditing()));

    QObject::connect(&imageProp.getProperties()->BaseMapToOthers.Convert,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (applyBaseConversion(const QtnPropertyButton*)));

    QObject::connect(&imageProp.getProperties()->NormalsMixer.PasteFromClipboard,
                     SIGNAL (click(const QtnPropertyButton*)), this,
                     SLOT (pasteNormalFromClipBoard(const QtnPropertyButton*)));


    QObject::connect(&imageProp.getProperties()->BaseMapToOthers.MinColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));

    QObject::connect(&imageProp.getProperties()->BaseMapToOthers.MaxColor,
                     SIGNAL (click( QtnPropertyABColor*)), this,
                     SLOT (pickColorFromImage( QtnPropertyABColor*)));


    QObject::connect(&imageProp.getProperties()->RMFilter.ColorFilter.PickColor,
                     SIGNAL(click( QtnPropertyABColor*)), this,
                     SLOT(pickColorFromImage( QtnPropertyABColor*)));

    ui->groupBoxConvertToHeightSettings->hide();

    imageProp.setOpenGLWidget(openGLWidget);
    
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

    getImage()->setTextureType(textureType);
    setupPropertiesGUI();
    setImageName("image");
}

ImageWidget::~ImageWidget()
{
    qDebug() << "calling" << Q_FUNC_INFO;
    delete heightCalculator;
    //delete imageProp.getProperties();
    delete ui;
}

void ImageWidget::setImage(QImage newImage)
{
    image = newImage;
    if (imageProp.getOpenGLWidget()->isValid())
        imageProp.init(image);
    else
        qDebug() << Q_FUNC_INFO << "Invalid context.";
}

void ImageWidget::setOpenGLWidget(QOpenGLWidget *openGLWidget)
{
    imageProp.setOpenGLWidget(openGLWidget);
}

void ImageWidget::setupPropertiesGUI()
{
    imageProp.getProperties()->ImageType.setValue((imageProp.getTextureType()));

    //    ui->groupBoxConvertToHeightSettings->setVisible(false);
    //    ui->groupBoxHN                 ->setVisible(false);
    ui->groupBoxNormalInputImage   ->setVisible(false);
    ui->groupBoxOcclusionInputImage->setVisible(false);
    ui->groupBoxRoughnessInputImage->setVisible(false);
    ui->groupBoxSpecularInputImage ->setVisible(false);
    ui->groupBoxNtoHConversion     ->setVisible(false);

    switch(imageProp.getTextureType())
    {
    case(DIFFUSE_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->RemoveShading.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->EnableRemoveShading.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->BaseMapToOthers.switchState(QtnPropertyStateInvisible,false);
        break;
    case(NORMAL_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.NormalsStep.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->GrungeOnImage.BlendingMode.switchState(QtnPropertyStateInvisible,true);
        imageProp.getProperties()->GrungeOnImage.ImageWeight.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->NormalsMixer.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxNormalInputImage->setVisible(true);
        break;
    case(SPECULAR_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxSpecularInputImage->setVisible(true);
        break;
    case(HEIGHT_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxNtoHConversion->setVisible(true);
        break;
    case(OCCLUSION_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->AO.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxOcclusionInputImage->setVisible(true);
        break;
    case(ROUGHNESS_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->RMFilter.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(METALLIC_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->RMFilter.switchState(QtnPropertyStateInvisible,false);
        ui->groupBoxRoughnessInputImage->setVisible(true);
        break;
    case(MATERIAL_TEXTURE):
        break;
    case(GRUNGE_TEXTURE):
        imageProp.getProperties()->Basic.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.GrayScale.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorComponents.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Basic.ColorHue.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->ColorLevels.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->SurfaceDetails.switchState(QtnPropertyStateInvisible,false);
        imageProp.getProperties()->Grunge.switchState(QtnPropertyStateInvisible,false);
        break;
    default:
        break;
    }
}

void ImageWidget::reloadSettings()
{
    loadingImages = true;

    if(imageProp.getTextureType() == HEIGHT_TEXTURE)
    {
        ui->horizontalSliderNormalToHeightNoiseLevel    ->setValue(imageProp.getProperties()->NormalHeightConv.NoiseLevel);
        ui->horizontalSliderNormalToHeightItersHuge     ->setValue(imageProp.getProperties()->NormalHeightConv.Huge);
        ui->horizontalSliderNormalToHeightItersVeryLarge->setValue(imageProp.getProperties()->NormalHeightConv.VeryLarge);
        ui->horizontalSliderNormalToHeightItersLarge    ->setValue(imageProp.getProperties()->NormalHeightConv.Large);
        ui->horizontalSliderNormalToHeightItersMedium   ->setValue(imageProp.getProperties()->NormalHeightConv.Medium);
        ui->horizontalSliderNormalToHeightItersVerySmall->setValue(imageProp.getProperties()->NormalHeightConv.Small);
        ui->horizontalSliderNormalToHeightItersSmall    ->setValue(imageProp.getProperties()->NormalHeightConv.VerySmall);
    }
    // Input image case study
    switch(imageProp.getTextureType())
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);
        switch(imageProp.getInputImageType())
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
        switch(imageProp.getInputImageType())
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
        switch(imageProp.getInputImageType())
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
        switch(imageProp.getInputImageType())
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
        switch(imageProp.getInputImageType())
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
    if(imageProp.getProperties()->NormalsMixer.EnableMixer)
    {
        qDebug() << "<FormImageProp> Open normal mixer image:" << fileName;
        //imageProp.getOpenGLWidget()->makeCurrent();
        //if(glIsTexture(imageProp.normalMixerInputTexId))
        //    imageProp.glWidget_ptr->deleteTexture(imageProp.normalMixerInputTexId);
        //    imageProp.normalMixerInputTexId = imageProp.glWidget_ptr->bindTexture(_image,GL_TEXTURE_2D);
        imageProp.setNormalMixerInputTexture(_image);

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
        if(imageProp.getTextureType() == GRUNGE_TEXTURE)emit imageChanged();
    }
    return true;
}

void ImageWidget::reloadImageSettings()
{
    emit reloadSettingsFromConfigFile(imageProp.getTextureType());
}

void ImageWidget::copyToClipboard()
{
    qDebug() << "<FormImageProp> Image :" +
                PostfixNames::getTextureName(imageProp.getTextureType()) +
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
                    PostfixNames::getTextureName(imageProp.getTextureType())+
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
                    PostfixNames::getTextureName(imageProp.getTextureType()) +
                    " loaded from clipboard.";
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage _image = pixmap.toImage();

        //imageProp.getOpenGLWidget()->makeCurrent();
        //        if(glIsTexture(imageProp.normalMixerInputTexId))
        //            imageProp.glWidget_ptr->deleteTexture(imageProp.normalMixerInputTexId);
        //        imageProp.normalMixerInputTexId = imageProp.glWidget_ptr->bindTexture(_image,GL_TEXTURE_2D);
        imageProp.setNormalMixerInputTexture(_image);

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
    switch(imageProp.getTextureType())
    {
    case(NORMAL_TEXTURE):
        // Select propper input image for normals.
        ui->pushButtonConverToNormal->setEnabled(false);

        switch(ui->comboBoxNormalInputImage->currentIndex())
        {
        case(0):
            imageProp.setInputImageType(INPUT_FROM_NORMAL_INPUT);
            ui->pushButtonConverToNormal->setEnabled(true);
            break;
        case(1):
            imageProp.setInputImageType(INPUT_FROM_HEIGHT_INPUT);
            break;
        case(2):
            imageProp.setInputImageType(INPUT_FROM_HEIGHT_OUTPUT);
            break;
        }
        break;
        // End case NORMAL_TEXTURE.

    case(SPECULAR_TEXTURE):
        // Select propper input image for specular.
        switch(ui->comboBoxSpecularInputImage->currentIndex())
        {
        case(0):
            imageProp.setInputImageType(INPUT_FROM_SPECULAR_INPUT);
            break;
        case(1):
            imageProp.setInputImageType(INPUT_FROM_DIFFUSE_INPUT);
            break;
        case(2):
            imageProp.setInputImageType(INPUT_FROM_DIFFUSE_OUTPUT);
            break;
        case(3):
            imageProp.setInputImageType(INPUT_FROM_HEIGHT_INPUT);
            break;
        case(4):
            imageProp.setInputImageType(INPUT_FROM_HEIGHT_OUTPUT);
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
            imageProp.setInputImageType(INPUT_FROM_OCCLUSION_INPUT);
            ui->pushButtonConvertOcclusionFromHN->setEnabled(true);
            break;
        case(1):
            imageProp.setInputImageType(INPUT_FROM_HI_NI);
            break;
        case(2):
            imageProp.setInputImageType(INPUT_FROM_HO_NO);
            break;
        }
        break;
        // End case OCCLUSION_TEXTURE.

    case(ROUGHNESS_TEXTURE):
        // Select propper input image for roughness.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            imageProp.setInputImageType(INPUT_FROM_ROUGHNESS_INPUT);
            break;
        case(1):
            imageProp.setInputImageType(INPUT_FROM_DIFFUSE_INPUT);
            break;
        case(2):
            imageProp.setInputImageType(INPUT_FROM_DIFFUSE_OUTPUT);
            break;
        }
        break;
        // End case ROUGHNESS_TEXTURE.

    case(METALLIC_TEXTURE):
        // Select propper input image for metallic.
        switch(ui->comboBoxRoughnessInputImage->currentIndex())
        {
        case(0):
            imageProp.setInputImageType(INPUT_FROM_METALLIC_INPUT);
            break;
        case(1):
            imageProp.setInputImageType(INPUT_FROM_DIFFUSE_INPUT);
            break;
        case(2):
            imageProp.setInputImageType(INPUT_FROM_DIFFUSE_OUTPUT);
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

    imageProp.setConversionHNDepth(ui->doubleSpinBoxConversionHNDepth->value());

    imageProp.getProperties()->NormalHeightConv.NoiseLevel = ui->horizontalSliderNormalToHeightNoiseLevel     ->value();
    imageProp.getProperties()->NormalHeightConv.Huge       = ui->horizontalSliderNormalToHeightItersHuge      ->value();
    imageProp.getProperties()->NormalHeightConv.VeryLarge  = ui->horizontalSliderNormalToHeightItersVeryLarge ->value();
    imageProp.getProperties()->NormalHeightConv.Large      = ui->horizontalSliderNormalToHeightItersLarge     ->value();
    imageProp.getProperties()->NormalHeightConv.Medium     = ui->horizontalSliderNormalToHeightItersMedium    ->value();
    imageProp.getProperties()->NormalHeightConv.Small      = ui->horizontalSliderNormalToHeightItersSmall     ->value();
    imageProp.getProperties()->NormalHeightConv.VerySmall  = ui->horizontalSliderNormalToHeightItersVerySmall ->value();
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
                == &imageProp.getProperties()->Grunge.Patterns)
        {
            loadPredefinedGrunge(imageProp.getProperties()->Grunge.Patterns.value());
        }
        // Enable Grunge.
        if(imageProp.getProperties()->Grunge.OverallWeight.value() == 0)
        {
            emit  toggleGrungeSettings(false);
        }
        else
        {
            emit  toggleGrungeSettings(true);
        }
        // Open Normal mixer.
        if(dynamic_cast<const QtnPropertyQString*>(changedProperty)
                == &imageProp.getProperties()->NormalsMixer.NormalImage)
        {
            loadFile(imageProp.getProperties()->NormalsMixer.NormalImage);
        }
        // Bool activated, enum changed.
        if(dynamic_cast<const QtnPropertyBool*>(changedProperty)
                || dynamic_cast<const QtnPropertyEnum*>(changedProperty))
        {
            // Enable BaseMapToOthers Conversion Tool.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &imageProp.getProperties()->BaseMapToOthers.EnableConversion)
            {
                Image::bConversionBaseMap = imageProp.getProperties()->BaseMapToOthers.EnableConversion;
            }
            // Enable BaseMapToOthers Conversion Tool Height Preview.
            if( dynamic_cast<const QtnPropertyBool*>(changedProperty)
                    == &imageProp.getProperties()->BaseMapToOthers.EnableHeightPreview)
            {
                Image::bConversionBaseMapShowHeightTexture = imageProp.getProperties()->BaseMapToOthers.EnableHeightPreview;
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
    //heightCalculator->setImageSize(imageProp.ref_fbo->width(),imageProp.ref_fbo->height());
    heightCalculator->setImageSize(imageProp.getFBO()->width(),imageProp.getFBO()->height());
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
    imageProp.getProperties()->GrungeOnImage.switchState(QtnPropertyStateInvisible,!toggle);
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
    if(imageProp.getTextureType() == GRUNGE_TEXTURE)emit imageChanged();
}
