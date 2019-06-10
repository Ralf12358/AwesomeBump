#ifndef IMAGE_H
#define IMAGE_H

#include <QImage>

#include "properties/ImageProperties.peg.h"

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
    ImageType getInputImageType();
    void setInputImageType(ImageType inputImageType);

    // Base to others settings
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
    static QString outputFormat;

private:
    QtnPropertySetFormImageProp properties;

    // Input image type
    ImageType inputImageType;
};

#endif // IMAGE_H
