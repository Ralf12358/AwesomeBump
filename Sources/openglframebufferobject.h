#ifndef OPENGLFRAMEBUFFEROBJECT_H
#define OPENGLFRAMEBUFFEROBJECT_H

#include <QOpenGLFramebufferObject>

#define TEXTURE_FORMAT GL_RGB16F

class OpenGLFramebufferObject
{
public:
    static void create(
            QOpenGLFramebufferObject *&fbo,
            int width,
            int height,
            GLuint internal_format = TEXTURE_FORMAT);

    static void resize(
            QOpenGLFramebufferObject *&src,
            QOpenGLFramebufferObject *&ref,
            GLuint internal_format = TEXTURE_FORMAT);

    static void resize(
            QOpenGLFramebufferObject *&src,
            int width,
            int height,
            GLuint internal_format = TEXTURE_FORMAT);

    static bool bUseLinearInterpolation;
};

#endif // OPENGLFRAMEBUFFEROBJECT_H
