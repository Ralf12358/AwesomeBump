#include "opengltexturecube.h"

#include "openglerrorcheck.h"

OpenGLTextureCube::OpenGLTextureCube(int size) : m_texture(0), m_failed(false)
{
    initializeOpenGLFunctions();
    fbo = 0;
    GLCHK( glGenTextures(1, &m_texture) );

    GLCHK( glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture) );

    for (int i = 0; i < 6; ++i)
        GLCHK( glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, size, size, 0,
                     GL_BGRA, GL_UNSIGNED_BYTE, 0) );

    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    //GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR) );
    //GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE) );
    //GLCHK( glGenerateMipmap(GL_TEXTURE_CUBE_MAP) );
    GLCHK( glBindTexture(GL_TEXTURE_CUBE_MAP, 0) );

    // from http://stackoverflow.com/questions/462721/rendering-to-cube-map
    // framebuffer object
    GLCHK( glGenFramebuffers   (1, &fbo) );
    GLCHK( glBindFramebuffer   (GL_FRAMEBUFFER, fbo) );
    GLCHK( glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0) );
    GLCHK( glDrawBuffer        (GL_COLOR_ATTACHMENT0) );

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
    {
        GLCHK( glDeleteFramebuffers(1, &fbo) );
        fbo = 0;
    }
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER,0) );
}

OpenGLTextureCube::OpenGLTextureCube(const QStringList& fileNames, int size) : m_texture(0), m_failed(false)
{
    initializeOpenGLFunctions();
    fbo = 0;
    glGenTextures(1, &m_texture);

    // TODO: Add error handling.

    GLCHK( glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture) );

    int index = 0;
    foreach (QString file, fileNames) {
        QImage image(file);
        if (image.isNull()) {
            m_failed = true;
            break;
        }

        image = image.convertToFormat(QImage::Format_ARGB32);

        //qDebug() << "Image size:" << image.width() << "x" << image.height();
        if (size <= 0)
            size = image.width();
        if (size != image.width() || size != image.height())
            image = image.scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

        // Works on x86, so probably works on all little-endian systems.
        // Does it work on big-endian systems?
        GLCHK(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, 4, image.width(), image.height(), 0,
                           GL_BGRA, GL_UNSIGNED_BYTE, image.bits()) );

        if (++index == 6)
            break;
    }

    // Clear remaining faces.
    while (index < 6) {
        GLCHK( glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + index, 0, GL_RGB, size, size, 0,
                           GL_BGRA, GL_UNSIGNED_BYTE, 0) );
        ++index;
    }

    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    //glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR) );
    GLCHK( glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_GENERATE_MIPMAP, GL_TRUE) );
    GLCHK( glGenerateMipmap(GL_TEXTURE_CUBE_MAP) );


    // get number of mip maps
    if ( (numMipmaps = textureCalcLevels(GL_TEXTURE_CUBE_MAP_POSITIVE_X)) == -1 ) {
        int max_level;
        glGetTexParameteriv( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &max_level );
        numMipmaps = 1 + floor(log2(size > max_level?size:max_level) );
    }

    GLCHK( glBindTexture(GL_TEXTURE_CUBE_MAP, 0) );
    qDebug() << "Generated number of mipmaps:" << numMipmaps;
}

OpenGLTextureCube::~OpenGLTextureCube()
{
    glDeleteTextures(1, &m_texture);
    if(fbo != 0 ) glDeleteFramebuffers(1, &fbo);
}

void OpenGLTextureCube::bind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
    glEnable(GL_TEXTURE_CUBE_MAP);
}

void OpenGLTextureCube::bindFBO(){
    glBindFramebuffer   (GL_FRAMEBUFFER, fbo);
    glDrawBuffer        (GL_COLOR_ATTACHMENT0); // important!
}


void OpenGLTextureCube::unbind()
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    glDisable(GL_TEXTURE_CUBE_MAP);
}

bool OpenGLTextureCube::failed() const
{
    return m_failed;
}

void OpenGLTextureCube::load(int size, int face, QRgb *data)
{
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_texture);
    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, 4, size, size, 0,
                 GL_BGRA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

int OpenGLTextureCube::textureCalcLevels(GLenum target)
{
    int max_level;
    GLCHK(glGetTexParameteriv( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &max_level ));
    int max_mipmap = -1;
    for ( int i = 0; i < max_level; ++i ) {
        int width;
        GLCHK(glGetTexLevelParameteriv( target, i, GL_TEXTURE_WIDTH, &width ));
        if ( 0 == width || GL_INVALID_VALUE == width) {
            max_mipmap = i - 1;
            break;
        }
    }
    return max_mipmap;
}
