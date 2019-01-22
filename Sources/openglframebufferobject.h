#ifndef OPENGLFRAMEBUFFEROBJECT_H
#define OPENGLFRAMEBUFFEROBJECT_H

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QVector>

#define TEXTURE_FORMAT GL_RGB16F

class OpenGLFramebufferObject : protected QOpenGLFunctions
{
public:
    OpenGLFramebufferObject(int width, int height);
    virtual ~OpenGLFramebufferObject();

    virtual bool failed() const {return m_failed;}

    bool isComplete();
    void bind();
    void bindDefault();
    bool addTexture(GLenum COLOR_ATTACHMENTn);
    const GLuint& getAttachedTexture(GLuint index);

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

    QOpenGLFramebufferObject *fbo;

    //friend class GLRenderTargetCube;

protected:
    int m_width, m_height;
    bool m_failed;
    QVector<GLuint> attachments;
};

#endif // OPENGLFRAMEBUFFEROBJECT_H
