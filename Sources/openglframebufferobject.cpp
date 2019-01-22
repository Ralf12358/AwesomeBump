#include "openglframebufferobject.h"

#include <QOpenGLFramebufferObjectFormat>

#include "fboimageproperties.h"
#include "qopenglerrorcheck.h"

bool OpenGLFramebufferObject::bUseLinearInterpolation = true;

OpenGLFramebufferObject::OpenGLFramebufferObject(int width, int height) :
    m_width(width), m_height(height), m_failed(false)
{
    initializeOpenGLFunctions();

    QOpenGLFramebufferObjectFormat format;
    format.setInternalTextureFormat(TEXTURE_3DRENDER_FORMAT);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setMipmap(true);
    format.setAttachment(QOpenGLFramebufferObject::Depth);
    fbo = new QOpenGLFramebufferObject(width, height, format);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLCHK(glBindTexture(GL_TEXTURE_2D, 0));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

OpenGLFramebufferObject::~OpenGLFramebufferObject()
{
    fbo->release();

    for(int i = 0; i < attachments.size(); i++)
    {
        glDeleteTextures(1, &attachments[i]);
    }
    attachments.erase(attachments.begin(), attachments.end());

    delete fbo;
}


bool OpenGLFramebufferObject::isComplete()
{
    return GL_FRAMEBUFFER_COMPLETE == glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

void OpenGLFramebufferObject::bind()
{
    fbo->bind();
}

void OpenGLFramebufferObject::bindDefault()
{
    fbo->bindDefault();
}

bool OpenGLFramebufferObject::addTexture(GLenum COLOR_ATTACHMENTn)
{
    GLuint tex[1];
    GLCHK(glGenTextures(1, &tex[0]));
    glBindTexture(GL_TEXTURE_2D, tex[0]);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);

    glTexImage2D(GL_TEXTURE_2D, 0, TEXTURE_3DRENDER_FORMAT, fbo->width(), fbo->height(), 0,GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    if(!glIsTexture(tex[0]))
    {
        qDebug() << "Error: Cannot create additional texture. Process stopped." << endl;
        return false;
    }
    GLCHK(fbo->bind());

    glFramebufferTexture2D(GL_FRAMEBUFFER, COLOR_ATTACHMENTn,GL_TEXTURE_2D, tex[0], 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE){
        qDebug() << "Cannot add new texture to current FBO! FBO is incomplete.";
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // Switch back to window-system-provided framebuffer.
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    attachments.push_back(tex[0]);
    return true;
}

const GLuint& OpenGLFramebufferObject::getAttachedTexture(GLuint index)
{
    return attachments[index];
}

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
