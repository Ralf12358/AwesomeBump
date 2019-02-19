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

Image::Image(OpenGL2DImageWidget *openGL2DImageWidget) :
    openGL2DImageWidget(openGL2DImageWidget),
    fbo(0),
    texture(0),
    normalMixerInputTexture(0),
    bFirstDraw(true),
    bSkipProcessing(false),
    conversionHNDepth(2.0),
    inputImageType(INPUT_NONE)
{
}

Image::~Image()
{
    if(normalMixerInputTexture) delete normalMixerInputTexture;
    if(texture) delete texture;
    if(fbo) delete fbo;
}

void Image::copySettings(Image *source)
{
    bFirstDraw         = source->bFirstDraw;
    conversionHNDepth  = source->conversionHNDepth;
    bConversionBaseMap = source->bConversionBaseMap;
    inputImageType     = source->inputImageType;
    properties.copyValues(&source->properties);
}

void Image::setImage(const QImage& image)
{
    this->image = image;
    bFirstDraw = true;
}

QOpenGLFramebufferObject* Image::getFBO()
{
    if (!fbo) createFBO(image.width(), image.height());
    return fbo;
}

void Image::updateImageFromFBO(QOpenGLFramebufferObject* sourceFBO)
{
    image = sourceFBO->toImage();
    bFirstDraw = true;
}

void Image::resizeFBO(int width, int height)
{
    createFBO(width, height);
}

QImage Image::getFBOImage()
{
    openGL2DImageWidget->makeCurrent();
    return fbo->toImage();
}

QtnPropertySetFormImageProp* Image::getProperties()
{
    return &properties;
}

QOpenGLTexture* Image::getTexture()
{
    if (bFirstDraw)
    {
        if(texture) delete texture;
        texture = new QOpenGLTexture(image);
        bFirstDraw = false;
    }
    return texture;
}

TextureType Image::getTextureType()
{
    return textureType;
}

void Image::setTextureType(TextureType textureType)
{
    this->textureType = textureType;
}

int Image::width()
{
    return image.width();
}

int Image::height()
{
    return image.height();
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
    if(normalMixerInputTexture) delete normalMixerInputTexture;
    normalMixerInputTexture = new QOpenGLTexture(image);
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
