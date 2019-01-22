#ifndef OPENGLTEXTURECUBE_H
#define OPENGLTEXTURECUBE_H

#include <QOpenGLExtraFunctions>
#include <QRgb>

class OpenGLTextureCube : protected QOpenGLExtraFunctions
{
public:
    explicit OpenGLTextureCube(int size);
    explicit OpenGLTextureCube(const QStringList& fileNames, int size = 0);
    virtual ~OpenGLTextureCube();

    void bind();
    void bindFBO();
    void unbind();
    bool failed() const;
    void load(int size, int face, QRgb *data);
    int textureCalcLevels(GLenum target);

    int numMipmaps;

protected:
    GLuint m_texture;
    GLuint fbo;
    bool m_failed;
};

#endif // OPENGLTEXTURECUBE_H
