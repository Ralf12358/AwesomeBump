#ifndef OPENGLFRAMEBUFFEROBJECT_H
#define OPENGLFRAMEBUFFEROBJECT_H

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

class OpenGLFramebufferObject : protected QOpenGLFunctions
{
public:
    static void create(
            QOpenGLFramebufferObject *&fbo,
            int width,
            int height,
            GLuint internal_format = GL_RGB16F);

    static void resize(
            QOpenGLFramebufferObject *&src,
            QOpenGLFramebufferObject *&ref,
            GLuint internal_format = GL_RGB16F);

    static void resize(
            QOpenGLFramebufferObject *&src,
            int width,
            int height,
            GLuint internal_format = GL_RGB16F);

    static bool bUseLinearInterpolation;
};

#endif // OPENGLFRAMEBUFFEROBJECT_H
