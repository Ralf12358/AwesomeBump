#include "image.h"

#include <QDebug>

#include "openglerrorcheck.h"
#include "openglframebufferobject.h"

SeamlessMode Image::seamlessMode                 = SEAMLESS_NONE;
float Image::seamlessSimpleModeRadius            = 0.5;
float Image::seamlessContrastPower               = 0.0;
float Image::seamlessContrastStrength            = 0.0;
int Image::seamlessSimpleModeDirection           = 0; // xy
ImageType Image::seamlessContrastImageType = INPUT_FROM_HEIGHT_INPUT;
bool Image::bSeamlessTranslationsFirst           = true;
int Image::seamlessMirroModeType                 = 0;
bool Image::bConversionBaseMap                   = false;
bool Image::bConversionBaseMapShowHeightTexture  = false;
int Image::currentMaterialIndex                 = MATERIALS_DISABLED;
RandomTilingMode Image::randomTilingMode     = RandomTilingMode();

Image::Image()
{
    bSkipProcessing       = false;
    properties            = NULL;
    fbo                   = NULL;
    normalMixerInputTexture = 0;
    openGLWidget          = NULL;
    bFirstDraw            = true;
    texture            = 0;
    conversionHNDepth     = 2.0;
    bConversionBaseMap    = false;
    inputImageType        = INPUT_NONE;
    seamlessMode          = SEAMLESS_NONE;
    properties            = new QtnPropertySetFormImageProp;
}

Image::~Image()
{
    if(openGLWidget != NULL)
    {
        qDebug() << Q_FUNC_INFO;
        openGLWidget->makeCurrent();

        if(normalMixerInputTexture)
            delete normalMixerInputTexture;
        if(texture)
            delete texture;
        normalMixerInputTexture = 0;
        texture = 0;
        openGLWidget = NULL;
        //qDebug() << "p=" << properties;
        if(properties != NULL ) delete properties;
        if(fbo        != NULL ) delete fbo;
        properties = NULL;
        fbo        = NULL;
    }
}

void Image::copySettings(const Image &source)
{
    bFirstDraw         = source.bFirstDraw;
    conversionHNDepth  = source.conversionHNDepth;
    bConversionBaseMap = source.bConversionBaseMap;
    inputImageType     = source.inputImageType;

    if(properties != NULL && source.properties != NULL )
        properties->copyValues(source.properties);
}

void Image::init(QImage& image)
{
    qDebug() << Q_FUNC_INFO;

    openGLWidget->makeCurrent();
    if(texture)
        delete texture;
    //scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
    texture = new QOpenGLTexture(image);
    textureWidth  = image.width();
    textureHeight = image.height();
    bFirstDraw    = true;

    /*
    switch(imageType)
    {
    case(HEIGHT_TEXTURE):
    case(OCCLUSION_TEXTURE):
        GLCHK( OpenGLFramebufferObject::create(fbo, image.width(), image.height(), GL_R16F) );
        break;
    default:
        GLCHK( OpenGLFramebufferObject::create(fbo, image.width(), image.height()) );
        break;
    }
    */

    GLuint internal_format = TEXTURE_FORMAT;
    if(textureType == HEIGHT_TEXTURE)
        internal_format = TEXTURE_3DRENDER_FORMAT;
    GLCHK(OpenGLFramebufferObject::create(fbo , image.width(), image.height(),internal_format));
}

QOpenGLWidget* Image::getOpenGLWidget()
{
    return openGLWidget;
}

void Image::setOpenGLWidget(QOpenGLWidget* newWidget)
{
    openGLWidget = newWidget;
}

QOpenGLFramebufferObject* Image::getFBO()
{
    return fbo;
}

void Image::updateTextureFromFBO(QOpenGLFramebufferObject* sourceFBO)
{
    openGLWidget->makeCurrent();
    if(texture)
        delete texture;
    QImage image = sourceFBO->toImage();
    //scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
    texture = new QOpenGLTexture(image);
}

void Image::resizeFBO(int width, int height)
{
    GLuint internal_format = TEXTURE_FORMAT;
    if(textureType == HEIGHT_TEXTURE)
        internal_format = TEXTURE_3DRENDER_FORMAT;
    GLCHK( OpenGLFramebufferObject::resize(fbo,width,height,internal_format) );
    bFirstDraw = true;
}

QImage Image::getFBOImage()
{
    openGLWidget->makeCurrent();
    return fbo->toImage();
}

QtnPropertySetFormImageProp* Image::getProperties()
{
    return properties;
}

QOpenGLTexture* Image::getTexture()
{
    return texture;
}

void Image::setTexture(const QImage& image)
{
    if(texture)
        delete texture;
    texture = new QOpenGLTexture(image);
    openGLWidget->makeCurrent();
    texture->bind();
}

TextureType Image::getTextureType()
{
    return textureType;
}

void Image::setTextureType(TextureType textureType)
{
    this->textureType = textureType;
}

int Image::getTextureWidth()
{
    return textureWidth;
}

int Image::getTextureHeight()
{
    return textureHeight;
}

ImageType Image::getInputImageType()
{
    return inputImageType;
}

void Image::setInputImageType(ImageType inputImageType)
{
    this->inputImageType = inputImageType;
}

QOpenGLTexture* Image::getNormalMixerInputTexture()
{
    return normalMixerInputTexture;
}

void Image::setNormalMixerInputTexture(const QImage& image)
{
    if(normalMixerInputTexture)
        delete normalMixerInputTexture;
    normalMixerInputTexture = new QOpenGLTexture(image);
    openGLWidget->makeCurrent();
    normalMixerInputTexture->bind();
}

bool Image::isSkippingProcessing()
{
    return bSkipProcessing;
}

void Image::setSkipProcessing(bool skipProcessing)
{
    bSkipProcessing = skipProcessing;
}

bool Image::isFirstDraw()
{
    return bFirstDraw;
}

void Image::setFirstDraw(bool isFirstDraw)
{
    bFirstDraw = isFirstDraw;
}

BaseMapConvLevelProperties* Image::getBaseMapConvLevelProperties()
{
    return baseMapConvLevelProperties;
}

float Image::getConversionHNDepth()
{
    return conversionHNDepth;
}

void Image::setConversionHNDepth(float newDepth)
{
    conversionHNDepth = newDepth;
}
