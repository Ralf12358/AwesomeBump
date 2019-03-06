#include "opengltexturecube.h"

#include "openglerrorcheck.h"

OpenGLTextureCube::OpenGLTextureCube(const QStringList& fileNames, int size)
{
    initializeOpenGLFunctions();

    QImage image[6];
    int images = 0;
    for (int i = 0; i < 6; i++)
    {
        ++images;
        image[i] = QImage(fileNames[i]);
        //image[i] = image[i].convertToFormat(QImage::Format_ARGB32);
        if (size <= 0)
            size = image[i].width();
        if (image[i].width() != size || image[i].height() != size)
            image[i] = image[i].scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    createTexture(size);

    texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[0].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeX, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[1].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveY, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[2].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeY, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[3].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[4].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[5].bits());
}

OpenGLTextureCube::~OpenGLTextureCube()
{
    delete texture;
}

void OpenGLTextureCube::bind()
{
    texture->bind();
}

void OpenGLTextureCube::unbind()
{
    texture->release();
}

bool OpenGLTextureCube::isCreated() const
{
    return texture->isCreated();
}

int OpenGLTextureCube::mipLevels()
{
    return texture->mipLevels();
}

void OpenGLTextureCube::createTexture(int size)
{
    texture = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);
    texture->setSize(size, size);
    texture->setFormat(QOpenGLTexture::RGB32F);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    texture->setMipLevels(floor(log2(size)));
    texture->allocateStorage();
}
