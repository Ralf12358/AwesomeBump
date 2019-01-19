#ifndef FBOIMAGES_H
#define FBOIMAGES_H

#include <QGLFramebufferObject>

#define TEXTURE_FORMAT GL_RGB16F

class FBOImages
{
public:
    static void create(
            QGLFramebufferObject *&fbo,
            int width,
            int height,
            GLuint internal_format = TEXTURE_FORMAT);

    static void resize(
            QGLFramebufferObject *&src,
            QGLFramebufferObject *&ref,
            GLuint internal_format = TEXTURE_FORMAT);

    static void resize(
            QGLFramebufferObject *&src,
            int width,
            int height,
            GLuint internal_format = TEXTURE_FORMAT);

    static bool bUseLinearInterpolation;
};

#endif // FBOIMAGES_H
