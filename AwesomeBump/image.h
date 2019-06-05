#ifndef IMAGE_H
#define IMAGE_H

#include <QImage>
#include <QOpenGLTexture>

#include "properties/ImageProperties.peg.h"
#include "basemapconvlevelproperties.h"

enum ImageType
{
    INPUT_NONE = 0,
    INPUT_FROM_HEIGHT_INPUT,
    INPUT_FROM_HEIGHT_OUTPUT,
    INPUT_FROM_NORMAL_INPUT,
    INPUT_FROM_NORMAL_OUTPUT,
    INPUT_FROM_SPECULAR_INPUT,
    INPUT_FROM_SPECULAR_OUTPUT,
    INPUT_FROM_DIFFUSE_INPUT,
    INPUT_FROM_DIFFUSE_OUTPUT,
    INPUT_FROM_OCCLUSION_INPUT,
    INPUT_FROM_ROUGHNESS_INPUT,
    INPUT_FROM_ROUGHNESS_OUTPUT,
    INPUT_FROM_METALLIC_INPUT,
    INPUT_FROM_METALLIC_OUTPUT,
    INPUT_FROM_HI_NI,
    INPUT_FROM_HO_NO
};

#define TEXTURES 9
enum TextureType
{
    DIFFUSE_TEXTURE = 0,
    NORMAL_TEXTURE ,
    SPECULAR_TEXTURE,
    HEIGHT_TEXTURE,
    OCCLUSION_TEXTURE,
    ROUGHNESS_TEXTURE,
    METALLIC_TEXTURE,
    MATERIAL_TEXTURE,
    GRUNGE_TEXTURE
};

// Methods of making the texture seamless.
enum SeamlessMode
{
    SEAMLESS_NONE = 0,
    SEAMLESS_SIMPLE,
    SEAMLESS_MIRROR,
    SEAMLESS_RANDOM
};

class OpenGL2DImageWidget;

class Image
{
public:
    explicit Image();
    ~Image();

    void copySettings(Image *source);

    QtnPropertySetFormImageProp* getProperties();
    TextureType getTextureType();
    QString getTextureName();
    QString getTextureSuffix();
    void setTextureType(TextureType textureType);
    ImageType getInputImageType();
    void setInputImageType(ImageType inputImageType);
    QOpenGLTexture* getNormalMixerInputTexture();
    void setNormalMixerInputTexture(const QImage& image);

    bool isSkippingProcessing();
    void setSkipProcessing(bool skipProcessing);
    bool isFirstDraw();

    BaseMapConvLevelProperties *getBaseMapConvLevelProperties();
    float getConversionHNDepth();
    void setConversionHNDepth(float newDepth);

    QString getImageName();
    void setImageName(const QString& newName);

    // Base to others settings
    static bool bConversionBaseMap;
    static bool bConversionBaseMapShowHeightTexture;
    static SeamlessMode seamlessMode;
    static float seamlessSimpleModeRadius;
    // values: 2 - x repear, 1 - y  repeat, 0 - xy  repeat
    static int seamlessMirroModeType;
    static float randomAngles[9];
    static float randomCommonPhase;
    static float randomInnerRadius;
    static float randomOuterRadius;
    static void randomize();
    static void randomReset();
    static float seamlessContrastStrength;
    static float seamlessContrastPower;
    static int seamlessSimpleModeDirection;
    static ImageType seamlessContrastImageType;
    static bool bSeamlessTranslationsFirst;
    static int currentMaterialIndex;
    static bool materialsEnabled;
    static bool bUseLinearInterpolation;

    static QString diffuseName;
    static QString normalName;
    static QString specularName;
    static QString heightName;
    static QString occlusionName;
    static QString roughnessName;
    static QString metallicName;
    static QString outputFormat;

private:
    QtnPropertySetFormImageProp properties;
    // Used only by normal texture.
    QOpenGLTexture *normalMixerInputTexture;

    QString imageName;

    bool bFirstDraw;
    bool bSkipProcessing;

    // Conversion settings
    float conversionHNDepth;
    BaseMapConvLevelProperties baseMapConvLevelProperties[4];
    // Input image type
    ImageType inputImageType;
    // The kind of preprocessing that will be applied to the image.
    TextureType textureType;
};

#endif // IMAGE_H
