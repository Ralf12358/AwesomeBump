#ifndef COMMONOBJECTS_H
#define COMMONOBJECTS_H

#include <QtOpenGL>
#include <cstdio>
#include <iostream>
#include "qopenglerrorcheck.h"
#include <QOpenGLFunctions_3_3_Core>
#include "properties/ImageProperties.peg.h"
#include "targaimage.h"
#include "postfixnames.h"
#include "randomtilingmode.h"
#include "display3dsettings.h"

#define TAB_SETTINGS 9
#define TAB_TILING   10

#ifdef Q_OS_MAC
# define AB_INI "AwesomeBump.ini"
# define AB_LOG "AwesomeBump.log"
#else
# define AB_INI "config.ini"
# define AB_LOG "log.txt"
#endif

//#define TEXTURE_FORMAT GL_RGB16F
#define TEXTURE_FORMAT GL_RGB16F
#define TEXTURE_3DRENDER_FORMAT GL_RGB16F

#define KEY_SHOW_MATERIALS Qt::Key_S

//#define USE_OPENGL_330

#ifdef USE_OPENGL_330
#define AWESOME_BUMP_VERSION "AwesomeBump " VERSION_STRING " (2016) (openGL 330 release)"
#else
#define AWESOME_BUMP_VERSION "AwesomeBump " VERSION_STRING " (2016)"
#endif

using namespace std;

enum ConversionType
{
    CONVERT_NONE = 0,
    CONVERT_FROM_H_TO_N,
    CONVERT_FROM_N_TO_H,
    CONVERT_FROM_D_TO_O,    // Diffuse to others.
    CONVERT_FROM_HN_TO_OC,
    CONVERT_RESIZE
};

enum UVManipulationMethods
{
    UV_TRANSLATE = 0,
    UV_GRAB_CORNERS,
    UV_SCALE_XY
};

// Methods of making the texture seamless.
enum SeamlessMode
{
    SEAMLESS_NONE = 0,
    SEAMLESS_SIMPLE,
    SEAMLESS_MIRROR,
    SEAMLESS_RANDOM
};

// Compressed texture type.
enum CompressedFromTypes
{
    H_TO_D_AND_S_TO_N = 0,
    S_TO_D_AND_H_TO_N = 1
};

// Selective blur methods.
enum SelectiveBlurType
{
    SELECTIVE_BLUR_LEVELS = 0,
    SELECTIVE_BLUR_DIFFERENCE_OF_GAUSSIANS
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

enum ColorPickerMethod
{
    COLOR_PICKER_METHOD_A = 0,
    COLOR_PICKER_METHOD_B ,
};

enum MaterialIndicesType
{
    MATERIALS_DISABLED = -10,
    MATERIALS_ENABLED = -1
};

// Wrapper for FBO initialization.
class FBOImages
{
public:
    static void create(QGLFramebufferObject *&fbo,int width,int height,GLuint internal_format = TEXTURE_FORMAT)
    {
        if(fbo)
        {
            fbo->release();
            delete fbo;
        }
        QGLFramebufferObjectFormat format;
        format.setInternalTextureFormat(internal_format);
        format.setTextureTarget(GL_TEXTURE_2D);
        format.setMipmap(true);
        fbo = new QGLFramebufferObject(width,height,format);
        glBindTexture(GL_TEXTURE_2D, fbo->texture());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        if(FBOImages::bUseLinearInterpolation)
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        else
        {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
        float aniso = 0.0;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
        qDebug() << "FBOImages::creating new FBO(" << width << "," << height << ") with id=" << fbo->texture() ;
    }

    static void resize(QGLFramebufferObject *&src,QGLFramebufferObject *&ref,GLuint internal_format = TEXTURE_FORMAT)
    {
        if(src == NULL)
        {
            GLCHK(FBOImages::create(src ,ref->width(),ref->height(),internal_format));
        }
        else if( ref->width()  == src->width() &&
                  ref->height() == src->height() )
        {}
        else
        {
            GLCHK(FBOImages::create(src ,ref->width(),ref->height(),internal_format));
        }
    }

    static void resize(QGLFramebufferObject *&src,int width, int height,GLuint internal_format = TEXTURE_FORMAT)
    {
        if(!src)
        {
            GLCHK(FBOImages::create(src ,width,height,internal_format));
        }
        else if( width  == src->width() &&
                  height == src->height() )
        {}
        else
        {
            GLCHK(FBOImages::create(src ,width,height,internal_format));
        }
    }

public:
    static bool bUseLinearInterpolation;
};

struct BaseMapConvLevelProperties
{
    float conversionBaseMapAmplitude;
    float conversionBaseMapFlatness;
    int   conversionBaseMapNoIters;
    float conversionBaseMapFilterRadius;
    float conversionBaseMapMixNormals;
    float conversionBaseMapPreSmoothRadius;
    float conversionBaseMapBlending;

    BaseMapConvLevelProperties()
    {
        conversionBaseMapAmplitude       = 0;
        conversionBaseMapFlatness        = 0.5;
        conversionBaseMapNoIters         = 0;
        conversionBaseMapFilterRadius    = 3;
        conversionBaseMapMixNormals      = 1.0;
        conversionBaseMapPreSmoothRadius = 0;
        conversionBaseMapBlending        = 1.0;
    }

    void fromProperty(QtnPropertySetConvertsionBaseMapLevelProperty& level)
    {
        conversionBaseMapAmplitude       = level.Amplitude;
        conversionBaseMapFlatness        = level.Flatness;
        conversionBaseMapNoIters         = level.NumIters;
        conversionBaseMapFilterRadius    = level.FilterRadius;
        conversionBaseMapMixNormals      = level.Edges;
        conversionBaseMapPreSmoothRadius = level.PreSmoothRadius;
        conversionBaseMapBlending        = level.Blending;
    }
};

class FBOImageProporties
{
public:
    QtnPropertySetFormImageProp* properties;
    bool bSkipProcessing;
    // Output image
    QGLFramebufferObject *fbo;
    // Id of texture loaded from image, from loaded file.
    GLuint scr_tex_id;
    // Used only by normal texture.
    GLuint normalMixerInputTexId;
    // Width and height of the image loaded from file.
    int scr_tex_width;
    int scr_tex_height;
    // Pointer to GL context.
    QGLWidget* glWidget_ptr;
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

    FBOImageProporties()
    {
        bSkipProcessing       = false;
        properties            = NULL;
        fbo                   = NULL;
        normalMixerInputTexId = 0;
        glWidget_ptr          = NULL;
        bFirstDraw            = true;
        scr_tex_id            = 0;
        conversionHNDepth     = 2.0;
        bConversionBaseMap    = false;
        inputImageType        = INPUT_NONE;
        seamlessMode          = SEAMLESS_NONE;
        properties            = new QtnPropertySetFormImageProp;
    }

    void copySettings(FBOImageProporties &src)
    {
        bFirstDraw         = src.bFirstDraw;
        conversionHNDepth  = src.conversionHNDepth;
        bConversionBaseMap = src.bConversionBaseMap;
        inputImageType     = src.inputImageType;

        if(properties != NULL && src.properties != NULL )
            properties->copyValues(src.properties);
    }

    void init(QImage& image)
    {
        qDebug() << Q_FUNC_INFO;

        glWidget_ptr->makeCurrent();
        if(glIsTexture(scr_tex_id))
            glWidget_ptr->deleteTexture(scr_tex_id);
        scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
        scr_tex_width  = image.width();
        scr_tex_height = image.height();
        bFirstDraw    = true;

        /*
        switch(imageType)
        {
        case(HEIGHT_TEXTURE):
        case(OCCLUSION_TEXTURE):
            GLCHK( FBOImages::create(fbo, image.width(), image.height(), GL_R16F) );
            break;
        default:
            GLCHK( FBOImages::create(fbo, image.width(), image.height()) );
            break;
        }
        */

        GLuint internal_format = TEXTURE_FORMAT;
        if(imageType == HEIGHT_TEXTURE)
            internal_format = TEXTURE_3DRENDER_FORMAT;
        GLCHK(FBOImages::create(fbo , image.width(), image.height(),internal_format));
    }

    void updateSrcTexId(QGLFramebufferObject* in_ref_fbo)
    {
        glWidget_ptr->makeCurrent();
        if(glIsTexture(scr_tex_id))
            glWidget_ptr->deleteTexture(scr_tex_id);
        QImage image = in_ref_fbo->toImage();
        scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
    }

    void resizeFBO(int width, int height)
    {
        GLuint internal_format = TEXTURE_FORMAT;
        if(imageType == HEIGHT_TEXTURE)
            internal_format = TEXTURE_3DRENDER_FORMAT;
        GLCHK( FBOImages::resize(fbo,width,height,internal_format) );
        bFirstDraw = true;
    }

    // Convert FBO image to QImage
    QImage getImage()
    {
        glWidget_ptr->makeCurrent();
        return fbo->toImage();
    }

    ~FBOImageProporties()
    {
        if(glWidget_ptr != NULL)
        {
            qDebug() << Q_FUNC_INFO;
            glWidget_ptr->makeCurrent();

            if(glIsTexture(normalMixerInputTexId))
                glWidget_ptr->deleteTexture(normalMixerInputTexId);
            if(glIsTexture(scr_tex_id))
                GLCHK( glWidget_ptr->deleteTexture(scr_tex_id) );
            normalMixerInputTexId = 0;
            scr_tex_id = 0;
            glWidget_ptr = NULL;
            //qDebug() << "p=" << properties;
            if(properties != NULL ) delete properties;
            if(fbo        != NULL ) delete fbo;
            properties = NULL;
            fbo        = NULL;
        }
    }
};

#endif // COMMONOBJECTS_H
