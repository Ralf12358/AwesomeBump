#ifndef IMAGE_H
#define IMAGE_H

#include <QImage>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>

#include "properties/ImageProperties.peg.h"
#include "postfixnames.h"
#include "basemapconvlevelproperties.h"
#include "randomtilingmode.h"

#define TEXTURE_3DRENDER_FORMAT GL_RGB16F
#define TEXTURE_FORMAT GL_RGB16F

enum MaterialIndicesType
{
    MATERIALS_DISABLED = -10,
    MATERIALS_ENABLED = -1
};

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
    Image();
    ~Image();

    void copySettings(const Image *source);
    void setImage(const QImage &image);

    void setOpenGL2DImageWidget(OpenGL2DImageWidget *openGL2DImageWidget);

    QOpenGLFramebufferObject* getFBO();
    void updateImageFromFBO(QOpenGLFramebufferObject* in_ref_fbo);
    void resizeFBO(int width, int height);
    // Convert FBO image to QImage
    QImage getFBOImage();

    QtnPropertySetFormImageProp* getProperties();
    QOpenGLTexture* getTexture();
    TextureType getTextureType();
    void setTextureType(TextureType textureType);
    int getWidth();
    int getHeight();
    ImageType getInputImageType();
    void setInputImageType(ImageType inputImageType);
    QOpenGLTexture* getNormalMixerInputTexture();
    void setNormalMixerInputTexture(const QImage& image);

    bool isSkippingProcessing();
    void setSkipProcessing(bool skipProcessing);
    bool isFirstDraw();

    BaseMapConvLevelProperties* getBaseMapConvLevelProperties();
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
    static RandomTilingMode randomTilingMode;
    static float seamlessContrastStrength;
    static float seamlessContrastPower;
    static int seamlessSimpleModeDirection;
    static ImageType seamlessContrastImageType;
    static bool bSeamlessTranslationsFirst;
    static int currentMaterialIndex;
    static bool bUseLinearInterpolation;

private:
    void createFBO(int width, int height);

    QtnPropertySetFormImageProp *properties;
    bool bSkipProcessing;
    // Pointer to the OpenGL 2D Image Widget.
    OpenGL2DImageWidget *openGL2DImageWidget;
    // Output image.
    QOpenGLFramebufferObject *fbo;
    // Texture created from the image.
    QOpenGLTexture *texture;
    // Used only by normal texture.
    QOpenGLTexture *normalMixerInputTexture;
    // The kind of preprocessing that will be applied to the image.
    TextureType textureType;

    bool bFirstDraw;
    // Conversion settings
    float conversionHNDepth;
    BaseMapConvLevelProperties baseMapConvLevelProperties[4];
    // Input image type
    ImageType inputImageType;

    QString imageName;
    QImage image;
};

#endif // IMAGE_H
