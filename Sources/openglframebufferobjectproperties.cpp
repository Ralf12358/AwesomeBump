#include "openglframebufferobjectproperties.h"

#include <QDebug>

#include "openglerrorcheck.h"
#include "openglframebufferobject.h"

SeamlessMode OpenGLFramebufferObjectProperties::seamlessMode                 = SEAMLESS_NONE;
float OpenGLFramebufferObjectProperties::seamlessSimpleModeRadius            = 0.5;
float OpenGLFramebufferObjectProperties::seamlessContrastPower               = 0.0;
float OpenGLFramebufferObjectProperties::seamlessContrastStrenght            = 0.0;
int OpenGLFramebufferObjectProperties::seamlessSimpleModeDirection           = 0; // xy
SourceImageType OpenGLFramebufferObjectProperties::seamlessContrastInputType = INPUT_FROM_HEIGHT_INPUT;
bool OpenGLFramebufferObjectProperties::bSeamlessTranslationsFirst           = true;
int OpenGLFramebufferObjectProperties::seamlessMirroModeType                 = 0;
bool OpenGLFramebufferObjectProperties::bConversionBaseMap                   = false;
bool OpenGLFramebufferObjectProperties::bConversionBaseMapShowHeightTexture  = false;
int OpenGLFramebufferObjectProperties::currentMaterialIndeks                 = MATERIALS_DISABLED;
RandomTilingMode OpenGLFramebufferObjectProperties::seamlessRandomTiling     = RandomTilingMode();

OpenGLFramebufferObjectProperties::OpenGLFramebufferObjectProperties()
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

OpenGLFramebufferObjectProperties::~OpenGLFramebufferObjectProperties()
{
    if(glWidget_ptr != NULL)
    {
        qDebug() << Q_FUNC_INFO;
        glWidget_ptr->makeCurrent();

        if(normalMixerInputTexId)
            delete normalMixerInputTexId;
        if(scr_tex_id)
            delete scr_tex_id;
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

void OpenGLFramebufferObjectProperties::copySettings(OpenGLFramebufferObjectProperties &src)
{
    bFirstDraw         = src.bFirstDraw;
    conversionHNDepth  = src.conversionHNDepth;
    bConversionBaseMap = src.bConversionBaseMap;
    inputImageType     = src.inputImageType;

    if(properties != NULL && src.properties != NULL )
        properties->copyValues(src.properties);
}

void OpenGLFramebufferObjectProperties::init(QImage& image)
{
    qDebug() << Q_FUNC_INFO;

    glWidget_ptr->makeCurrent();
    if(scr_tex_id)
        delete scr_tex_id;
    //scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
    scr_tex_id = new QOpenGLTexture(image);
    scr_tex_width  = image.width();
    scr_tex_height = image.height();
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
    if(imageType == HEIGHT_TEXTURE)
        internal_format = TEXTURE_3DRENDER_FORMAT;
    GLCHK(OpenGLFramebufferObject::create(fbo , image.width(), image.height(),internal_format));
}

void OpenGLFramebufferObjectProperties::updateSrcTexId(QOpenGLFramebufferObject* in_ref_fbo)
{
    glWidget_ptr->makeCurrent();
    if(scr_tex_id)
        delete scr_tex_id;
    QImage image = in_ref_fbo->toImage();
    //scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
    scr_tex_id = new QOpenGLTexture(image);
}

void OpenGLFramebufferObjectProperties::resizeFBO(int width, int height)
{
    GLuint internal_format = TEXTURE_FORMAT;
    if(imageType == HEIGHT_TEXTURE)
        internal_format = TEXTURE_3DRENDER_FORMAT;
    GLCHK( OpenGLFramebufferObject::resize(fbo,width,height,internal_format) );
    bFirstDraw = true;
}

QImage OpenGLFramebufferObjectProperties::getImage()
{
    glWidget_ptr->makeCurrent();
    return fbo->toImage();
}
