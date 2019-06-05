#include "image.h"

#include <QDebug>

SeamlessMode Image::seamlessMode                = SEAMLESS_NONE;
float Image::seamlessSimpleModeRadius           = 0.5;
float Image::seamlessContrastPower              = 0.0;
float Image::seamlessContrastStrength           = 0.0;
int Image::seamlessSimpleModeDirection          = 0; // xy
ImageType Image::seamlessContrastImageType      = INPUT_FROM_HEIGHT_INPUT;
bool Image::bSeamlessTranslationsFirst          = true;
int Image::seamlessMirroModeType                = 0;
bool Image::bConversionBaseMap                  = false;
bool Image::bConversionBaseMapShowHeightTexture = false;
int Image::currentMaterialIndex                 = 0;
bool Image::materialsEnabled                    = false;
float Image::randomAngles[]                     = {0};
float Image::randomCommonPhase                  = 0.0;
float Image::randomInnerRadius                  = 0.2;
float Image::randomOuterRadius                  = 0.4;
bool Image::bUseLinearInterpolation             = true;

QString  Image::diffuseName   = "_d";
QString  Image::normalName    = "_n";
QString  Image::specularName  = "_s";
QString  Image::heightName    = "_h";
QString  Image::occlusionName = "_o";
QString  Image::roughnessName = "_r";
QString  Image::metallicName  = "_m";
QString  Image::outputFormat  = ".png";

void Image::randomize()
{
    static int seed = 312;
    qsrand(seed);
    // Fake seed
    seed = qrand() % 41211;
    for(int i = 0; i < 9 ; i++)
    {
        randomAngles[i] = 2 * 3.1415269 * qrand() / (RAND_MAX + 0.0);
    }
}

void Image::randomReset()
{
    for (unsigned int i = 0; i < 9; ++i) randomAngles[i] = 0;
}

Image::Image() :
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
}

void Image::copySettings(Image *source)
{
    bFirstDraw         = source->bFirstDraw;
    conversionHNDepth  = source->conversionHNDepth;
    bConversionBaseMap = source->bConversionBaseMap;
    inputImageType     = source->inputImageType;
    properties.copyValues(&source->properties);
}

QtnPropertySetFormImageProp* Image::getProperties()
{
    return &properties;
}

TextureType Image::getTextureType()
{
    return textureType;
}

QString Image::getTextureName()
{
    switch(textureType)
    {
    case(DIFFUSE_TEXTURE):
        return "diffuse";
        break;
    case(NORMAL_TEXTURE):
        return "normal";
        break;
    case(SPECULAR_TEXTURE):
        return "specular";
        break;
    case(HEIGHT_TEXTURE):
        return "height";
        break;
    case(OCCLUSION_TEXTURE):
        return "occlusion";
        break;
    case(ROUGHNESS_TEXTURE):
        return "roughness";
        break;
    case(METALLIC_TEXTURE):
        return "metallic";
        break;
    case(MATERIAL_TEXTURE):
        return "material";
    case(GRUNGE_TEXTURE):
        return "grunge";
        break;
    default:
        return "default-diffuse";
        break;
    }
}


QString Image::getTextureSuffix()
{
    switch(textureType)
    {
    case(DIFFUSE_TEXTURE):
        return diffuseName;
        break;
    case(NORMAL_TEXTURE):
        return normalName;
        break;
    case(SPECULAR_TEXTURE):
        return specularName;
        break;
    case(HEIGHT_TEXTURE):
        return heightName;
        break;
    case(OCCLUSION_TEXTURE):
        return occlusionName;
        break;
    case(ROUGHNESS_TEXTURE):
        return roughnessName;
        break;
    case(METALLIC_TEXTURE) :
        return metallicName;
        break;
    default:
        return diffuseName;
        break;
    }
}

void Image::setTextureType(TextureType textureType)
{
    this->textureType = textureType;
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
