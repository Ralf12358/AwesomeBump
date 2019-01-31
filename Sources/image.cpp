#include "image.h"

#include <QDebug>

#include "opengl2dimagewidget.h"
#include "openglerrorcheck.h"

SeamlessMode Image::seamlessMode                 = SEAMLESS_NONE;
float Image::seamlessSimpleModeRadius            = 0.5;
float Image::seamlessContrastPower               = 0.0;
float Image::seamlessContrastStrength            = 0.0;
int Image::seamlessSimpleModeDirection           = 0; // xy
ImageType Image::seamlessContrastImageType       = INPUT_FROM_HEIGHT_INPUT;
bool Image::bSeamlessTranslationsFirst           = true;
int Image::seamlessMirroModeType                 = 0;
bool Image::bConversionBaseMap                   = false;
bool Image::bConversionBaseMapShowHeightTexture  = false;
int Image::currentMaterialIndex                  = MATERIALS_DISABLED;
RandomTilingMode Image::randomTilingMode         = RandomTilingMode();
bool Image::bUseLinearInterpolation              = true;

Image::Image()
{
    bSkipProcessing       = false;
    properties            = NULL;
    fbo                   = NULL;
    normalMixerInputTexture = 0;
    openGL2DImageWidget          = NULL;
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
    if(openGL2DImageWidget != NULL)
    {
        qDebug() << Q_FUNC_INFO;
        openGL2DImageWidget->makeCurrent();

        if(normalMixerInputTexture)
            delete normalMixerInputTexture;
        if(texture)
            delete texture;
        normalMixerInputTexture = 0;
        texture = 0;
        openGL2DImageWidget = NULL;
        //qDebug() << "p=" << properties;
        if(properties != NULL ) delete properties;
        if(fbo        != NULL ) delete fbo;
        properties = NULL;
        fbo        = NULL;
    }
}

void Image::copySettings(const Image *source)
{
    bFirstDraw         = source->bFirstDraw;
    conversionHNDepth  = source->conversionHNDepth;
    bConversionBaseMap = source->bConversionBaseMap;
    inputImageType     = source->inputImageType;

    if(properties != NULL && source->properties != NULL )
        properties->copyValues(source->properties);
}

void Image::init(const QImage& image)
{
    qDebug() << Q_FUNC_INFO;

    if (!openGL2DImageWidget->isValid())
    {
          qDebug() << Q_FUNC_INFO << "OpenGL2DImageWidget not initialised.";
          qImage = image;
          return;
    }

    if(texture) delete texture;

    texture = new QOpenGLTexture(image);
    textureWidth  = image.width();
    textureHeight = image.height();
    bFirstDraw    = true;

    createFBO(image.width(), image.height());
}

OpenGL2DImageWidget* Image::getOpenGL2DImageWidget()
{
    return openGL2DImageWidget;
}

void Image::setOpenGL2DImageWidget(OpenGL2DImageWidget* openGL2DImageWidget)
{
    this->openGL2DImageWidget = openGL2DImageWidget;
}

QOpenGLFramebufferObject* Image::getFBO()
{
    if (!fbo)
        init(qImage);
    return fbo;
}

void Image::updateTextureFromFBO(QOpenGLFramebufferObject* sourceFBO)
{
    openGL2DImageWidget->makeCurrent();
    if(texture)
        delete texture;
    QImage image = sourceFBO->toImage();
    //scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
    texture = new QOpenGLTexture(image);
}

void Image::resizeFBO(int width, int height)
{
    createFBO(width, height);
    bFirstDraw = true;
}

QImage Image::getFBOImage()
{
    openGL2DImageWidget->makeCurrent();
    return fbo->toImage();
}

QtnPropertySetFormImageProp* Image::getProperties()
{
    return properties;
}

QOpenGLTexture* Image::getTexture()
{
    if (!texture)
        init(qImage);
    return texture;
}

void Image::setTexture(const QImage& image)
{
    if(texture)
        delete texture;
    texture = new QOpenGLTexture(image);
    openGL2DImageWidget->makeCurrent();
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
    openGL2DImageWidget->makeCurrent();
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

QString Image::getImageName()
{
    return imageName;
}

void Image::setImageName(const QString& newName)
{
    imageName = newName;
}

void Image::createFBO(int width, int height)
{
    if (fbo) delete fbo;

    GLenum internalFormat = TEXTURE_FORMAT;
    if(textureType == HEIGHT_TEXTURE)
        internalFormat = TEXTURE_3DRENDER_FORMAT;

    QOpenGLFramebufferObjectFormat format;
    format.setInternalTextureFormat(internalFormat);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setMipmap(true);
    fbo = new QOpenGLFramebufferObject(width, height, format);

    GLCHK( glBindTexture(GL_TEXTURE_2D, fbo->texture()) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) );

    if(bUseLinearInterpolation)
    {
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    }
    else
    {
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST) );
    }

    float aniso = 0.0f;
    GLCHK( glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, 0) );
}
