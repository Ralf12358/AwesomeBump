#include "fboimages.h"

#include "qopenglerrorcheck.h"

bool FBOImages::bUseLinearInterpolation = true;

void FBOImages::create(QGLFramebufferObject *&fbo,int width,int height,GLuint internal_format)
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

void FBOImages::resize(QGLFramebufferObject *&src,QGLFramebufferObject *&ref,GLuint internal_format)
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

void FBOImages::resize(QGLFramebufferObject *&src,int width, int height,GLuint internal_format)
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
