#include "opengl2dimagewidget.h"

#include "openglerrorcheck.h"

OpenGL2DImageWidget::OpenGL2DImageWidget(QWidget *parent) :
    QOpenGLWidget(parent), imageWidget(new ImageWidget*[TEXTURES]()),
    activeTextureType(DIFFUSE_TEXTURE),
    textures(new QOpenGLTexture*[TEXTURES]()), normalMixerInputTexture(0),
    textureFBOs(new QOpenGLFramebufferObject*[TEXTURES]()),
    conversionHNDepth(2.0),
    averageColorFBO(0), samplerFBO1(0), samplerFBO2(0),
    auxFBO1(0), auxFBO2(0), auxFBO3(0), auxFBO4(0),
    paintFBO(0), renderFBO(0),
    conversionType(CONVERT_NONE), uvManilupationMethod(UV_TRANSLATE),
    bShadowRender(false), bSkipProcessing(false), fboRatio(1),
    cornerWeights(QVector4D(0, 0, 0, 0)), draggingCorner(-1),
    gui_perspective_mode(0), gui_seamless_mode(0),
    bToggleColorPicking(false), bRendering(false),
    mouseUpdateIsQueued(false), blockMouseMovement(false), wrapMouse(true)
{
    // Initialise position of the corners.
    cornerPositions[0] = QVector2D(-0.0,-0);
    cornerPositions[1] = QVector2D( 1,-0);
    cornerPositions[2] = QVector2D( 1, 1);
    cornerPositions[3] = QVector2D(-0, 1);
    for(int i = 0; i < 4; i++)
    {
        grungeCornerPositions[i] = cornerPositions[i];
    }
    setCursor(Qt::OpenHandCursor);
    cornerCursors[0] = QCursor(QPixmap(":/resources/cursors/corner1.png"));
    cornerCursors[1] = QCursor(QPixmap(":/resources/cursors/corner2.png"));
    cornerCursors[2] = QCursor(QPixmap(":/resources/cursors/corner3.png"));
    cornerCursors[3] = QCursor(QPixmap(":/resources/cursors/corner4.png"));
}

OpenGL2DImageWidget::~OpenGL2DImageWidget()
{
    makeCurrent();
    averageColorFBO->bindDefault();
    typedef std::map<std::string,QOpenGLShaderProgram*>::iterator it_type;
    qDebug() << "Removing OpenGLImageEditor filters:";
    for(it_type iterator = filter_programs.begin(); iterator != filter_programs.end(); iterator++)
    {
        qDebug() << "Removing filter:" << QString(iterator->first.c_str());
        delete iterator->second;
    }
    delete averageColorFBO;
    delete samplerFBO1;
    delete samplerFBO2;
    delete auxFBO1;
    delete auxFBO2;
    delete auxFBO3;
    delete auxFBO4;

    for(int i = 0; i < 3 ; i++)
    {
        delete auxFBO0BMLevels[i] ;
        delete auxFBO1BMLevels[i] ;
        delete auxFBO2BMLevels[i] ;
    }
    delete  paintFBO;
    delete renderFBO;

    doneCurrent();
}

QSize OpenGL2DImageWidget::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize OpenGL2DImageWidget::sizeHint() const
{
    return QSize(500, 400);
}

void OpenGL2DImageWidget::setImageWidget(TextureType textureType, ImageWidget* imageWidget)
{
    this->imageWidget[textureType] = imageWidget;
}

TextureType OpenGL2DImageWidget::getActiveTextureType() const
{
    return activeTextureType;
}

void OpenGL2DImageWidget::setActiveTextureType(TextureType textureType)
{
    activeTextureType = textureType;
    update();
}

void OpenGL2DImageWidget::setTextureImage(TextureType textureType, const QImage& image)
{
    delete textures[textureType];
    textures[textureType] = new QOpenGLTexture(image);
}

int OpenGL2DImageWidget::getTextureWidth(TextureType textureType)
{
    return textureFBOs[textureType]->width();
}

int OpenGL2DImageWidget::getTextureHeight(TextureType textureType)
{
    return  textureFBOs[textureType]->height();
}

GLuint OpenGL2DImageWidget::getTextureId(TextureType textureType)
{
    return textureFBOs[textureType]->texture();
}

QImage OpenGL2DImageWidget::getTextureFBOImage(TextureType textureType)
{
    return textureFBOs[textureType]->toImage();
}

void OpenGL2DImageWidget::setNormalMixerInputTexture(const QImage& image)
{
    if(normalMixerInputTexture) delete normalMixerInputTexture;
    normalMixerInputTexture = new QOpenGLTexture(image);
}

void OpenGL2DImageWidget::setSkipProcessing(TextureType textureType, bool skipProcessing)
{
    this->skipProcessing[textureType] = skipProcessing;
}

void OpenGL2DImageWidget::setConversionHNDepth(float newDepth)
{
    conversionHNDepth = newDepth;
}

void OpenGL2DImageWidget::enableShadowRender(bool enable)
{
    bShadowRender = enable;
}

ConversionType OpenGL2DImageWidget::getConversionType()
{
    return conversionType;
}

void OpenGL2DImageWidget::setConversionType(ConversionType type)
{
    conversionType = type ;
}

void OpenGL2DImageWidget::updateCornersPosition(QVector2D dc1, QVector2D dc2, QVector2D dc3, QVector2D dc4)
{
    cornerPositions[0] = QVector2D(0,0) + dc1;
    cornerPositions[1] = QVector2D(1,0) + dc2;
    cornerPositions[2] = QVector2D(1,1) + dc3;
    cornerPositions[3] = QVector2D(0,1) + dc4;

    for(int i = 0 ; i < 4 ; i++)
    {
        grungeCornerPositions[i] = cornerPositions[i];
    }
    update();
}

void OpenGL2DImageWidget::resizeFBO(int width, int height)
{
    conversionType = CONVERT_RESIZE;
    resize_width   = width;
    resize_height  = height;
}

void OpenGL2DImageWidget::imageChanged()
{
    // Restore picking state when another property was changed.
    bToggleColorPicking = false;
}

void OpenGL2DImageWidget::resetView()
{
    makeCurrent();

    zoom = 0;
    windowRatio = float(width()) / height();
    fboRatio    = float(textureFBOs[activeTextureType]->width()) /
                        textureFBOs[activeTextureType]->height();
    // OpenGL window dimensions.
    orthographicProjHeight = (1 + zoom) / windowRatio;
    orthographicProjWidth = (1 + zoom) / fboRatio;

    if(orthographicProjWidth < 1.0)
    {
        // Fit x direction.
        zoom = fboRatio - 1;
        orthographicProjHeight = (1 + zoom) / windowRatio;
        orthographicProjWidth = (1 + zoom) / fboRatio;
    }

    if(orthographicProjHeight < 1.0)
    {
        // Fit y direction.
        zoom = windowRatio - 1;
        orthographicProjHeight = (1 + zoom) / windowRatio;
        orthographicProjWidth = (1 + zoom) / fboRatio;
    }

    // Set the image in the center.
    xTranslation = orthographicProjWidth / 2;
    yTranslation = orthographicProjHeight / 2;
}

void OpenGL2DImageWidget::selectPerspectiveTransformMethod(int method)
{
    gui_perspective_mode = method;
    update();
}

void OpenGL2DImageWidget::selectUVManipulationMethod(UVManipulationMethod method)
{
    uvManilupationMethod = method;
    update();
}

void OpenGL2DImageWidget::updateCornersWeights(float w1, float w2, float w3, float w4)
{
    cornerWeights.setX(w1);
    cornerWeights.setY(w2);
    cornerWeights.setZ(w3);
    cornerWeights.setW(w4);
    update();
}

void OpenGL2DImageWidget::selectSeamlessMode(SeamlessMode mode)
{
    Image::seamlessMode = mode;
    update();
}

void OpenGL2DImageWidget::toggleColorPicking(bool toggle)
{
    bToggleColorPicking = toggle;
    ptr_ABColor = NULL;
    if(toggle)
    {
        setCursor(Qt::UpArrowCursor);
    }
    else
    {
        setCursor(Qt::PointingHandCursor);
    }
}

void OpenGL2DImageWidget::pickImageColor(QtnPropertyABColor *property)
{
    bToggleColorPicking = true;
    ptr_ABColor = property;
    setCursor(Qt::UpArrowCursor);
    repaint();
}

void OpenGL2DImageWidget::toggleMouseWrap(bool toggle)
{
    wrapMouse = toggle;
}

void OpenGL2DImageWidget::initializeGL()
{
    qDebug() << Q_FUNC_INFO;

    initializeOpenGLFunctions();

    QColor clearColor = QColor::fromCmykF(0.79, 0.79, 0.79, 0.0).dark();
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());

    // Use multiple fragment samples to compute the final color of a pixel.
    glEnable(GL_MULTISAMPLE);
    // Do depth comparisons and update the depth buffer.
    glEnable(GL_DEPTH_TEST);

    bool successful = true;
    program = new QOpenGLShaderProgram(this);
    successful &= program->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/resources/shaders/filters.vert");
    successful &= program->addShaderFromSourceFile(QOpenGLShader::Fragment, ":/resources/shaders/filters.frag");
    program->bindAttributeLocation("positionIn", 0);
    successful &= program->link();
    successful &= program->bind();
    if (!successful) qDebug() << program->log();

    program->setUniformValue("layerA", 0);
    program->setUniformValue("layerB", 1);
    program->setUniformValue("layerC", 2);
    program->setUniformValue("layerD", 3);
    program->setUniformValue("materialTexture", 10);

    subroutines["mode_normal_filter"]                  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_filter");
    subroutines["mode_color_hue_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_color_hue_filter");
    subroutines["mode_overlay_filter"]                 = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_overlay_filter");
    subroutines["mode_ao_cancellation_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_ao_cancellation_filter");
    subroutines["mode_invert_filter"]                  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_invert_filter");
    subroutines["mode_gauss_filter"]                   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_gauss_filter");
    subroutines["mode_seamless_filter"]                = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_seamless_filter");
    subroutines["mode_seamless_linear_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_seamless_linear_filter");
    subroutines["mode_dgaussians_filter"]              = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_dgaussians_filter");
    subroutines["mode_constrast_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_constrast_filter");
    subroutines["mode_small_details_filter"]           = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_small_details_filter");
    subroutines["mode_gray_scale_filter"]              = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_gray_scale_filter");
    subroutines["mode_medium_details_filter"]          = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_medium_details_filter");
    subroutines["mode_height_to_normal"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_height_to_normal");
    subroutines["mode_sharpen_blur"]                   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_sharpen_blur");
    subroutines["mode_normals_step_filter"]            = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normals_step_filter");
    subroutines["mode_normal_mixer_filter"]            = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_mixer_filter");
    subroutines["mode_invert_components_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_invert_components_filter");
    subroutines["mode_normal_to_height"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_to_height");
    subroutines["mode_sobel_filter"]                   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_sobel_filter");
    subroutines["mode_normal_expansion_filter"]        = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_expansion_filter");
    subroutines["mode_mix_normal_levels_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_mix_normal_levels_filter");
    subroutines["mode_normalize_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normalize_filter");
    subroutines["mode_occlusion_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_occlusion_filter");
    subroutines["mode_combine_normal_height_filter"]   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_combine_normal_height_filter");
    subroutines["mode_perspective_transform_filter"]   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_perspective_transform_filter");
    subroutines["mode_height_processing_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_height_processing_filter");
    subroutines["mode_roughness_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_roughness_filter");
    subroutines["mode_roughness_color_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_roughness_color_filter");
    subroutines["mode_remove_low_freq_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_remove_low_freq_filter");
    subroutines["mode_grunge_filter"]                  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_grunge_filter");
    subroutines["mode_grunge_randomization_filter"]    = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_grunge_randomization_filter");
    subroutines["mode_grunge_normal_warp_filter"]      = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_grunge_normal_warp_filter");
    subroutines["mode_normal_angle_correction_filter"] = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_angle_correction_filter");
    subroutines["mode_add_noise_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_add_noise_filter");

    // Make Screen Quad
    int size = 2;
    float vertexes[3 * size * size];
    int iter = 0;
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            float offset = 0.5;
            float x = i / (size - 1.0);
            float y = j / (size - 1.0);
            vertexes[iter * 3 + 0] = x-offset;
            vertexes[iter * 3 + 1] = y-offset;
            vertexes[iter * 3 + 2] = 0;
            iter++;
        }
    }

    int no_triangles = 2 * (size - 1) * (size - 1);
    unsigned int indices[no_triangles * 3];
    iter = 0;
    for(int i = 0 ; i < size -1 ; i++)
    {
        for(int j = 0 ; j < size -1 ; j++)
        {
            unsigned int i1 = i +      j      * size;
            unsigned int i2 = i +     (j + 1) * size;
            unsigned int i3 = i + 1 +  j      * size;
            unsigned int i4 = i + 1 + (j + 1) * size;
            indices[iter++] = (i1);
            indices[iter++] = (i3);
            indices[iter++] = (i2);
            indices[iter++] = (i2);
            indices[iter++] = (i3);
            indices[iter++] = (i4);
        }
    }

    // Create buffers and store state in vertex array object.
    vertexArray = new QOpenGLVertexArrayObject(this);
    vertexArray->create();
    vertexArray->bind();

    QOpenGLBuffer vertexBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vertexBuffer.allocate(vertexes, sizeof(float) * 3 * size * size);

    QOpenGLBuffer elementBuffer(QOpenGLBuffer::IndexBuffer);
    elementBuffer.create();
    elementBuffer.bind();
    elementBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    elementBuffer.allocate(indices, sizeof(unsigned int) * no_triangles * 3);

    // Assign buffer data to attributes.
    program->setAttributeBuffer("positionIn", GL_FLOAT, 0, 3, 0);
    program->enableAttributeArray("positionIn");

    vertexArray->release();
    program->release();

    averageColorFBO = createFBO(256, 256);
    samplerFBO1     = createFBO(1024, 1024);
    samplerFBO2     = createFBO(1024, 1024);

    textures[DIFFUSE_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_D.png"));
    textures[NORMAL_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_N.png"));
    textures[SPECULAR_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_S.png"));
    textures[HEIGHT_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_H.png"));
    textures[OCCLUSION_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_O.png"));
    textures[ROUGHNESS_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_R.png"));
    textures[METALLIC_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_M.png"));
    textures[GRUNGE_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_R.png"));
    textures[MATERIAL_TEXTURE] = new QOpenGLTexture(QImage(":/resources/logo/logo_R.png"));

    for (unsigned int textureIndex = 0; textureIndex < TEXTURES; ++textureIndex)
    {
        TextureType textureType = static_cast<TextureType>(textureIndex);
        textureFBOs[textureIndex] = createTextureFBO(textures[textureType]->width(), textures[textureType]->height());
    }
}

void OpenGL2DImageWidget::paintGL()
{
    program->bind();
    vertexArray->bind();

    // Perform filters on images and render the final result to renderFBO.
    // Avoid rendering function if there is rendered something already.
    if(!bSkipProcessing && !bRendering)
    {
        bRendering = true;
        render();
    }

    bSkipProcessing = false;
    conversionType  = CONVERT_NONE;

    // Draw current FBO using current image after rendering the paintFBO will be replaced.
    if(!bShadowRender)
    {
        // Since grunge map can be different we need to calculate ratio each time.
        if (paintFBO != NULL)
        {
            fboRatio = float(paintFBO->width()) / paintFBO->height();
            orthographicProjHeight = (1 + zoom) / windowRatio;
            orthographicProjWidth  = (1 + zoom) / fboRatio;
        }
        GLCHK( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
        GLCHK( glDisable(GL_CULL_FACE) );
        GLCHK( glDisable(GL_DEPTH_TEST) );

        // Positions.
        //GLCHK( glBindBuffer(GL_ARRAY_BUFFER, vbos[0]) );
        //GLCHK( glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, (void*)0) );
        // Indices
        //GLCHK( glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[2]) );

        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );

        // Displaying new image
        GLCHK( paintFBO->bindDefault() );
        GLCHK( program->setUniformValue("quad_draw_mode", 1) );

        GLCHK( glViewport(0,0,width(),height()) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, paintFBO->texture()) );

        QMatrix4x4 matrix;
        matrix.ortho(0, orthographicProjWidth, 0, orthographicProjHeight, -1, 1);
        GLCHK( program->setUniformValue("ProjectionMatrix", matrix) );
        matrix.setToIdentity();
        matrix.translate(xTranslation, yTranslation, 0);
        GLCHK( program->setUniformValue("ModelViewMatrix", matrix) );
        GLCHK( program->setUniformValue("material_id", int(-1)) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        GLCHK( program->setUniformValue("quad_draw_mode", int(0)) );
    }

    vertexArray->release();
    program->release();
}

void OpenGL2DImageWidget::resizeGL(int width, int height)
{
    windowRatio = float(width) / height;
    GLCHK( glViewport(0, 0, width, height) );
    fboRatio = float(textureFBOs[activeTextureType]->width()) /
                     textureFBOs[activeTextureType]->height();
    orthographicProjHeight = (1 + zoom) / windowRatio;
    orthographicProjWidth = (1 + zoom) / fboRatio;

    resetView();
}

void OpenGL2DImageWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent( event );
    resetView();
}

void OpenGL2DImageWidget::mousePressEvent(QMouseEvent *event)
{
    lastCursorPos = event->pos();

    // Reset the mouse handling state with, to avoid a bad state
    blockMouseMovement = false;
    mouseUpdateIsQueued = false;

    bSkipProcessing = true;
    draggingCorner = -1;

    // Change cursor.
    if (event->buttons() & Qt::RightButton)
    {
        setCursor(Qt::ClosedHandCursor);
    }
    update();

    // In case of color picking: emit and stop picking
    if(bToggleColorPicking)
    {
        std::vector<unsigned char> pixels(1 * 1 * 4);
        GLCHK( glReadPixels(event->pos().x(), height()-event->pos().y(), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]) );
        QVector4D color(pixels[0], pixels[1], pixels[2], pixels[3]);
        qDebug() << "Picked pixel (" << event->pos().x() << ", " << height()-event->pos().y() << ") with color:" << color;

        QColor qcolor = QColor(pixels[0], pixels[1], pixels[2]);
        if(ptr_ABColor != NULL)
            ptr_ABColor->setValue(qcolor);
        toggleColorPicking(false);
    }
}

void OpenGL2DImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    draggingCorner = -1;
    event->accept();
    bSkipProcessing = true;
    repaint();
}

void OpenGL2DImageWidget::wheelEvent(QWheelEvent *event)
{
    if(event->delta() > 0)
        zoom -= 0.1;
    else
        zoom += 0.1;
    if(zoom < -0.90)
        zoom = -0.90;

    updateMousePosition();

    windowRatio = float(width())/height();
    if (isValid())
    {
        GLCHK( glViewport(0, 0, width(), height()) );

        fboRatio = float(textureFBOs[activeTextureType]->width()) /
                         textureFBOs[activeTextureType]->height();
        orthographicProjHeight = (1+zoom)/windowRatio;
        orthographicProjWidth = (1+zoom)/fboRatio;
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "invalid context.";
    }

    // Getting the global position of cursor.
    QPoint p = mapFromGlobal(QCursor::pos());

    // Restoring the translation after zooming.
    xTranslation = double(p.x()) / width() * orthographicProjWidth - cursorPhysicalXPosition;
    yTranslation = (1.0 - double(p.y()) / height()) * orthographicProjHeight - cursorPhysicalYPosition;

    update();
}

void OpenGL2DImageWidget::applyNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                          QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyHeightToNormal(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_to_normal"]) );

    GLCHK( program->setUniformValue("gui_hn_conversion_depth", conversionHNDepth) );
    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyColorHueFilter(  QOpenGLFramebufferObject *inputFBO,
                                              QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_color_hue_filter"]) );

    GLCHK( program->setUniformValue("gui_hue"   , float(imageWidget[activeTextureType]->getProperties()->Basic.ColorHue)) );

    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyPerspectiveTransformFilter(  QOpenGLFramebufferObject *inputFBO,
                                                          QOpenGLFramebufferObject *outputFBO)
{
    // When materials texture is enabled, UV transformations are disabled.
    if(Image::materialsEnabled)
    {
        copyFBO(inputFBO,outputFBO);
        return;
    }

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_perspective_transform_filter"]) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );

    if(activeTextureType == GRUNGE_TEXTURE)
    {
        GLCHK( program->setUniformValue("corner1"  , grungeCornerPositions[0]) );
        GLCHK( program->setUniformValue("corner2"  , grungeCornerPositions[1]) );
        GLCHK( program->setUniformValue("corner3"  , grungeCornerPositions[2]) );
        GLCHK( program->setUniformValue("corner4"  , grungeCornerPositions[3]) );
    }
    else
    {
        GLCHK( program->setUniformValue("corner1"  , cornerPositions[0]) );
        GLCHK( program->setUniformValue("corner2"  , cornerPositions[1]) );
        GLCHK( program->setUniformValue("corner3"  , cornerPositions[2]) );
        GLCHK( program->setUniformValue("corner4"  , cornerPositions[3]) );
    }

    GLCHK( program->setUniformValue("corners_weights"  , cornerWeights) );
    GLCHK( program->setUniformValue("uv_scaling_mode", 0) );
    GLCHK( program->setUniformValue("gui_perspective_mode"  , gui_perspective_mode) );

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( inputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( program->setUniformValue("uv_scaling_mode", 1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( inputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyGaussFilter(QOpenGLFramebufferObject *sourceFBO,
                                         QOpenGLFramebufferObject *auxFBO,
                                         QOpenGLFramebufferObject *outputFBO,int no_iter,float w )
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );

    GLCHK( program->setUniformValue("gui_gauss_radius", no_iter) );
    if( w == 0)
    {
        GLCHK( program->setUniformValue("gui_gauss_w", float(no_iter)) );
    }
    else
    {
        GLCHK( program->setUniformValue("gui_gauss_w", float(w)) );
    }

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( program->setUniformValue("gauss_mode",1) );

    GLCHK( auxFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, sourceFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( program->setUniformValue("gauss_mode",0) );
}

void OpenGL2DImageWidget::applyInverseColorFilter(QOpenGLFramebufferObject *inputFBO,
                                                QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_invert_filter"]) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyRemoveShadingFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *aoMaskFBO,
                                                 QOpenGLFramebufferObject *refFBO,
                                                 QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_ao_cancellation_filter"]) );

    GLCHK( program->setUniformValue("gui_remove_shading", RemoveShadingProp.RemoveShadingByGaussian) );
    GLCHK( program->setUniformValue("gui_ao_cancellation", RemoveShadingProp.AOCancellation) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, aoMaskFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE2) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, refFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

void OpenGL2DImageWidget::applyRemoveLowFreqFilter(QOpenGLFramebufferObject *inputFBO,
                                                   QOpenGLFramebufferObject *outputFBO)
{
    applyGaussFilter(inputFBO, samplerFBO1, samplerFBO2, RemoveShadingProp.LowFrequencyFilterRadius * 50);

    // Calculate the average color on CPU.
    // Copy large file to smaller FBO to save time.
    applyNormalFilter(inputFBO, averageColorFBO);

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, averageColorFBO->texture()) );

    GLint textureWidth, textureHeight;
    GLCHK( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH , &textureWidth ) );
    GLCHK( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight) );

    float* img = new float[textureWidth*textureHeight * 3];
    float ave_color[3] = {0, 0, 0};

    GLCHK( glGetTexImage(	GL_TEXTURE_2D,0,GL_RGB,GL_FLOAT,img) );

    for(int i = 0 ; i < textureWidth*textureHeight ; i++)
    {
        for(int c = 0; c < 3; c++)
        {
            ave_color[c] += img[3 * i + c];
        }
    }

    // Normalization sum.
    ave_color[0] /= (textureWidth*textureHeight);
    ave_color[1] /= (textureWidth*textureHeight);
    ave_color[2] /= (textureWidth*textureHeight);
    delete[] img;

    QVector3D aveColor = QVector3D(ave_color[0], ave_color[1], ave_color[2]);

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_remove_low_freq_filter"]) );

    GLCHK( program->setUniformValue("average_color"  , aveColor ) );
    GLCHK( program->setUniformValue("gui_remove_shading_lf_blending"  , RemoveShadingProp.LowFrequencyFilterBlending ) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, samplerFBO2->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyOverlayFilter(QOpenGLFramebufferObject *layerAFBO,
                                           QOpenGLFramebufferObject *layerBFBO,
                                           QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_overlay_filter"]) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, layerAFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, layerBFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );

    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applySeamlessLinearFilter(QOpenGLFramebufferObject *inputFBO,
                                                  QOpenGLFramebufferObject *outputFBO)
{
    // When materials texture is enabled, UV transformations are disabled.
    if(Image::materialsEnabled)
    {
        copyFBO(inputFBO,outputFBO);
        return;
    }
    switch(Image::seamlessContrastImageType)
    {
    default:
    case(INPUT_FROM_HEIGHT_INPUT):
        copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(), auxFBO2);
        break;
    case(INPUT_FROM_DIFFUSE_INPUT):
        copyTex2FBO(textures[DIFFUSE_TEXTURE]->textureId(), auxFBO2);
        break;
    };

    // When translations are applied, first one has to translate.
    // else the contrast mask image
    if(Image::bSeamlessTranslationsFirst)
    {
        // The output is saved to auxFBO1
        applyPerspectiveTransformFilter(auxFBO2,outputFBO);
    }

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO2->texture()) );

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_seamless_linear_filter"]) );

    GLCHK( program->setUniformValue("make_seamless_radius"           , Image::seamlessSimpleModeRadius) );
    GLCHK( program->setUniformValue("gui_seamless_contrast_strenght" , Image::seamlessContrastStrength) );
    GLCHK( program->setUniformValue("gui_seamless_contrast_power"    , Image::seamlessContrastPower) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    switch(Image::seamlessSimpleModeDirection)
    {
    default:
    case(0): //XY
        // Horizontal filtering
        GLCHK( program->setUniformValue("gui_seamless_mode", (int)0) );
        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );

        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( inputFBO->bind() );
        // Vertical filtering.
        GLCHK( program->setUniformValue("gui_seamless_mode", (int)1) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        break;
    case(1): //X
        // Horizontal filtering.
        GLCHK( program->setUniformValue("gui_seamless_mode", (int)0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        inputFBO->bindDefault();
        copyFBO(outputFBO,inputFBO);
        break;
    case(2): //Y
        GLCHK( outputFBO->bind() );
        // Vertical filtering.
        GLCHK( program->setUniformValue("gui_seamless_mode", (int)1) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        copyFBO(outputFBO,inputFBO);
        break;
    }

    inputFBO->bindDefault();
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

void OpenGL2DImageWidget::applySeamlessFilter(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    // When materials texture is enabled, UV transformations are disabled.
    if(Image::materialsEnabled)
    {
        copyFBO(inputFBO,outputFBO);
        return;
    }
    switch(Image::seamlessContrastImageType)
    {
    default:
    case(INPUT_FROM_HEIGHT_INPUT):
        copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(), auxFBO1);
        break;
    case(INPUT_FROM_DIFFUSE_INPUT):
        copyTex2FBO(textures[DIFFUSE_TEXTURE]->textureId(), auxFBO1);
        break;
    };

    // When translations are applied first one has to translate.
    // else the contrast mask image.
    if(Image::bSeamlessTranslationsFirst)
    {
        // The output is save to auxFBO2.
        applyPerspectiveTransformFilter(auxFBO1,outputFBO);
    }

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_seamless_filter"]) );

    GLCHK( program->setUniformValue("make_seamless_radius" , Image::seamlessSimpleModeRadius) );
    GLCHK( program->setUniformValue("gui_seamless_contrast_strenght", Image::seamlessContrastStrength) );
    GLCHK( program->setUniformValue("gui_seamless_contrast_power", Image::seamlessContrastPower) );
    GLCHK( program->setUniformValue("gui_seamless_mode", (int)Image::seamlessMode) );
    GLCHK( program->setUniformValue("gui_seamless_mirror_type", Image::seamlessMirroModeType) );

    // Send random angles.
    QMatrix3x3 random_angles;
    for(int i = 0; i < 9; i++)
        random_angles.data()[i] = Image::randomAngles[i];

    GLCHK( program->setUniformValue("gui_seamless_random_angles" , random_angles) );
    GLCHK( program->setUniformValue("gui_seamless_random_phase" , Image::randomCommonPhase) );
    GLCHK( program->setUniformValue("gui_seamless_random_inner_radius" , Image::randomInnerRadius) );
    GLCHK( program->setUniformValue("gui_seamless_random_outer_radius" , Image::randomOuterRadius) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO1->texture()) );

    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    outputFBO->bindDefault();
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

void OpenGL2DImageWidget::applyDGaussiansFilter(QOpenGLFramebufferObject *inputFBO,
                                              QOpenGLFramebufferObject *auxFBO,
                                              QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );

    GLCHK( program->setUniformValue("gui_gauss_radius", int(SurfaceDetailsProp.Radius)) );
    GLCHK( program->setUniformValue("gui_gauss_w", SurfaceDetailsProp.WeightA) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );

    GLCHK( program->setUniformValue("gauss_mode",1) );

    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( program->setUniformValue("gui_gauss_w", SurfaceDetailsProp.WeightB) );

    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( program->setUniformValue("gauss_mode",1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( inputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]) );

    GLCHK( program->setUniformValue("gui_mode_dgaussian", 1) );
    GLCHK( program->setUniformValue("gui_specular_amplifier", SurfaceDetailsProp.Amplifier) );

    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( program->setUniformValue("gauss_mode",0) );

    copyFBO(auxFBO,outputFBO);
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyContrastFilter(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_constrast_filter"]) );

    GLCHK( program->setUniformValue("gui_specular_contrast", SurfaceDetailsProp.Contrast) );
    // Not used since offset does the same.
    GLCHK( program->setUniformValue("gui_specular_brightness", 0.0f) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault());
}

void OpenGL2DImageWidget::applySmallDetailsFilter(QOpenGLFramebufferObject *inputFBO,
                                                QOpenGLFramebufferObject *auxFBO,
                                                QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );

    GLCHK( program->setUniformValue("gui_depth", BasicProp.DetailDepth) );
    GLCHK( program->setUniformValue("gui_gauss_radius", int(3.0)) );
    GLCHK( program->setUniformValue("gui_gauss_w", float(3.0)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( program->setUniformValue("gauss_mode",1) );

    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]) );

    GLCHK( program->setUniformValue("gui_mode_dgaussian", 0) );

    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( program->setUniformValue("gauss_mode",0) );
    GLCHK( program->setUniformValue("gui_mode_dgaussian", 1) );

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_small_details_filter"]) );

    GLCHK( program->setUniformValue("gui_small_details", BasicProp.SmallDetails) );
    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault() );
    GLCHK( program->setUniformValue("gui_depth", float(1.0)) );
}

void OpenGL2DImageWidget::applyMediumDetailsFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *auxFBO,
                                                 QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );

    GLCHK( program->setUniformValue("gui_depth", BasicProp.DetailDepth) );
    GLCHK( program->setUniformValue("gui_gauss_radius", int(15.0)) );
    GLCHK( program->setUniformValue("gui_gauss_w", float(15.0)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( program->setUniformValue("gauss_mode",1) );

    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( program->setUniformValue("gauss_mode",1) );
    GLCHK( program->setUniformValue("gui_gauss_radius", int(20.0)) );
    GLCHK( program->setUniformValue("gui_gauss_w"     , float(20.0)) );

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_medium_details_filter"]) );

    program->setUniformValue("gui_small_details", BasicProp.MediumDetails);
    glViewport(0,0,inputFBO->width(),inputFBO->height());
    GLCHK( auxFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault() );
    GLCHK( program->setUniformValue("gui_depth", float(1.0)) );
    copyFBO(auxFBO, outputFBO);
}

void OpenGL2DImageWidget::applyGrayScaleFilter(QOpenGLFramebufferObject *inputFBO,
                                             QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gray_scale_filter"]) );

    // There is a change if gray scale filter is used for convertion from diffuse
    // texture to others in any other case this filter works just a simple gray scale filter.
    // Check if the baseMapToOthers is enabled, if yes check is min and max values are defined.
    GLCHK( program->setUniformValue("gui_gray_scale_max_color_defined",false) );
    GLCHK( program->setUniformValue("gui_gray_scale_min_color_defined",false) );

    if(imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.EnableConversion)
    {
        if(QColor(BaseMapToOthersProp.MaxColor.value()).red() >= 0)
        {
            QColor color = QColor(BaseMapToOthersProp.MaxColor.value());
            QVector3D dcolor(color.redF(),color.greenF(),color.blueF());
            GLCHK( program->setUniformValue("gui_gray_scale_max_color_defined",true) );
            GLCHK( program->setUniformValue("gui_gray_scale_max_color",dcolor) );
        }
        if(QColor(BaseMapToOthersProp.MinColor.value()).red() >= 0)
        {
            QColor color = QColor(BaseMapToOthersProp.MinColor.value());
            QVector3D dcolor(color.redF(),color.greenF(),color.blueF());
            GLCHK( program->setUniformValue("gui_gray_scale_min_color_defined",true) );
            GLCHK( program->setUniformValue("gui_gray_scale_min_color",dcolor) );
        }
        GLCHK( program->setUniformValue("gui_gray_scale_range_tol",float(BaseMapToOthersProp.ColorBalance*10)) );
    }

    GLCHK( program->setUniformValue("gui_gray_scale_preset",QVector3D(BasicProp.GrayScale.GrayScaleR,
                                                                      BasicProp.GrayScale.GrayScaleG,
                                                                      BasicProp.GrayScale.GrayScaleB)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyInvertComponentsFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_invert_components_filter"]) );

    GLCHK( program->setUniformValue("gui_inverted_components", QVector3D(BasicProp.ColorComponents.InvertRed,
                                                                           BasicProp.ColorComponents.InvertGreen,
                                                                           BasicProp.ColorComponents.InvertBlue)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applySharpenBlurFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *auxFBO,
                                               QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_sharpen_blur"]) );

    GLCHK( program->setUniformValue("gui_sharpen_blur", BasicProp.SharpenBlur) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );

    GLCHK( auxFBO->bind() );
    GLCHK( program->setUniformValue("gauss_mode",1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bind() );
    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( outputFBO->bindDefault() );
    GLCHK( program->setUniformValue("gauss_mode",0) );
}

void OpenGL2DImageWidget::applyNormalsStepFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normals_step_filter"]) );

    GLCHK( program->setUniformValue("gui_normals_step", BasicProp.NormalsStep) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyNormalMixerFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_mixer_filter"]) );

    GLCHK( program->setUniformValue("gui_normal_mixer_depth", NormalMixerProp.Depth*2 ) );
    GLCHK( program->setUniformValue("gui_normal_mixer_angle", NormalMixerProp.Angle/180.0f*3.1415926f) );
    GLCHK( program->setUniformValue("gui_normal_mixer_scale", NormalMixerProp.Scale) );
    GLCHK( program->setUniformValue("gui_normal_mixer_pos_x", NormalMixerProp.PosX) );
    GLCHK( program->setUniformValue("gui_normal_mixer_pos_y", NormalMixerProp.PosY) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, normalMixerInputTexture->textureId()) );

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

void OpenGL2DImageWidget::applyPreSmoothFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *auxFBO,
                                               QOpenGLFramebufferObject *outputFBO,
                                               const QtnPropertySetConvertsionBaseMapLevelProperty& convProp)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );

    GLCHK( program->setUniformValue("gui_gauss_radius", int(convProp.PreSmoothRadius)) );
    GLCHK( program->setUniformValue("gui_gauss_w", float(convProp.PreSmoothRadius)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( program->setUniformValue("gauss_mode",1) );

    GLCHK( auxFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( program->setUniformValue("gauss_mode",2) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( program->setUniformValue("gauss_mode",0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applySobelToNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *outputFBO,
                                                 const QtnPropertySetConvertsionBaseMapLevelProperty& convProp)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_sobel_filter"]) );

    GLCHK( program->setUniformValue("gui_basemap_amp", convProp.Amplitude) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyNormalToHeight(QOpenGLFramebufferObject *normalFBO,
                                            QOpenGLFramebufferObject *heightFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    applyGrayScaleFilter(normalFBO,heightFBO);

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_to_height"]) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,heightFBO->width(),heightFBO->height()) );
    GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,1.0)) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, normalFBO->texture()) );

    // When conversion is enabled (diffuse image only) then
    // this texture (auxFBO4) keeps the orginal diffuse image
    // before the gray scale filter is applied, this is then
    // used to force propper height levels in the bump map
    // since user wants to have defined min/max colors to be 0 or 1
    // in the height map. In case of other conversion this is no more used.
    if(imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.EnableConversion)
    {
        GLCHK( glActiveTexture(GL_TEXTURE2) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO4->texture()) );
    }

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    for(int i = 0; i < imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.Huge ; i++)
    {
        GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,5))) );
        GLCHK( heightFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    }

    for(int i = 0; i < imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.VeryLarge ; i++)
    {
        GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,4))) );
        GLCHK( heightFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    }

    for(int i = 0; i < imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.Large ; i++)
    {
        GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,3))) );
        GLCHK( heightFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    }

    for(int i = 0; i < imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.Medium; i++)
    {
        GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,2))) );
        GLCHK( heightFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    }

    for(int i = 0; i < imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.Small; i++)
    {
        GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,1))) );
        GLCHK( heightFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    }

    for(int i = 0; i < imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.VerySmall; i++)
    {
        GLCHK( program->setUniformValue("hn_min_max_scale",QVector3D(-0.0,1.0,pow(2.0,0))) );
        GLCHK( heightFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, outputFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    }

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyNormalAngleCorrectionFilter(QOpenGLFramebufferObject *inputFBO,
                                                         QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_angle_correction_filter"]) );

    GLCHK( program->setUniformValue("base_map_angle_correction"  , BaseMapToOthersProp.AngleCorrection/180.0f*3.1415926f) );
    GLCHK( program->setUniformValue("base_map_angle_weight"      , BaseMapToOthersProp.AngleWeight) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyNormalExpansionFilter(QOpenGLFramebufferObject *inputFBO,
                                                   QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_expansion_filter"]) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyMixNormalLevels(GLuint level0,
                                             GLuint level1,
                                             GLuint level2,
                                             GLuint level3,
                                             QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_mix_normal_levels_filter"]) );

    GLCHK( program->setUniformValue("gui_base_map_w0"  , imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.WeightSmall ) );
    GLCHK( program->setUniformValue("gui_base_map_w1"  , imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.WeightMedium ) );
    GLCHK( program->setUniformValue("gui_base_map_w2"  , imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.WeightBig) );
    GLCHK( program->setUniformValue("gui_base_map_w3"  , imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.WeightHuge ) );

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( outputFBO->bind() );

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, level0) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, level1) );
    GLCHK( glActiveTexture(GL_TEXTURE2) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, level2) );
    GLCHK( glActiveTexture(GL_TEXTURE3) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, level3) );

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyCPUNormalizationFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLint textureWidth, textureHeight;
    GLCHK( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth) );
    GLCHK( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight) );

    float* img = new float[textureWidth*textureHeight*3];

    GLCHK( glGetTexImage(GL_TEXTURE_2D,0,GL_RGB,GL_FLOAT,img) );

    float min[3] = {img[0],img[1],img[2]};
    float max[3] = {img[0],img[1],img[2]};

    // If materials are enabled calulate the height only in the region of selected material color.
    if(Image::materialsEnabled)
    {
        QImage maskImage = textureFBOs[MATERIAL_TEXTURE]->toImage();
        int currentMaterialIndex = Image::currentMaterialIndex;
        // Number of components.
        int nc = maskImage. byteCount () / (textureWidth*textureHeight) ;
        bool bFirstTimeChecked = true;
        unsigned char * data = maskImage.bits();
        for(int i = 0 ; i < textureWidth*textureHeight ; i++)
        {
            int materialColor = data[nc*i+0]*255*255 + data[nc*i+1]*255 + data[nc*i+2];
            if(materialColor == currentMaterialIndex)
            {
                if(bFirstTimeChecked)
                {
                    for(int c = 0 ; c < 3 ; c++)
                    {
                        min[c] = img[3*i+c];
                        max[c] = max[c];
                    }
                    bFirstTimeChecked = false;
                } // End of first time if.

                for(int c = 0 ; c < 3 ; c++)
                {
                    if( max[c] < img[3*i+c] )
                        max[c] = img[3*i+c];
                    if( min[c] > img[3*i+c] )
                        min[c] = img[3*i+c];
                }
            } // End of material colors are same if.
        } // End of image loop.
    }
    else
    {
        // If materials are disabled calculate
        for(int i = 0 ; i < textureWidth*textureHeight ; i++)
        {
            for(int c = 0 ; c < 3 ; c++)
            {
                if( max[c] < img[3*i+c] )
                    max[c] = img[3*i+c];
                if( min[c] > img[3*i+c] )
                    min[c] = img[3*i+c];
            }
        }
    } // End of materials are enabled if.

    // Prevent singularities.
    for(int k = 0; k < 3 ; k ++)
        if(qAbs(min[k] - max[k]) < 0.0001)
            max[k] += 0.1;

    qDebug() << "Image normalization:";
    qDebug() << "Min color = (" << min[0] << "," << min[1] << "," << min[2] << ")";
    qDebug() << "Max color = (" << max[0] << "," << max[1] << "," << max[2] << ")";

    delete[] img;

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normalize_filter"]) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( program->setUniformValue("min_color",QVector3D(min[0],min[1],min[2])) );
    GLCHK( program->setUniformValue("max_color",QVector3D(max[0],max[1],max[2])) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyAddNoiseFilter(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_add_noise_filter"]) );

    GLCHK( program->setUniformValue("gui_add_noise_amp"  , float(imageWidget[HEIGHT_TEXTURE]->getProperties()->NormalHeightConv.NoiseLevel/100.0) ));

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyBaseMapConversion(QOpenGLFramebufferObject *baseMapFBO,
                                               QOpenGLFramebufferObject *auxFBO,
                                               QOpenGLFramebufferObject *outputFBO,
                                               const QtnPropertySetConvertsionBaseMapLevelProperty& convProp)
{
    applyGrayScaleFilter(baseMapFBO,outputFBO);
    applySobelToNormalFilter(outputFBO,auxFBO,convProp);
    applyInvertComponentsFilter(auxFBO,baseMapFBO);
    applyPreSmoothFilter(baseMapFBO,auxFBO,outputFBO,convProp);

    GLCHK( program->setUniformValue("gui_combine_normals" , 0) );
    GLCHK( program->setUniformValue("gui_filter_radius" , convProp.FilterRadius) );
    GLCHK( program->setUniformValue("gui_normal_flatting" , convProp.Flatness) );

    for(int i = 0; i < convProp.NumIters ; i ++)
    {
        copyFBO(outputFBO,auxFBO);
        applyNormalExpansionFilter(auxFBO,outputFBO);
    }

    GLCHK( program->setUniformValue("gui_combine_normals" , 1) );
    GLCHK( program->setUniformValue("gui_mix_normals"   , convProp.Edges) );
    GLCHK( program->setUniformValue("gui_blend_normals" , convProp.Blending) );

    copyFBO(outputFBO,auxFBO);
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, baseMapFBO->texture()) );

    applyNormalExpansionFilter(auxFBO,outputFBO);
    GLCHK( glActiveTexture(GL_TEXTURE0) );

    GLCHK( program->setUniformValue("gui_combine_normals" , 0 ) );
}

void OpenGL2DImageWidget::applyOcclusionFilter(GLuint height_tex,GLuint normal_tex,
                                             QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_occlusion_filter"]) );

    GLCHK( program->setUniformValue("gui_ssao_no_iters"   ,AOProp.NumIters) );
    GLCHK( program->setUniformValue("gui_ssao_depth"      ,AOProp.Depth) );
    GLCHK( program->setUniformValue("gui_ssao_bias"       ,AOProp.Bias) );
    GLCHK( program->setUniformValue("gui_ssao_intensity"  ,AOProp.Intensity) );

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( outputFBO->bind() );

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, height_tex) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, normal_tex) );

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyHeightProcessingFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_processing_filter"]) );

    GLCHK( program->setUniformValue("gui_height_proc_min_value"   ,ColorLevelsProp.MinValue) );
    GLCHK( program->setUniformValue("gui_height_proc_max_value"   ,ColorLevelsProp.MaxValue) );
    GLCHK( program->setUniformValue("gui_height_proc_ave_radius"  ,int(ColorLevelsProp.DetailsRadius*100.0) ));
    GLCHK( program->setUniformValue("gui_height_proc_offset_value",ColorLevelsProp.Offset) );
    GLCHK( program->setUniformValue("gui_height_proc_normalization",ColorLevelsProp.EnableNormalization) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault());
}

void OpenGL2DImageWidget::applyCombineNormalHeightFilter(QOpenGLFramebufferObject *normalFBO,
                                                       QOpenGLFramebufferObject *heightFBO,
                                                       QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_combine_normal_height_filter"]) );

    GLCHK( glViewport(0,0,normalFBO->width(),normalFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, normalFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, heightFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyRoughnessFilter(QOpenGLFramebufferObject *inputFBO,
                                             QOpenGLFramebufferObject *auxFBO,
                                             QOpenGLFramebufferObject *outputFBO)
{
    applyGaussFilter(inputFBO,auxFBO,outputFBO,int(RMFilterProp.NoiseFilter.Depth));
    copyFBO(outputFBO,auxFBO);

    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_roughness_filter"]) );

    GLCHK( program->setUniformValue("gui_roughness_depth"     ,  RMFilterProp.NoiseFilter.Depth ) );
    GLCHK( program->setUniformValue("gui_roughness_treshold"  ,  RMFilterProp.NoiseFilter.Treshold) );
    GLCHK( program->setUniformValue("gui_roughness_amplifier"  , RMFilterProp.NoiseFilter.Amplifier) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGL2DImageWidget::applyRoughnessColorFilter(QOpenGLFramebufferObject *inputFBO,
                                                  QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_roughness_color_filter"]) );

    QColor color = QColor(RMFilterProp.ColorFilter.PickColor);
    QVector3D dcolor(color.redF(),color.greenF(),color.blueF());

    GLCHK( program->setUniformValue("gui_roughness_picked_color"  , dcolor ) );
    GLCHK( program->setUniformValue("gui_roughness_color_method"  , (int)RMFilterProp.ColorFilter.Method) );
    GLCHK( program->setUniformValue("gui_roughness_color_offset"  , RMFilterProp.ColorFilter.Bias ) );
    GLCHK( program->setUniformValue("gui_roughness_color_global_offset"  , RMFilterProp.ColorFilter.Offset) );
    GLCHK( program->setUniformValue("gui_roughness_invert_mask"   , RMFilterProp.ColorFilter.InvertColors ) );
    GLCHK( program->setUniformValue("gui_roughness_color_amplifier", RMFilterProp.ColorFilter.Amplifier ) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyGrungeImageFilter (QOpenGLFramebufferObject *inputFBO,
                                                QOpenGLFramebufferObject *outputFBO,
                                                QOpenGLFramebufferObject *grungeFBO)
{
    // Grunge is treated differently in normal texture.
    if(activeTextureType == NORMAL_TEXTURE)
    {
        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_to_normal"]) );

        GLCHK( program->setUniformValue("gui_hn_conversion_depth", 2*GrungeOnImageProp.GrungeWeight * GrungeProp.OverallWeight) );
        GLCHK( glViewport(0,0,auxFBO3->width(),auxFBO3->height()) );
        GLCHK( auxFBO3->bind() );
        GLCHK( glBindTexture(GL_TEXTURE_2D, grungeFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        GLCHK( auxFBO3->bindDefault() );

        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_mixer_filter"]) );

        float weight = 2 * GrungeOnImageProp.ImageWeight;

        GLCHK( program->setUniformValue("gui_normal_mixer_depth", weight) );
        GLCHK( program->setUniformValue("gui_normal_mixer_angle", 0.0f) );
        GLCHK( program->setUniformValue("gui_normal_mixer_scale", 1.0f) );
        GLCHK( program->setUniformValue("gui_normal_mixer_pos_x", 0.0f) );
        GLCHK( program->setUniformValue("gui_normal_mixer_pos_y", 0.0f) );

        GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE1) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );

        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO3->texture()) );

        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        GLCHK( outputFBO->bindDefault() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
    }
    else
    {
        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_grunge_filter"]) );

        float weight = GrungeOnImageProp.GrungeWeight * GrungeProp.OverallWeight;

        GLCHK( program->setUniformValue("gui_grunge_overall_weight"  , weight ) );
        GLCHK( program->setUniformValue("gui_grunge_blending_mode"  , (int)GrungeOnImageProp.BlendingMode) );

        GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
        GLCHK( outputFBO->bind() );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
        GLCHK( glActiveTexture(GL_TEXTURE1) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, grungeFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        outputFBO->bindDefault();
    }
}

void OpenGL2DImageWidget::applyGrungeRandomizationFilter(QOpenGLFramebufferObject *inputFBO,
                                                       QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_grunge_randomization_filter"]) );

    QMatrix3x3 random_angles;
    qsrand(GrungeProp.Randomize);
    for(int i = 0; i < 9; i++)
        random_angles.data()[i] = 3.1415*qrand() / (RAND_MAX+0.0);
    GLCHK( program->setUniformValue("gui_seamless_random_angles" , random_angles) );
    float phase = 3.1415*qrand()/(RAND_MAX+0.0);
    GLCHK( program->setUniformValue("gui_seamless_random_phase" , phase) );
    GLCHK( program->setUniformValue("gui_grunge_radius"    , GrungeProp.Scale) );
    GLCHK( program->setUniformValue("gui_grunge_brandomize" , bool(GrungeProp.Randomize!=0)) );

    GLCHK( program->setUniformValue("gui_grunge_translations" , int(GrungeProp.RandomTranslations) ) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, textureFBOs[GRUNGE_TEXTURE]->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyGrungeWarpNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_grunge_normal_warp_filter"]) );

    GLCHK( program->setUniformValue("grunge_normal_warp"  , GrungeProp.NormalWarp) );

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, textures[NORMAL_TEXTURE]->textureId()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGL2DImageWidget::applyAllUVsTransforms(QOpenGLFramebufferObject *inoutFBO)
{
    if(Image::bSeamlessTranslationsFirst)
    {
        // The output is saved to the activeFBO.
        applyPerspectiveTransformFilter(inoutFBO, auxFBO1);
    }
    // Make seamless.
    switch(Image::seamlessMode)
    {
    case(SEAMLESS_SIMPLE):
        // The output is saved to the activeFBO.
        applySeamlessLinearFilter(inoutFBO, auxFBO1);
        break;
    case(SEAMLESS_MIRROR):
    case(SEAMLESS_RANDOM):
        applySeamlessFilter(inoutFBO, auxFBO1);
        copyFBO(auxFBO1, inoutFBO);
        break;
    case(SEAMLESS_NONE):
    default:
        break;
    }
    if(!Image::bSeamlessTranslationsFirst)
    {
        applyPerspectiveTransformFilter(inoutFBO,auxFBO1);// the output is save to activeFBO
    }
}

void OpenGL2DImageWidget::updateProgramUniforms(int step)
{
    switch(step)
    {
    case(0):
        GLCHK( program->setUniformValue("gui_image_type", openGL330ForceTexType) );
        GLCHK( program->setUniformValue("gui_depth", float(1.0)) );
        GLCHK( program->setUniformValue("gui_mode_dgaussian", 1) );
        GLCHK( program->setUniformValue("material_id", int(Image::currentMaterialIndex) ) );

        if(activeTextureType == MATERIAL_TEXTURE)
        {
            GLCHK( program->setUniformValue("material_id", int(-1) ) );
        }
        break;
    default:
        break;
    };
}

void OpenGL2DImageWidget::updateMousePosition()
{
    QPoint p = mapFromGlobal(QCursor::pos());
    cursorPhysicalXPosition = double(p.x())/width() * orthographicProjWidth - xTranslation;
    cursorPhysicalYPosition = (1.0-double(p.y())/height())* orthographicProjHeight - yTranslation;
    bSkipProcessing = true;
}

void OpenGL2DImageWidget::render()
{
    if (!imageWidget[activeTextureType]) return;

    QOpenGLFramebufferObject* activeFBO = textureFBOs[activeTextureType];

    // Since grunge map can be different we need to calculate ratio each time.
    if (activeFBO)
    {
        fboRatio = float(activeFBO->width())/activeFBO->height();
        orthographicProjHeight = (1+zoom)/windowRatio;
        orthographicProjWidth  = (1+zoom)/fboRatio;
    }

    // Do not clear the background during rendering process.
    if(!bShadowRender)
    {
        GLCHK( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
    }

    GLCHK( glDisable(GL_CULL_FACE) );
    GLCHK( glDisable(GL_DEPTH_TEST) );

    // Images which depend on others will not be affected by UV changes again.
    bool bTransformUVs = true;
    bool bSkipStandardProcessing = false;

    // Do not process images when disabled.
    if (skipProcessing[activeTextureType] && activeTextureType != MATERIAL_TEXTURE)
        bSkipProcessing = true;
    if(bToggleColorPicking)
        bSkipStandardProcessing = true;

    // Resizing the FBOs in case of convertion procedure.
    if(!bSkipProcessing == true)
    {
        switch(conversionType)
        {
        case(CONVERT_FROM_H_TO_N):
            break;
        case(CONVERT_FROM_N_TO_H):
            break;
        case(CONVERT_FROM_D_TO_O):
            break;
        case(CONVERT_RESIZE):
            // Apply resize textures.
            delete textureFBOs[activeTextureType];
            textureFBOs[activeTextureType] = createTextureFBO(resize_width, resize_height);
            // Pointers were changed in resize function.
            activeFBO = textureFBOs[activeTextureType];
            bSkipStandardProcessing = true;
            break;
        default:
            break;
        }

        // Create or resize when image was changed.
        if (auxFBO1) delete auxFBO1;
        auxFBO1 = createFBO(activeFBO->width(), activeFBO->height());
        if (auxFBO2) delete auxFBO2;
        auxFBO2 = createFBO(activeFBO->width(), activeFBO->height());
        if (auxFBO3) delete auxFBO3;
        auxFBO3 = createFBO(activeFBO->width(), activeFBO->height());
        if (auxFBO4) delete auxFBO4;
        auxFBO4 = createFBO(activeFBO->width(), activeFBO->height());

        // Allocate aditional FBOs when conversion from BaseMap is enabled.
        if(activeTextureType == DIFFUSE_TEXTURE && imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.EnableConversion)
        {
            for(int i = 0; i < 3 ; i++)
            {
                int width = activeFBO->width() / pow (2, i + 1);
                int height = activeFBO->height() / pow (2, i + 1);
                if (auxFBO0BMLevels[i]) delete auxFBO0BMLevels[i];
                auxFBO0BMLevels[i] = createFBO(width, height);
                if (auxFBO1BMLevels[i]) delete auxFBO1BMLevels[i];
                auxFBO1BMLevels[i] = createFBO(width, height);
                if (auxFBO2BMLevels[i]) delete auxFBO2BMLevels[i];
                auxFBO2BMLevels[i] = createFBO(width, height);
            }
        }

        GLCHK( program->bind() );
        GLCHK( program->setUniformValue("gui_image_type", activeTextureType) );
        openGL330ForceTexType = activeTextureType;
        GLCHK( program->setUniformValue("gui_depth", float(1.0)) );
        GLCHK( program->setUniformValue("gui_mode_dgaussian", 1) );
        GLCHK( program->setUniformValue("material_id", int(Image::currentMaterialIndex) ) );

        resetView();

        // Skip all precessing when material tab is selected.
        if(activeTextureType == MATERIAL_TEXTURE)
        {
            bSkipStandardProcessing = true;
            GLCHK( program->setUniformValue("material_id", int(-1) ) );
        }
        if(activeTextureType == GRUNGE_TEXTURE)
        {
            bTransformUVs = false;
            GLCHK( program->setUniformValue("material_id", int(-10) ) );
        }

        GLCHK( glActiveTexture(GL_TEXTURE10) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, textures[MATERIAL_TEXTURE]->textureId()) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );

        copyTex2FBO(textures[activeTextureType]->textureId(), textureFBOs[activeTextureType]);

        // In some cases the output image will be taken from other sources.
        switch(activeTextureType)
        {
        case(NORMAL_TEXTURE):
        {
            // Choose proper action.
            switch(imageWidget[NORMAL_TEXTURE]->getInputImageType())
            {
            case(INPUT_FROM_NORMAL_INPUT):
                if(conversionType == CONVERT_FROM_H_TO_N)
                {
                    applyHeightToNormal(textureFBOs[HEIGHT_TEXTURE],activeFBO);
                    bTransformUVs = false;
                }
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                // Transform height before normal calculation.
                if(conversionType == CONVERT_NONE)
                {
                    // Perspective transformation treats normal texture differently.
                    // Temporarily change the texture type to perform transformations.
                    // Used for GL3.30 version
                    openGL330ForceTexType = HEIGHT_TEXTURE;
                    GLCHK( program->setUniformValue("gui_image_type", HEIGHT_TEXTURE) );

                    copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(),activeFBO);
                    applyAllUVsTransforms(activeFBO);
                    copyFBO(activeFBO,auxFBO1);
                    applyHeightToNormal(auxFBO1,activeFBO);
                    GLCHK( program->setUniformValue("gui_image_type", NORMAL_TEXTURE) );
                    openGL330ForceTexType = NORMAL_TEXTURE;
                    bTransformUVs = false;
                }
                else
                {
                    copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(),activeFBO);
                }
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                applyHeightToNormal(textureFBOs[HEIGHT_TEXTURE],activeFBO);

                if(!skipProcessing[HEIGHT_TEXTURE])
                    bTransformUVs = false;
                break;
            default:
                break;
            }
            break;
        } // End of NORMAL_TEXTURE case.

        case(SPECULAR_TEXTURE):
        {
            // Choose proper action.
            switch(imageWidget[SPECULAR_TEXTURE]->getInputImageType())
            {
            case(INPUT_FROM_SPECULAR_INPUT):
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(),activeFBO);
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                copyFBO(textureFBOs[HEIGHT_TEXTURE],activeFBO);
                if(!skipProcessing[HEIGHT_TEXTURE])
                    bTransformUVs = false;
                break;
            case(INPUT_FROM_DIFFUSE_INPUT):
                copyTex2FBO(textures[DIFFUSE_TEXTURE]->textureId(), activeFBO);
                break;
            case(INPUT_FROM_DIFFUSE_OUTPUT):
                copyFBO(textureFBOs[DIFFUSE_TEXTURE], activeFBO);
                if(!skipProcessing[DIFFUSE_TEXTURE])
                    bTransformUVs = false;
                break;
            default:
                break;
            }
            break;
        } // End of SPECULAR_TEXTURE case.

        case(OCCLUSION_TEXTURE):
        {
            // Choose proper action.
            switch(imageWidget[OCCLUSION_TEXTURE]->getInputImageType())
            {
            case(INPUT_FROM_OCCLUSION_INPUT):
                if(conversionType == CONVERT_FROM_HN_TO_OC)
                {
                    // Ambient occlusion is calculated from normal and height map.
                    // Skip some parts of the processing.
                    applyOcclusionFilter(textureFBOs[HEIGHT_TEXTURE]->texture(),textureFBOs[NORMAL_TEXTURE]->texture(),activeFBO);
                    bSkipStandardProcessing =  true;
                    if(!skipProcessing[HEIGHT_TEXTURE] && !skipProcessing[NORMAL_TEXTURE])
                        bTransformUVs = false;
                    qDebug() << "Calculation AO from Normal and Height";
                }
                break;
            case(INPUT_FROM_HI_NI):
                // Ambient occlusion is calculated from normal and height map.
                // Skip some parts of the processing.
                applyOcclusionFilter(textures[HEIGHT_TEXTURE]->textureId(), textures[NORMAL_TEXTURE]->textureId(), activeFBO);
                break;
            case(INPUT_FROM_HO_NO):
                applyOcclusionFilter(textureFBOs[HEIGHT_TEXTURE]->texture(),textureFBOs[NORMAL_TEXTURE]->texture(),activeFBO);
                if(!skipProcessing[HEIGHT_TEXTURE] && !skipProcessing[NORMAL_TEXTURE])
                    bTransformUVs = false;
                break;
            default:
                break;
            }
            break;
        } // End of OCCULSION_TEXTURE case.

        case(HEIGHT_TEXTURE):
        {
            if(conversionType == CONVERT_FROM_N_TO_H)
            {
                applyNormalToHeight(textureFBOs[NORMAL_TEXTURE],activeFBO,auxFBO1);
                applyCPUNormalizationFilter(auxFBO1,activeFBO);
                applyAddNoiseFilter(activeFBO,auxFBO1);
                copyFBO(auxFBO1,activeFBO);

                if(!skipProcessing[NORMAL_TEXTURE])
                    bTransformUVs = false;
            }
            break;
        } // End of HEIGHT_TEXTURE case.

        case(ROUGHNESS_TEXTURE):
        {
            // Choose proper action.
            switch(imageWidget[ROUGHNESS_TEXTURE]->getInputImageType())
            {
            case(INPUT_FROM_ROUGHNESS_INPUT):
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(), activeFBO);
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                copyFBO(textureFBOs[HEIGHT_TEXTURE],activeFBO);
                if(!skipProcessing[HEIGHT_TEXTURE])
                    bTransformUVs = false;
                break;
            case(INPUT_FROM_DIFFUSE_INPUT):
                copyTex2FBO(textures[DIFFUSE_TEXTURE]->textureId(), activeFBO);
                break;
            case(INPUT_FROM_DIFFUSE_OUTPUT):
                copyFBO(textureFBOs[DIFFUSE_TEXTURE], activeFBO);
                if(!skipProcessing[DIFFUSE_TEXTURE])
                    bTransformUVs = false;
                break;
            default:
                break;
            }
            break;
        } // End of ROUGHNESS_TEXTURE case.

        case(METALLIC_TEXTURE):
        {
            // Choose proper action.
            switch(imageWidget[METALLIC_TEXTURE]->getInputImageType())
            {
            case(INPUT_FROM_METALLIC_INPUT):
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                copyTex2FBO(textures[HEIGHT_TEXTURE]->textureId(), activeFBO);
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                copyFBO(textureFBOs[HEIGHT_TEXTURE],activeFBO);
                if(!skipProcessing[HEIGHT_TEXTURE])
                    bTransformUVs = false;
                break;
            case(INPUT_FROM_DIFFUSE_INPUT):
                copyTex2FBO(textures[DIFFUSE_TEXTURE]->textureId(), activeFBO);
                break;
            case(INPUT_FROM_DIFFUSE_OUTPUT):
                copyFBO(textureFBOs[DIFFUSE_TEXTURE],activeFBO);
                if(!skipProcessing[DIFFUSE_TEXTURE])
                    bTransformUVs = false;
                break;
            default:
                break;
            }
            break;
        } // End of METALLIC_TEXTURE case.

        case(GRUNGE_TEXTURE):
        {
            applyGrungeWarpNormalFilter(activeFBO,auxFBO2);

            //if(activeImage->grungeSeed != 0)
            applyGrungeRandomizationFilter(auxFBO2,activeFBO);
            //else
            //    copyTex2FBO(auxFBO2->texture(),activeFBO);
            break;
        } // End of GRUNGE_TEXTURE case.

        default:
            break;
        };

        // Apply grunge filter when enabled.
        if(conversionType == CONVERT_NONE && GrungeProp.OverallWeight != 0.0f )
        {
            if(activeTextureType < MATERIAL_TEXTURE)
            {
                copyTex2FBO(textureFBOs[GRUNGE_TEXTURE]->texture(),auxFBO2);
                // If "output" type selected, transform grunge map.
                if(bTransformUVs == false)
                {
                    // auxFBO1 is used inside
                    applyAllUVsTransforms(auxFBO2);
                }
                // auxFBO3 is used inside
                applyGrungeImageFilter(activeFBO,auxFBO1,auxFBO2);
                copyTex2FBO(auxFBO1->texture(),activeFBO);
            }
        }

        // Transform UVs in some cases.
        if(conversionType == CONVERT_NONE && bTransformUVs)
        {
            applyAllUVsTransforms(activeFBO);
        }

        // Skip all processing and when mouse is dragged.
        if(!bSkipStandardProcessing)
        {
            // Begin standard pipe-line (for each image).
            applyInvertComponentsFilter(activeFBO,auxFBO1);
            copyFBO(auxFBO1,activeFBO);

            if(activeTextureType != HEIGHT_TEXTURE &&
                    activeTextureType != NORMAL_TEXTURE &&
                    activeTextureType != OCCLUSION_TEXTURE &&
                    activeTextureType != ROUGHNESS_TEXTURE)
            {
                // Hue manipulation.
                applyColorHueFilter(activeFBO,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(BasicProp.GrayScale.EnableGrayScale ||
                    activeTextureType == ROUGHNESS_TEXTURE ||
                    activeTextureType == OCCLUSION_TEXTURE ||
                    activeTextureType == HEIGHT_TEXTURE )
            {
                applyGrayScaleFilter(auxFBO1,activeFBO);
            }
            else
            {
                copyFBO(auxFBO1,activeFBO);
            }

            // Specular manipulation.
            if(SurfaceDetailsProp.EnableSurfaceDetails &&
                    activeTextureType != HEIGHT_TEXTURE)
            {
                applyDGaussiansFilter(activeFBO,auxFBO2,auxFBO1);
                //copyFBO(auxFBO1,activeFBO);
                applyContrastFilter(auxFBO1,activeFBO);
            }

            // Removing shading.
            if(imageWidget[activeTextureType]->getProperties()->EnableRemoveShading)
            {
                applyRemoveLowFreqFilter(activeFBO,auxFBO2);
                copyFBO(auxFBO2,activeFBO);

                applyGaussFilter(activeFBO,auxFBO2,auxFBO1,1);
                applyInverseColorFilter(auxFBO1,auxFBO2);
                copyFBO(auxFBO2,auxFBO1);
                applyOverlayFilter(activeFBO,auxFBO1,auxFBO2);

                applyRemoveShadingFilter(auxFBO2, textureFBOs[OCCLUSION_TEXTURE], activeFBO, auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(BasicProp.EnhanceDetails > 0)
            {
                for(int i = 0 ; i < BasicProp.EnhanceDetails ; i++ )
                {
                    applyGaussFilter(activeFBO,auxFBO2,auxFBO1,1);
                    applyOverlayFilter(activeFBO,auxFBO1,auxFBO2);
                    copyFBO(auxFBO2,activeFBO);
                }
            }

            if(BasicProp.SmallDetails  > 0.0f)
            {
                applySmallDetailsFilter(activeFBO,auxFBO2,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(BasicProp.MediumDetails > 0.0f)
            {
                applyMediumDetailsFilter(activeFBO,auxFBO2,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(BasicProp.SharpenBlur != 0)
            {
                applySharpenBlurFilter(activeFBO,auxFBO2,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(activeTextureType != NORMAL_TEXTURE)
            {
                applyHeightProcessingFilter(activeFBO,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            // Roughness color mapping.
            // Use the same filters for both metallic and roughness, because they are almost the same.
            if( (activeTextureType == ROUGHNESS_TEXTURE ||
                 activeTextureType == METALLIC_TEXTURE )
                    && RMFilterProp.Filter == COLOR_FILTER::Noise )
            {
                // Processing surface.
                applyRoughnessFilter(activeFBO,auxFBO2,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(activeTextureType == ROUGHNESS_TEXTURE ||
                    activeTextureType == METALLIC_TEXTURE)
            {
                if(RMFilterProp.Filter == COLOR_FILTER::Color)
                {
                    applyRoughnessColorFilter(activeFBO,auxFBO1);
                    copyFBO(auxFBO1,activeFBO);
                }
            }

            // Height processing pipeline.

            // Normal processing pipeline.
            if(activeTextureType == NORMAL_TEXTURE)
            {
                applyNormalsStepFilter(activeFBO,auxFBO1);
                // Apply normal mixer filter.
                if(NormalMixerProp.EnableMixer && normalMixerInputTexture != 0)
                {
                    applyNormalMixerFilter(auxFBO1,activeFBO);
                }
                else
                {
                    // Skip.
                    copyFBO(auxFBO1,activeFBO);
                }
            }

            // Diffuse processing pipeline.
            if(!bToggleColorPicking)
            {
                // Skip this step if the Color picking is enabled
                if(activeTextureType == DIFFUSE_TEXTURE &&
                        (imageWidget[activeTextureType]->getProperties()->BaseMapToOthers.EnableConversion ||
                         conversionType == CONVERT_FROM_D_TO_O ))
                {
                    // Create mipmaps.
                    copyTex2FBO(activeFBO->texture(),auxFBO0BMLevels[0]);
                    copyTex2FBO(activeFBO->texture(),auxFBO0BMLevels[1]);
                    copyTex2FBO(activeFBO->texture(),auxFBO0BMLevels[2]);
                    // Calculate normal for orginal image.
                    applyBaseMapConversion(activeFBO,auxFBO2,auxFBO1, BaseMapToOthersProp.LevelSmall);
                    applyBaseMapConversion(auxFBO0BMLevels[0],auxFBO1BMLevels[0],auxFBO2BMLevels[0],BaseMapToOthersProp.LevelMedium);
                    applyBaseMapConversion(auxFBO0BMLevels[1],auxFBO1BMLevels[1],auxFBO2BMLevels[1],BaseMapToOthersProp.LevelBig);
                    applyBaseMapConversion(auxFBO0BMLevels[2],auxFBO1BMLevels[2],auxFBO2BMLevels[2],BaseMapToOthersProp.LevelHuge);

                    // Mix normals together.
                    applyMixNormalLevels(auxFBO1->texture(),
                            auxFBO2BMLevels[0]->texture(),
                            auxFBO2BMLevels[1]->texture(),
                            auxFBO2BMLevels[2]->texture(),
                            activeFBO);

                    // Apply angle correction.
                    applyNormalAngleCorrectionFilter(activeFBO,auxFBO1);
                    copyTex2FBO(auxFBO1->texture(),activeFBO);

                    if(conversionType == CONVERT_FROM_D_TO_O)
                    {
                        applyNormalToHeight(activeFBO,auxFBO1,auxFBO2);
                        applyCPUNormalizationFilter(auxFBO2,auxFBO1);
                        applyAddNoiseFilter(auxFBO1,auxFBO2);
                        copyFBO(auxFBO2,auxFBO1);

                    }
                    else if(Image::bConversionBaseMapShowHeightTexture)
                    {
                        applyNormalToHeight(activeFBO,auxFBO1,auxFBO2);
                        applyCPUNormalizationFilter(auxFBO2,activeFBO);
                    }
                } // End of base map conversion.
            }
        } // End of skip standard processing.


        // Copy the conversion results to proper textures.
        switch(conversionType)
        {
        case(CONVERT_FROM_H_TO_N):
            if(activeTextureType == NORMAL_TEXTURE)
            {
                copyFBO(activeFBO,textureFBOs[NORMAL_TEXTURE]);
            }
            break;
        case(CONVERT_FROM_N_TO_H):
            if(activeTextureType == HEIGHT_TEXTURE)
            {
                qDebug() << "Changing reference image of height";
            }
            break;
        case(CONVERT_FROM_D_TO_O):
            copyFBO(activeFBO, textureFBOs[NORMAL_TEXTURE]);
            copyFBO(auxFBO1,textureFBOs[HEIGHT_TEXTURE]);
            applyOcclusionFilter(textures[HEIGHT_TEXTURE]->textureId(), textures[NORMAL_TEXTURE]->textureId(), textureFBOs[OCCLUSION_TEXTURE]);
            copyTex2FBO(textures[activeTextureType]->textureId(), textureFBOs[SPECULAR_TEXTURE]);
            copyTex2FBO(textures[activeTextureType]->textureId(), textureFBOs[ROUGHNESS_TEXTURE]);
            copyTex2FBO(textures[activeTextureType]->textureId(), textureFBOs[METALLIC_TEXTURE]);
            break;
        case(CONVERT_FROM_HN_TO_OC):
            copyFBO(activeFBO, textureFBOs[OCCLUSION_TEXTURE]);
            break;
        default:
            break;
        }

        activeFBO = textureFBOs[activeTextureType];

    } // End of skip processing

    bSkipProcessing = false;
    conversionType  = CONVERT_NONE;

    if(!bShadowRender)
    {
        if (renderFBO) delete renderFBO;
        renderFBO = createFBO(activeFBO->width(), activeFBO->height());
        GLCHK( program->setUniformValue("material_id", int(-1)) );
        GLCHK(applyNormalFilter(activeFBO, renderFBO));
    }

    if (paintFBO) delete paintFBO;
    paintFBO = createFBO(renderFBO->width(), renderFBO->height());
    GLCHK( program->setUniformValue("material_id", int(-1)) );
    copyFBO(renderFBO, paintFBO);
    bRendering = false;
}

QOpenGLFramebufferObject* OpenGL2DImageWidget::createFBO(int width, int height)
{
    QOpenGLFramebufferObjectFormat format;
    format.setInternalTextureFormat(GL_RGB16F);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setMipmap(true);

    QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(width, height, format);

    GLCHK( glBindTexture(GL_TEXTURE_2D, fbo->texture()) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) );

    if(Image::bUseLinearInterpolation)
    {
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    }
    else
    {
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST) );
        GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST) );
    }

    float aniso = 0.0f;
    GLCHK( glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, 0) );

    return fbo;
}

QOpenGLFramebufferObject* OpenGL2DImageWidget::createTextureFBO(int width, int height)
{
    QOpenGLFramebufferObjectFormat format;
    format.setInternalTextureFormat(GL_RGB16F);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setMipmap(true);
    QOpenGLFramebufferObject* fbo = new QOpenGLFramebufferObject(width, height, format);

    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    if(Image::bUseLinearInterpolation)
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    else
    {
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    }

    float aniso = 0.0f;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    glBindTexture(GL_TEXTURE_2D, 0);

    return fbo;
}

void OpenGL2DImageWidget::copyFBO(QOpenGLFramebufferObject *src,
                                QOpenGLFramebufferObject *dst)
{
    copyTex2FBO(src->texture(), dst);
}

void OpenGL2DImageWidget::copyTex2FBO(GLuint src_tex_id,
                                    QOpenGLFramebufferObject *dst)
{
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );

    GLCHK( dst->bind() );
    GLCHK( glViewport(0,0,dst->width(),dst->height()) );

    GLCHK( glBindTexture(GL_TEXTURE_2D, src_tex_id) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    dst->bindDefault();
}
