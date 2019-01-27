#ifndef OPENGLIMAGE
#define OPENGLIMAGE

#include <QImage>
#include <QOpenGLWidget>
#include <QOpenGLFramebufferObject>
#include <QOpenGLTexture>

#include "properties/ImageProperties.peg.h"
#include "postfixnames.h"
#include "basemapconvlevelproperties.h"
#include "randomtilingmode.h"

#define TEXTURE_3DRENDER_FORMAT GL_RGB16F

enum MaterialIndicesType
{
    MATERIALS_DISABLED = -10,
    MATERIALS_ENABLED = -1
};

enum SourceImageType
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

class OpenGLImage
{
public:
    OpenGLImage();
    ~OpenGLImage();

    void copySettings(OpenGLImage &src);
    void init(QImage& image);
    void updateSrcTexId(QOpenGLFramebufferObject* in_ref_fbo);
    void resizeFBO(int width, int height);
    // Convert FBO image to QImage
    QImage getImage();

    QtnPropertySetFormImageProp* properties;
    bool bSkipProcessing;
    // Output image
    QOpenGLFramebufferObject *fbo;
    // Id of texture loaded from image, from loaded file.
    //GLuint scr_tex_id;
    QOpenGLTexture *scr_tex_id;
    // Used only by normal texture.
    //GLuint normalMixerInputTexId;
    QOpenGLTexture *normalMixerInputTexId;
    // Width and height of the image loaded from file.
    int scr_tex_width;
    int scr_tex_height;
    // Pointer to GL context.
    QOpenGLWidget* glWidget_ptr;
    // This will define what kind of preprocessing will be applied to image
    TextureTypes imageType;

    bool bFirstDraw;
    // Conversion settings
    float conversionHNDepth;
    // Base to others settings
    static bool bConversionBaseMap;
    static bool bConversionBaseMapShowHeightTexture;
    BaseMapConvLevelProperties baseMapConvLevels[4];
    // Input image type
    SourceImageType inputImageType;

    static SeamlessMode seamlessMode;
    static float seamlessSimpleModeRadius;
    // values: 2 - x repear, 1 - y  repeat, 0 - xy  repeat
    static int seamlessMirroModeType;
    static RandomTilingMode seamlessRandomTiling;
    static float seamlessContrastStrenght;
    static float seamlessContrastPower;
    static int seamlessSimpleModeDirection;
    static SourceImageType seamlessContrastInputType;
    static bool bSeamlessTranslationsFirst;
    static int currentMaterialIndeks;
};

#endif // OPENGLIMAGE
