#ifndef OPENGLTEXTURECUBE_H
#define OPENGLTEXTURECUBE_H

#include <QOpenGLFunctions>
#include <QOpenGLTexture>
#include <QRgb>

class OpenGLTextureCube : protected QOpenGLFunctions
{
public:
    explicit OpenGLTextureCube(const QStringList& fileNames, int size = 0);
    virtual ~OpenGLTextureCube();

    void bind();
    void unbind();
    bool isCreated() const;
    int mipLevels();

private:
    void createTexture(int size);

    QOpenGLTexture *texture;
};

#endif // OPENGLTEXTURECUBE_H
