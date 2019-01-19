#include "fboimageproperties.h"

#include <QDebug>

#include "qopenglerrorcheck.h"
#include "fboimages.h"

FBOImageProperties::FBOImageProperties()
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

FBOImageProperties::~FBOImageProperties()
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

void FBOImageProperties::copySettings(FBOImageProperties &src)
{
    bFirstDraw         = src.bFirstDraw;
    conversionHNDepth  = src.conversionHNDepth;
    bConversionBaseMap = src.bConversionBaseMap;
    inputImageType     = src.inputImageType;

    if(properties != NULL && src.properties != NULL )
        properties->copyValues(src.properties);
}

void FBOImageProperties::init(QImage& image)
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

void FBOImageProperties::updateSrcTexId(QGLFramebufferObject* in_ref_fbo)
{
    glWidget_ptr->makeCurrent();
    if(glIsTexture(scr_tex_id))
        glWidget_ptr->deleteTexture(scr_tex_id);
    QImage image = in_ref_fbo->toImage();
    scr_tex_id = glWidget_ptr->bindTexture(image,GL_TEXTURE_2D);
}

void FBOImageProperties::resizeFBO(int width, int height)
{
    GLuint internal_format = TEXTURE_FORMAT;
    if(imageType == HEIGHT_TEXTURE)
        internal_format = TEXTURE_3DRENDER_FORMAT;
    GLCHK( FBOImages::resize(fbo,width,height,internal_format) );
    bFirstDraw = true;
}

QImage FBOImageProperties::getImage()
{
    glWidget_ptr->makeCurrent();
    return fbo->toImage();
}
