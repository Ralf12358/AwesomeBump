#include "openglframebufferobject.h"

#include <QOpenGLFramebufferObjectFormat>

#include "openglerrorcheck.h"

bool OpenGLFramebufferObject::bUseLinearInterpolation = true;

void OpenGLFramebufferObject::create(
        QOpenGLFramebufferObject *&fbo,
        int width,
        int height,
        GLuint internalTextureFormat)
{
    if(fbo)
    {
        fbo->release();
        delete fbo;
    }

    QOpenGLFramebufferObjectFormat format;
    format.setInternalTextureFormat(internalTextureFormat);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setMipmap(true);

    fbo = new QOpenGLFramebufferObject(width, height, format);
//    glBindTexture(GL_TEXTURE_2D, fbo->texture());
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
//    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if(OpenGLFramebufferObject::bUseLinearInterpolation)
    {
//        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
//        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
//        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

//    float aniso = 0.0f;
//    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
//    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);

//    GLCHK(glBindTexture(GL_TEXTURE_2D, 0));
    qDebug() << "OpenGLFramebufferObject::creating new FBO(" << width << "," << height << ") with id=" << fbo->texture();
}

void OpenGLFramebufferObject::resize(
        QOpenGLFramebufferObject *&src,
        QOpenGLFramebufferObject *&ref,
        GLuint internalTextureFormat)
{
    resize(src, ref->width(), ref->height(), internalTextureFormat);
}

void OpenGLFramebufferObject::resize(
        QOpenGLFramebufferObject *&src,
        int width,
        int height,
        GLuint internalTextureFormat)
{
    if(src == NULL)
    {
        GLCHK(OpenGLFramebufferObject::create(src, width, height, internalTextureFormat));
    }
    else if(width == src->width() && height == src->height())
    {}
    else
    {
        GLCHK(OpenGLFramebufferObject::create(src, width, height, internalTextureFormat));
    }
}
