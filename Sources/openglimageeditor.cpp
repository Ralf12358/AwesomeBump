#include "openglimageeditor.h"

#include "openglframebufferobject.h"
#include "openglerrorcheck.h"

OpenGLImageEditor::OpenGLImageEditor(QWidget *parent) :
    OpenGLWidgetBase(parent)
{
    bShadowRender         = false;
    bSkipProcessing       = false;
    bRendering            = false;
    bToggleColorPicking   = false;
    conversionType        = CONVERT_NONE;
    uvManilupationMethod  = UV_TRANSLATE;
    cornerWeights         = QVector4D(0,0,0,0);
    fboRatio = 1;
    renderFBO		  = NULL;
    paintFBO		  = NULL;

    // initialize position of the corners
    cornerPositions[0] = QVector2D(-0.0,-0);
    cornerPositions[1] = QVector2D( 1,-0);
    cornerPositions[2] = QVector2D( 1, 1);
    cornerPositions[3] = QVector2D(-0, 1);
    for(int i = 0 ; i < 4 ; i++)
    {
        grungeCornerPositions[i] = cornerPositions[i];
    }
    draggingCorner       = -1;
    gui_perspective_mode =  0;
    gui_seamless_mode    =  0;
    setCursor(Qt::OpenHandCursor);
    cornerCursors[0] = QCursor(QPixmap(":/resources/cursors/corner1.png"));
    cornerCursors[1] = QCursor(QPixmap(":/resources/cursors/corner2.png"));
    cornerCursors[2] = QCursor(QPixmap(":/resources/cursors/corner3.png"));
    cornerCursors[3] = QCursor(QPixmap(":/resources/cursors/corner4.png"));
    activeImage = NULL;
    connect(this,SIGNAL(rendered()),this,SLOT(copyRenderToPaintFBO()));
}

OpenGLImageEditor::~OpenGLImageEditor()
{
    cleanup();
}

void OpenGLImageEditor::cleanup()
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

    glDeleteBuffers(sizeof(vbos)/sizeof(GLuint), &vbos[0]);
    doneCurrent();
}

QSize OpenGLImageEditor::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize OpenGLImageEditor::sizeHint() const
{
    return QSize(500, 400);
}

void OpenGLImageEditor::initializeGL()
{
    qDebug() << "calling " << Q_FUNC_INFO;

    initializeOpenGLFunctions();

    QColor clearColor = QColor::fromCmykF(0.79, 0.79, 0.79, 0.0).dark();
    GLCHK( glClearColor((GLfloat)clearColor.red() / 255.0, (GLfloat)clearColor.green() / 255.0,
                        (GLfloat)clearColor.blue() / 255.0, (GLfloat)clearColor.alpha() / 255.0) );
    GLCHK( glEnable(GL_MULTISAMPLE) );
    GLCHK( glEnable(GL_DEPTH_TEST) );

    QVector<QString> filters_list;
    filters_list.push_back("mode_normal_filter");
    filters_list.push_back("mode_height_to_normal");
    filters_list.push_back("mode_perspective_transform_filter");
    filters_list.push_back("mode_seamless_linear_filter");
    filters_list.push_back("mode_seamless_filter");
    filters_list.push_back("mode_occlusion_filter");
    filters_list.push_back("mode_normal_to_height");
    filters_list.push_back("mode_normalize_filter");
    filters_list.push_back("mode_gauss_filter");
    filters_list.push_back("mode_gray_scale_filter");
    filters_list.push_back("mode_invert_components_filter");
    filters_list.push_back("mode_color_hue_filter");
    filters_list.push_back("mode_roughness_filter");
    filters_list.push_back("mode_dgaussians_filter");
    filters_list.push_back("mode_constrast_filter");
    filters_list.push_back("mode_height_processing_filter");
    filters_list.push_back("mode_remove_low_freq_filter");
    filters_list.push_back("mode_invert_filter");
    filters_list.push_back("mode_overlay_filter");
    filters_list.push_back("mode_ao_cancellation_filter");
    filters_list.push_back("mode_small_details_filter");
    filters_list.push_back("mode_medium_details_filter");
    filters_list.push_back("mode_sharpen_blur");
    filters_list.push_back("mode_normals_step_filter");
    filters_list.push_back("mode_normal_mixer_filter");
    filters_list.push_back("mode_sobel_filter");
    filters_list.push_back("mode_normal_expansion_filter");
    filters_list.push_back("mode_mix_normal_levels_filter");
    filters_list.push_back("mode_combine_normal_height_filter");
    filters_list.push_back("mode_roughness_color_filter");
    filters_list.push_back("mode_grunge_filter");
    filters_list.push_back("mode_grunge_randomization_filter");
    filters_list.push_back("mode_grunge_normal_warp_filter");
    filters_list.push_back("mode_normal_angle_correction_filter");
    filters_list.push_back("mode_add_noise_filter");

    qDebug() << "Loading filters (fragment shader)";
    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile(":/resources/shaders/filters.vert");
    if (!vshader->log().isEmpty())
        qDebug() << vshader->log();
    else
        qDebug() << "done";

    QFile fFile(":/resources/shaders/filters.frag");
    fFile.open(QFile::ReadOnly);
    QTextStream inf(&fFile);
    QString shaderCode = inf.readAll();

#ifdef USE_OPENGL_330
    for(int filter = 0 ; filter < filters_list.size() ; filter++ )
    {
        qDebug() << "Compiling filter:" << filters_list[filter];
        QString preambule = "#version 330 core\n"
                            "#define USE_OPENGL_330\n"
                            "#define "+filters_list[filter]+"_330\n" ;

        QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
        fshader->compileSourceCode(preambule + shaderCode);
        if (!fshader->log().isEmpty()) qDebug() << fshader->log();

        program = new QOpenGLShaderProgram(this);
        program->addShader(vshader);
        program->addShader(fshader);
        program->bindAttributeLocation("positionIn", 0);
        GLCHK( program->link() );
        GLCHK( program->bind() );
        GLCHK( program->setUniformValue("layerA" , 0) );
        GLCHK( program->setUniformValue("layerB" , 1) );
        GLCHK( program->setUniformValue("layerC" , 2) );
        GLCHK( program->setUniformValue("layerD" , 3) );
        GLCHK( program->setUniformValue("materialTexture" ,10) );

        filter_programs[filters_list[filter].toStdString()] = program;
        GLCHK( program->release());
        delete fshader;
    }
    delete vshader;
#else
    qDebug() << "Loading filters (vertex shader)";
    QString preambule = "#version 400 core\n";

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(preambule + shaderCode);
    if (!fshader->log().isEmpty())
        qDebug() << fshader->log();
    else
        qDebug() << "done";

    program = new QOpenGLShaderProgram(this);
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("positionIn", 0);
    GLCHK( program->link() );
    GLCHK( program->bind() );
    GLCHK( program->setUniformValue("layerA" , 0) );
    GLCHK( program->setUniformValue("layerB" , 1) );
    GLCHK( program->setUniformValue("layerC" , 2) );
    GLCHK( program->setUniformValue("layerD" , 3) );
    GLCHK( program->setUniformValue("materialTexture" ,10) );

    delete vshader;
    delete fshader;

    GLCHK( subroutines["mode_normal_filter"]                  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_filter") );
    GLCHK( subroutines["mode_color_hue_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_color_hue_filter") );
    GLCHK( subroutines["mode_overlay_filter"]                 = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_overlay_filter") );
    GLCHK( subroutines["mode_ao_cancellation_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_ao_cancellation_filter") );
    GLCHK( subroutines["mode_invert_filter"]                  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_invert_filter") );
    GLCHK( subroutines["mode_gauss_filter"]                   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_gauss_filter") );
    GLCHK( subroutines["mode_seamless_filter"]                = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_seamless_filter") );
    GLCHK( subroutines["mode_seamless_linear_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_seamless_linear_filter") );
    GLCHK( subroutines["mode_dgaussians_filter"]              = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_dgaussians_filter") );
    GLCHK( subroutines["mode_constrast_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_constrast_filter") );
    GLCHK( subroutines["mode_small_details_filter"]           = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_small_details_filter") );
    GLCHK( subroutines["mode_gray_scale_filter"]              = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_gray_scale_filter") );
    GLCHK( subroutines["mode_medium_details_filter"]          = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_medium_details_filter") );
    GLCHK( subroutines["mode_height_to_normal"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_height_to_normal") );
    GLCHK( subroutines["mode_sharpen_blur"]                   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_sharpen_blur") );
    GLCHK( subroutines["mode_normals_step_filter"]            = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normals_step_filter") );
    GLCHK( subroutines["mode_normal_mixer_filter"]            = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_mixer_filter") );
    GLCHK( subroutines["mode_invert_components_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_invert_components_filter") );
    GLCHK( subroutines["mode_normal_to_height"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_to_height") );
    GLCHK( subroutines["mode_sobel_filter"]                   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_sobel_filter") );
    GLCHK( subroutines["mode_normal_expansion_filter"]        = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_expansion_filter") );
    GLCHK( subroutines["mode_mix_normal_levels_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_mix_normal_levels_filter") );
    GLCHK( subroutines["mode_normalize_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normalize_filter") );
    GLCHK( subroutines["mode_occlusion_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_occlusion_filter") );
    GLCHK( subroutines["mode_combine_normal_height_filter"]   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_combine_normal_height_filter") );
    GLCHK( subroutines["mode_perspective_transform_filter"]   = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_perspective_transform_filter") );
    GLCHK( subroutines["mode_height_processing_filter"]       = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_height_processing_filter" ) );
    GLCHK( subroutines["mode_roughness_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_roughness_filter" ) );
    GLCHK( subroutines["mode_roughness_color_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_roughness_color_filter" ) );
    GLCHK( subroutines["mode_remove_low_freq_filter"]         = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_remove_low_freq_filter" ) );
    GLCHK( subroutines["mode_grunge_filter"]                  = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_grunge_filter" ) );
    GLCHK( subroutines["mode_grunge_randomization_filter"]    = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_grunge_randomization_filter" ) );
    GLCHK( subroutines["mode_grunge_normal_warp_filter"]      = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_grunge_normal_warp_filter" ) );
    GLCHK( subroutines["mode_normal_angle_correction_filter"] = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_normal_angle_correction_filter" ) );
    GLCHK( subroutines["mode_add_noise_filter"]               = glGetSubroutineIndex(program->programId(),GL_FRAGMENT_SHADER,"mode_add_noise_filter" ) );
#endif

    makeScreenQuad();

    averageColorFBO = NULL;
    samplerFBO1     = NULL;
    samplerFBO2     = NULL;
    OpenGLFramebufferObject::create(averageColorFBO,256,256);
    OpenGLFramebufferObject::create(samplerFBO1,1024,1024);
    OpenGLFramebufferObject::create(samplerFBO2,1024,1024);

    auxFBO1 = NULL;
    auxFBO2 = NULL;
    auxFBO3 = NULL;
    auxFBO4 = NULL;
    for(int i = 0; i < 3 ; i++)
    {
        auxFBO0BMLevels[i] = NULL;
        auxFBO1BMLevels[i] = NULL;
        auxFBO2BMLevels[i] = NULL;
    }
    paintFBO   = NULL;

    emit readyGL();
}

void OpenGLImageEditor::paintGL()
{
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
        //if (!activeImage) return;
        // Since grunge map can be different we need to calculate ratio each time.
        if (paintFBO != NULL)
        {
            fboRatio = float(paintFBO->width())/paintFBO->height();
            orthographicProjHeight = (1+zoom)/windowRatio;
            orthographicProjWidth  = (1+zoom)/fboRatio;
        }
        GLCHK( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
        GLCHK( glDisable(GL_CULL_FACE) );
        GLCHK( glDisable(GL_DEPTH_TEST) );
        // Positions.
        glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);
        // Indices
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[2]);

        QOpenGLFramebufferObject *activeFBO     = paintFBO;

#ifdef USE_OPENGL_330
        program = filter_programs["mode_normal_filter"];
        program->bind();
#else
        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );
#endif

        // Displaying new image
        activeFBO->bindDefault();
        program->setUniformValue("quad_draw_mode", 1);

        GLCHK( glViewport(0,0,width(),height()) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, activeFBO->texture()) );

        QMatrix4x4 m;
        m.ortho(0,orthographicProjWidth,0,orthographicProjHeight,-1,1);
        GLCHK( program->setUniformValue("ProjectionMatrix", m) );
        m.setToIdentity();
        m.translate(xTranslation,yTranslation,0);
        GLCHK( program->setUniformValue("ModelViewMatrix", m) );
        GLCHK( program->setUniformValue("material_id", int(-1)) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        GLCHK( program->setUniformValue("quad_draw_mode", int(0)) );
    }
}

void OpenGLImageEditor::render()
{
    if (!activeImage)
        return;

    // Since grunge map can be different we need to calculate ratio each time.
    if (activeImage->getFBO())
    {
        fboRatio = float(activeImage->getFBO()->width())/activeImage->getFBO()->height();
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

    // Positions.
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);
    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[2]);

    QOpenGLFramebufferObject *activeFBO = activeImage->getFBO();

    // Images which depend on others will not be affected by UV changes again.
    bool bTransformUVs = true;
    bool bSkipStandardProcessing = false;

    // Do not process images when disabled.
    if((activeImage->isSkippingProcessing()) && (activeImage->getTextureType() != MATERIAL_TEXTURE))
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
            activeImage->resizeFBO(resize_width,resize_height);
            // Pointers were changed in resize function.
            activeFBO = activeImage->getFBO();
            bSkipStandardProcessing = true;
            break;
        default:
            break;
        }

        // Create or resize when image was changed.
        OpenGLFramebufferObject::resize(auxFBO1,activeFBO->width(),activeFBO->height());
        OpenGLFramebufferObject::resize(auxFBO2,activeFBO->width(),activeFBO->height());
        OpenGLFramebufferObject::resize(auxFBO3,activeFBO->width(),activeFBO->height());
        OpenGLFramebufferObject::resize(auxFBO4,activeFBO->width(),activeFBO->height());
        // Allocate aditional FBOs when conversion from BaseMap is enabled.
        if(activeImage->getTextureType() == DIFFUSE_TEXTURE && activeImage->bConversionBaseMap)
        {
            for(int i = 0; i < 3 ; i++)
            {
                OpenGLFramebufferObject::resize(auxFBO0BMLevels[i],activeFBO->width()/pow(2,i+1),activeFBO->height()/pow(2,i+1));
                OpenGLFramebufferObject::resize(auxFBO1BMLevels[i],activeFBO->width()/pow(2,i+1),activeFBO->height()/pow(2,i+1));
                OpenGLFramebufferObject::resize(auxFBO2BMLevels[i],activeFBO->width()/pow(2,i+1),activeFBO->height()/pow(2,i+1));
            }
        }
        else
        {
            // Delete unnecessary FBOs (I know that this is stupid...)
            int small_w_h = 1;
            for(int i = 0; i < 3 ; i++)
            {
                OpenGLFramebufferObject::resize(auxFBO0BMLevels[i],small_w_h,small_w_h);
                OpenGLFramebufferObject::resize(auxFBO1BMLevels[i],small_w_h,small_w_h);
                OpenGLFramebufferObject::resize(auxFBO2BMLevels[i],small_w_h,small_w_h);
            }
        }

        GLCHK( program->bind() );
        GLCHK( program->setUniformValue("gui_image_type", activeImage->getTextureType()) );
        openGL330ForceTexType = activeImage->getTextureType();
        GLCHK( program->setUniformValue("gui_depth", float(1.0)) );
        GLCHK( program->setUniformValue("gui_mode_dgaussian", 1) );
        GLCHK( program->setUniformValue("material_id", int(activeImage->currentMaterialIndex) ) );

        if(activeImage->isFirstDraw())
        {
            resetView();
            qDebug() << "Doing first draw of" << PostfixNames::getTextureName(activeImage->getTextureType()) << " texture.";
            activeImage->setFirstDraw(false);
        }

        // Skip all precessing when material tab is selected.
        if(activeImage->getTextureType() == MATERIAL_TEXTURE)
        {
            bSkipStandardProcessing = true;
            GLCHK( program->setUniformValue("material_id", int(-1) ) );
        }
        if(activeImage->getTextureType() == GRUNGE_TEXTURE)
        {
            bTransformUVs = false;
            GLCHK( program->setUniformValue("material_id", int(MATERIALS_DISABLED) ) );
        }

        GLCHK( glActiveTexture(GL_TEXTURE10) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, targetImageMaterial->getTexture()->textureId()) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );

        //if(int(activeImage->currentMaterialIndeks) < 0)
        //{
        copyTex2FBO(activeImage->getTexture()->textureId(),activeImage->getFBO());
        //}

        // In some cases the output image will be taken from other sources.
        switch(activeImage->getTextureType())
        {
        case(NORMAL_TEXTURE):
        {
            // Choose proper action.
            switch(activeImage->getInputImageType())
            {
            case(INPUT_FROM_NORMAL_INPUT):
                if(conversionType == CONVERT_FROM_H_TO_N)
                {
                    applyHeightToNormal(targetImageHeight->getFBO(),activeFBO);
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

                    copyTex2FBO(targetImageHeight->getTexture()->textureId(),activeFBO);
                    applyAllUVsTransforms(activeFBO);
                    copyFBO(activeFBO,auxFBO1);
                    applyHeightToNormal(auxFBO1,activeFBO);
                    GLCHK( program->setUniformValue("gui_image_type", activeImage->getTextureType()) );
                    openGL330ForceTexType = activeImage->getTextureType();
                    bTransformUVs = false;
                }
                else
                {
                    copyTex2FBO(targetImageHeight->getTexture()->textureId(),activeFBO);
                }
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                applyHeightToNormal(targetImageHeight->getFBO(),activeFBO);

                if(!targetImageHeight->isSkippingProcessing())
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
            switch(activeImage->getInputImageType())
            {
            case(INPUT_FROM_SPECULAR_INPUT):
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                copyTex2FBO(targetImageHeight->getTexture()->textureId(),activeFBO);
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                copyFBO(targetImageHeight->getFBO(),activeFBO);
                if(!targetImageHeight->isSkippingProcessing())
                    bTransformUVs = false;
                break;
            case(INPUT_FROM_DIFFUSE_INPUT):
                copyTex2FBO(targetImageDiffuse->getTexture()->textureId(), activeFBO);
                break;
            case(INPUT_FROM_DIFFUSE_OUTPUT):
                copyFBO(targetImageDiffuse->getFBO(),activeFBO);
                if(!targetImageDiffuse->isSkippingProcessing())
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
            switch(activeImage->getInputImageType())
            {
            case(INPUT_FROM_OCCLUSION_INPUT):
                if(conversionType == CONVERT_FROM_HN_TO_OC)
                {
                    // Ambient occlusion is calculated from normal and height map.
                    // Skip some parts of the processing.
                    applyOcclusionFilter(targetImageHeight->getFBO()->texture(),targetImageNormal->getFBO()->texture(),activeFBO);
                    bSkipStandardProcessing =  true;
                    if(!targetImageHeight->isSkippingProcessing() && !targetImageNormal->isSkippingProcessing())
                        bTransformUVs = false;
                    qDebug() << "Calculation AO from Normal and Height";
                }
                break;
            case(INPUT_FROM_HI_NI):
                // Ambient occlusion is calculated from normal and height map.
                // Skip some parts of the processing.
                applyOcclusionFilter(targetImageHeight->getTexture()->textureId(), targetImageNormal->getTexture()->textureId(), activeFBO);
                break;
            case(INPUT_FROM_HO_NO):
                applyOcclusionFilter(targetImageHeight->getFBO()->texture(),targetImageNormal->getFBO()->texture(),activeFBO);
                if(!targetImageHeight->isSkippingProcessing() && !targetImageNormal->isSkippingProcessing())
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
                applyNormalToHeight(activeImage,targetImageNormal->getFBO(),activeFBO,auxFBO1);
                applyCPUNormalizationFilter(auxFBO1,activeFBO);
                applyAddNoiseFilter(activeFBO,auxFBO1);
                copyFBO(auxFBO1,activeFBO);

                targetImageHeight->updateTextureFromFBO(activeFBO);
                if(!targetImageNormal->isSkippingProcessing())
                    bTransformUVs = false;
            }
            break;
        } // End of HEIGHT_TEXTURE case.

        case(ROUGHNESS_TEXTURE):
        {
            // Choose proper action.
            switch(activeImage->getInputImageType())
            {
            case(INPUT_FROM_ROUGHNESS_INPUT):
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                copyTex2FBO(targetImageHeight->getTexture()->textureId(), activeFBO);
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                copyFBO(targetImageHeight->getFBO(),activeFBO);
                if(!targetImageHeight->isSkippingProcessing())
                    bTransformUVs = false;
                break;
            case(INPUT_FROM_DIFFUSE_INPUT):
                copyTex2FBO(targetImageDiffuse->getTexture()->textureId(), activeFBO);
                break;
            case(INPUT_FROM_DIFFUSE_OUTPUT):
                copyFBO(targetImageDiffuse->getFBO(),activeFBO);
                if(!targetImageDiffuse->isSkippingProcessing())
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
            switch(activeImage->getInputImageType())
            {
            case(INPUT_FROM_METALLIC_INPUT):
                break;
            case(INPUT_FROM_HEIGHT_INPUT):
                copyTex2FBO(targetImageHeight->getTexture()->textureId(), activeFBO);
                break;
            case(INPUT_FROM_HEIGHT_OUTPUT):
                copyFBO(targetImageHeight->getFBO(),activeFBO);
                if(!targetImageHeight->isSkippingProcessing())
                    bTransformUVs = false;
                break;
            case(INPUT_FROM_DIFFUSE_INPUT):
                copyTex2FBO(targetImageDiffuse->getTexture()->textureId(), activeFBO);
                break;
            case(INPUT_FROM_DIFFUSE_OUTPUT):
                copyFBO(targetImageDiffuse->getFBO(),activeFBO);
                if(!targetImageDiffuse->isSkippingProcessing())
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
            if(activeImage->getTextureType() < MATERIAL_TEXTURE)
            {
                copyTex2FBO(targetImageGrunge->getFBO()->texture(),auxFBO2);
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

            if(activeImage->getTextureType() != HEIGHT_TEXTURE &&
                    activeImage->getTextureType() != NORMAL_TEXTURE &&
                    activeImage->getTextureType() != OCCLUSION_TEXTURE &&
                    activeImage->getTextureType() != ROUGHNESS_TEXTURE)
            {
                // Hue manipulation.
                applyColorHueFilter(activeFBO,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(BasicProp.GrayScale.EnableGrayScale ||
                    activeImage->getTextureType() == ROUGHNESS_TEXTURE ||
                    activeImage->getTextureType() == OCCLUSION_TEXTURE ||
                    activeImage->getTextureType() == HEIGHT_TEXTURE )
            {
                applyGrayScaleFilter(auxFBO1,activeFBO);
            }
            else
            {
                copyFBO(auxFBO1,activeFBO);
            }

            // Specular manipulation.
            if(SurfaceDetailsProp.EnableSurfaceDetails &&
                    activeImage->getTextureType() != HEIGHT_TEXTURE)
            {
                applyDGaussiansFilter(activeFBO,auxFBO2,auxFBO1);
                //copyFBO(auxFBO1,activeFBO);
                applyContrastFilter(auxFBO1,activeFBO);
            }

            // Removing shading.
            if(activeImage->getProperties()->EnableRemoveShading)
            {
                applyRemoveLowFreqFilter(activeFBO,auxFBO1,auxFBO2);
                copyFBO(auxFBO2,activeFBO);

                applyGaussFilter(activeFBO,auxFBO2,auxFBO1,1);
                applyInverseColorFilter(auxFBO1,auxFBO2);
                copyFBO(auxFBO2,auxFBO1);
                applyOverlayFilter(activeFBO,auxFBO1,auxFBO2);

                applyRemoveShadingFilter(auxFBO2, targetImageOcclusion->getFBO(), activeFBO, auxFBO1);
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

            if(activeImage->getTextureType() != NORMAL_TEXTURE)
            {
                applyHeightProcessingFilter(activeFBO,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            // Roughness color mapping.
            // Use the same filters for both metallic and roughness, because they are almost the same.
            if( (activeImage->getTextureType() == ROUGHNESS_TEXTURE ||
                 activeImage->getTextureType() == METALLIC_TEXTURE )
                    && RMFilterProp.Filter == COLOR_FILTER::Noise )
            {
                // Processing surface.
                applyRoughnessFilter(activeFBO,auxFBO2,auxFBO1);
                copyFBO(auxFBO1,activeFBO);
            }

            if(activeImage->getTextureType() == ROUGHNESS_TEXTURE ||
                    activeImage->getTextureType() == METALLIC_TEXTURE)
            {
                if(RMFilterProp.Filter == COLOR_FILTER::Color)
                {
                    applyRoughnessColorFilter(activeFBO,auxFBO1);
                    copyFBO(auxFBO1,activeFBO);
                }
            }

            // Height processing pipeline.

            // Normal processing pipeline.
            if(activeImage->getTextureType() == NORMAL_TEXTURE)
            {
                applyNormalsStepFilter(activeFBO,auxFBO1);
                // Apply normal mixer filter.
                if(NormalMixerProp.EnableMixer && activeImage->getNormalMixerInputTexture() != 0)
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
                if(activeImage->getTextureType() == DIFFUSE_TEXTURE &&
                        (activeImage->bConversionBaseMap || conversionType == CONVERT_FROM_D_TO_O ))
                {
                    // Create mipmaps.
                    copyTex2FBO(activeFBO->texture(),auxFBO0BMLevels[0]);
                    copyTex2FBO(activeFBO->texture(),auxFBO0BMLevels[1]);
                    copyTex2FBO(activeFBO->texture(),auxFBO0BMLevels[2]);
                    // Calculate normal for orginal image.
                    activeImage->getBaseMapConvLevelProperties()[0].fromProperty(BaseMapToOthersProp.LevelSmall);
                    activeImage->getBaseMapConvLevelProperties()[1].fromProperty(BaseMapToOthersProp.LevelMedium);
                    activeImage->getBaseMapConvLevelProperties()[2].fromProperty(BaseMapToOthersProp.LevelBig);
                    activeImage->getBaseMapConvLevelProperties()[3].fromProperty(BaseMapToOthersProp.LevelHuge);
                    applyBaseMapConversion(activeFBO,auxFBO2,auxFBO1,activeImage->getBaseMapConvLevelProperties()[0]);

                    // Calulcate normal for mipmaps.
                    for(int i = 0 ; i < 3 ; i++)
                    {
                        applyBaseMapConversion(auxFBO0BMLevels[i],auxFBO1BMLevels[i],auxFBO2BMLevels[i],activeImage->getBaseMapConvLevelProperties()[i+1]);
                    }

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
                        applyNormalToHeight(targetImageHeight,activeFBO,auxFBO1,auxFBO2);
                        applyCPUNormalizationFilter(auxFBO2,auxFBO1);
                        applyAddNoiseFilter(auxFBO1,auxFBO2);
                        copyFBO(auxFBO2,auxFBO1);

                    }
                    else if(activeImage->bConversionBaseMapShowHeightTexture)
                    {
                        applyNormalToHeight(targetImageHeight,activeFBO,auxFBO1,auxFBO2);
                        applyCPUNormalizationFilter(auxFBO2,activeFBO);
                    }
                } // End of base map conversion.
            }
        } // End of skip standard processing.


        // Copy the conversion results to proper textures.
        switch(conversionType)
        {
        case(CONVERT_FROM_H_TO_N):
            if(activeImage->getTextureType() == NORMAL_TEXTURE)
            {
                copyFBO(activeFBO,targetImageNormal->getFBO());
                targetImageNormal->updateTextureFromFBO(targetImageNormal->getFBO());
            }
            break;
        case(CONVERT_FROM_N_TO_H):
            if(activeImage->getTextureType() == HEIGHT_TEXTURE)
            {
                qDebug() << "Changing reference image of height";
            }
            break;
        case(CONVERT_FROM_D_TO_O):
            copyFBO(activeFBO,targetImageNormal->getFBO());
            targetImageNormal->updateTextureFromFBO(targetImageNormal->getFBO());
            copyFBO(auxFBO1,targetImageHeight->getFBO());
            targetImageHeight->updateTextureFromFBO(targetImageHeight->getFBO());
            applyOcclusionFilter(targetImageHeight->getTexture()->textureId(), targetImageNormal->getTexture()->textureId(), targetImageOcclusion->getFBO());
            targetImageOcclusion->updateTextureFromFBO(targetImageOcclusion->getFBO());
            copyTex2FBO(activeImage->getTexture()->textureId(), targetImageSpecular->getFBO());
            targetImageSpecular->updateTextureFromFBO(targetImageSpecular->getFBO());
            copyTex2FBO(activeImage->getTexture()->textureId(), targetImageRoughness->getFBO());
            targetImageRoughness->updateTextureFromFBO(targetImageRoughness->getFBO());
            copyTex2FBO(activeImage->getTexture()->textureId(), targetImageMetallic->getFBO());
            targetImageMetallic->updateTextureFromFBO(targetImageMetallic->getFBO());
            break;
        case(CONVERT_FROM_HN_TO_OC):
            //copyFBO(activeFBO,targetImageOcclusion->ref_fbo);
            copyFBO(activeFBO,targetImageOcclusion->getFBO());
            targetImageOcclusion->updateTextureFromFBO(activeFBO);
            break;
        default:
            break;
        }

        activeFBO = activeImage->getFBO();

    } // End of skip processing

    bSkipProcessing = false;
    conversionType  = CONVERT_NONE;

    if(!bShadowRender)
    {
        GLCHK(OpenGLFramebufferObject::resize(renderFBO,activeFBO->width(),activeFBO->height()));
        GLCHK( program->setUniformValue("material_id", int(-1)) );
        GLCHK(applyNormalFilter(activeFBO,renderFBO));
    }
    emit rendered();
}

void OpenGLImageEditor::showEvent(QShowEvent* event)
{
    QWidget::showEvent( event );
    resetView();
}

void OpenGLImageEditor::resizeFBO(int width, int height)
{
    conversionType = CONVERT_RESIZE;
    resize_width   = width;
    resize_height  = height;
}

void OpenGLImageEditor::resetView()
{
    if (!activeImage) return;

    makeCurrent();

    zoom = 0;
    windowRatio = float(width())/height();
    fboRatio    = float(activeImage->getFBO()->width())/activeImage->getFBO()->height();
    // OpenGL window dimensions.
    orthographicProjHeight = (1+zoom)/windowRatio;
    orthographicProjWidth = (1+zoom)/fboRatio;

    if(orthographicProjWidth < 1.0)
    {
        // Fit x direction.
        zoom = fboRatio - 1;
        orthographicProjHeight = (1+zoom)/windowRatio;
        orthographicProjWidth = (1+zoom)/fboRatio;
    }

    if(orthographicProjHeight < 1.0)
    {
        // Fit y direction.
        zoom = windowRatio - 1;
        orthographicProjHeight = (1+zoom)/windowRatio;
        orthographicProjWidth = (1+zoom)/fboRatio;
    }

    // Set the image in the center.
    xTranslation = orthographicProjWidth /2;
    yTranslation = orthographicProjHeight/2;
}

void OpenGLImageEditor::resizeGL(int width, int height)
{
    windowRatio = float(width)/height;
    if (isValid())
    {
        GLCHK( glViewport(0, 0, width, height) );
        if (activeImage && activeImage->getFBO())
        {
            fboRatio = float(activeImage->getFBO()->width())/activeImage->getFBO()->height();
            orthographicProjHeight = (1+zoom)/windowRatio;
            orthographicProjWidth = (1+zoom)/fboRatio;
        }
        else
        {
            qWarning() << Q_FUNC_INFO;
            if (!activeImage)
                qWarning() << "  activeImage is null";
            else if (!activeImage->getFBO())
                qWarning() << "  activeImage->getFBO() is null";
        }
    }
    else
    {
        qDebug() << Q_FUNC_INFO << "invalid context.";
    }

    resetView();
}

void OpenGLImageEditor::setActiveImage(Image *ptr)
{
    activeImage = ptr;
    update();
}

void OpenGLImageEditor::enableShadowRender(bool enable)
{
    bShadowRender = enable;
}

void OpenGLImageEditor::setConversionType(ConversionType type)
{
    conversionType = type ;
}

ConversionType OpenGLImageEditor::getConversionType()
{
    return conversionType;
}

void OpenGLImageEditor::updateCornersPosition(QVector2D dc1, QVector2D dc2, QVector2D dc3, QVector2D dc4)
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

void OpenGLImageEditor::selectPerspectiveTransformMethod(int method)
{
    gui_perspective_mode = method;
    update();
}

void OpenGLImageEditor::selectUVManipulationMethod(UVManipulationMethods method)
{
    uvManilupationMethod = method;
    update();
}

void OpenGLImageEditor::updateCornersWeights(float w1, float w2, float w3, float w4)
{
    cornerWeights.setX(w1);
    cornerWeights.setY(w2);
    cornerWeights.setZ(w3);
    cornerWeights.setW(w4);
    update();
}

void OpenGLImageEditor::selectSeamlessMode(SeamlessMode mode)
{
    Image::seamlessMode = mode;
    update();
}

void OpenGLImageEditor::imageChanged()
{
    // Restore picking state when another property was changed.
    bToggleColorPicking = false;
}

void OpenGLImageEditor::applyNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                          QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );
#endif

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    outputFBO->bindDefault();
}

void OpenGLImageEditor::applyHeightToNormal(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_height_to_normal"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_to_normal"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("gui_hn_conversion_depth", activeImage->getConversionHNDepth()) );
    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyColorHueFilter(  QOpenGLFramebufferObject *inputFBO,
                                              QOpenGLFramebufferObject *outputFBO)
{
    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
#ifdef USE_OPENGL_330
    program = filter_programs["mode_color_hue_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_color_hue_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("gui_hue"   , float(activeImage->getProperties()->Basic.ColorHue)) );

    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    outputFBO->bindDefault();
}

void OpenGLImageEditor::applyPerspectiveTransformFilter(  QOpenGLFramebufferObject *inputFBO,
                                                          QOpenGLFramebufferObject *outputFBO)
{
    // When materials texture is enabled, UV transformations are disabled.
    if(Image::currentMaterialIndex != MATERIALS_DISABLED)
    {
        copyFBO(inputFBO,outputFBO);
        return;
    }

#ifdef USE_OPENGL_330
    program = filter_programs["mode_perspective_transform_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_perspective_transform_filter"]) );
#endif

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    if(activeImage->getTextureType() == GRUNGE_TEXTURE)
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
    inputFBO->bindDefault();
}

void OpenGLImageEditor::applyGaussFilter(QOpenGLFramebufferObject *sourceFBO,
                                         QOpenGLFramebufferObject *auxFBO,
                                         QOpenGLFramebufferObject *outputFBO,int no_iter,float w )
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_gauss_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );
#endif

    GLCHK( program->setUniformValue("gui_gauss_radius", no_iter) );
    if( w == 0)
    {
        GLCHK( program->setUniformValue("gui_gauss_w", float(no_iter)) );
    }
    else
    {
        GLCHK( program->setUniformValue("gui_gauss_w", float(w)) );
    }

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

void OpenGLImageEditor::applyInverseColorFilter(QOpenGLFramebufferObject *inputFBO,
                                                QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_invert_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_invert_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyRemoveShadingFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *aoMaskFBO,
                                                 QOpenGLFramebufferObject *refFBO,
                                                 QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_ao_cancellation_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_ao_cancellation_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("gui_remove_shading",RemoveShadingProp.RemoveShadingByGaussian));
    GLCHK( program->setUniformValue("gui_ao_cancellation",RemoveShadingProp.AOCancellation ));

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

void OpenGLImageEditor::applyRemoveLowFreqFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *,
                                                 QOpenGLFramebufferObject *outputFBO)
{
    applyGaussFilter(inputFBO,samplerFBO1,samplerFBO2,RemoveShadingProp.LowFrequencyFilterRadius*50);

    // Calculate the average color on CPU.
    // Copy large file to smaller FBO to save time.
    applyNormalFilter(inputFBO,averageColorFBO);

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, averageColorFBO->texture()) );

    GLint textureWidth, textureHeight;
    GLCHK( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH , &textureWidth ) );
    GLCHK( glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight) );

    float* img = new float[textureWidth*textureHeight*3];
    float ave_color[3] = {0,0,0};

    GLCHK( glGetTexImage(	GL_TEXTURE_2D,0,GL_RGB,GL_FLOAT,img) );

    for(int i = 0 ; i < textureWidth*textureHeight ; i++)
    {
        for(int c = 0 ; c < 3 ; c++)
        {
            ave_color[c] += img[3*i+c];
        }
    }

    // Normalization sum.
    ave_color[0] /= (textureWidth*textureHeight);
    ave_color[1] /= (textureWidth*textureHeight);
    ave_color[2] /= (textureWidth*textureHeight);
    delete[] img;

    QVector3D aveColor = QVector3D(ave_color[0],ave_color[1],ave_color[2]);

#ifdef USE_OPENGL_330
    program = filter_programs["mode_remove_low_freq_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_remove_low_freq_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applyOverlayFilter(QOpenGLFramebufferObject *layerAFBO,
                                           QOpenGLFramebufferObject *layerBFBO,
                                           QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_overlay_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_overlay_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

void OpenGLImageEditor::applySeamlessLinearFilter(QOpenGLFramebufferObject *inputFBO,
                                                  QOpenGLFramebufferObject *outputFBO)
{
    // When materials texture is enabled, UV transformations are disabled.
    if(Image::currentMaterialIndex != MATERIALS_DISABLED)
    {
        copyFBO(inputFBO,outputFBO);
        return;
    }
    switch(Image::seamlessContrastImageType)
    {
    default:
    case(INPUT_FROM_HEIGHT_INPUT):
        //copyFBO(targetImageHeight->ref_fbo,activeImage->aux2_fbo);
        copyTex2FBO(targetImageHeight->getTexture()->textureId(), auxFBO2);
        break;
    case(INPUT_FROM_DIFFUSE_INPUT):
        //copyFBO(targetImageDiffuse->ref_fbo,activeImage->aux2_fbo);
        copyTex2FBO(targetImageDiffuse->getTexture()->textureId(), auxFBO2);
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

#ifdef USE_OPENGL_330
    program = filter_programs["mode_seamless_linear_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_seamless_linear_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applySeamlessFilter(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    // When materials texture is enabled, UV transformations are disabled.
    if(Image::currentMaterialIndex != MATERIALS_DISABLED)
    {
        copyFBO(inputFBO,outputFBO);
        return;
    }
    switch(Image::seamlessContrastImageType)
    {
    default:
    case(INPUT_FROM_HEIGHT_INPUT):
        copyTex2FBO(targetImageHeight->getTexture()->textureId(), auxFBO1);
        break;
    case(INPUT_FROM_DIFFUSE_INPUT):
        copyTex2FBO(targetImageDiffuse->getTexture()->textureId(), auxFBO1);
        break;
    };

    // When translations are applied first one has to translate.
    // else the contrast mask image.
    if(Image::bSeamlessTranslationsFirst)
    {
        // The output is save to auxFBO2.
        applyPerspectiveTransformFilter(auxFBO1,outputFBO);
    }

#ifdef USE_OPENGL_330
    program = filter_programs["mode_seamless_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_seamless_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("make_seamless_radius" , Image::seamlessSimpleModeRadius) );
    GLCHK( program->setUniformValue("gui_seamless_contrast_strenght", Image::seamlessContrastStrength) );
    GLCHK( program->setUniformValue("gui_seamless_contrast_power", Image::seamlessContrastPower) );
    GLCHK( program->setUniformValue("gui_seamless_mode", (int)Image::seamlessMode) );
    GLCHK( program->setUniformValue("gui_seamless_mirror_type", Image::seamlessMirroModeType) );

    // Send random angles.
    QMatrix3x3 random_angles;
    for(int i = 0; i < 9; i++)
        random_angles.data()[i] = Image::randomTilingMode.angles[i];

    GLCHK( program->setUniformValue("gui_seamless_random_angles" , random_angles) );
    GLCHK( program->setUniformValue("gui_seamless_random_phase" , Image::randomTilingMode.common_phase) );
    GLCHK( program->setUniformValue("gui_seamless_random_inner_radius" , Image::randomTilingMode.inner_radius) );
    GLCHK( program->setUniformValue("gui_seamless_random_outer_radius" , Image::randomTilingMode.outer_radius) );

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

void OpenGLImageEditor::applyDGaussiansFilter(QOpenGLFramebufferObject *inputFBO,
                                              QOpenGLFramebufferObject *auxFBO,
                                              QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_gauss_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );
#endif

    GLCHK( program->setUniformValue("gui_gauss_radius", int(SurfaceDetailsProp.Radius)) );
    GLCHK( program->setUniformValue("gui_gauss_w", SurfaceDetailsProp.WeightA) );

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

#ifdef USE_OPENGL_330
    program = filter_programs["mode_dgaussians_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]) );
#endif

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

void OpenGLImageEditor::applyContrastFilter(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_constrast_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_constrast_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

    GLCHK( program->setUniformValue("gui_specular_contrast", SurfaceDetailsProp.Contrast) );
    // Not used since offset does the same.
    GLCHK( program->setUniformValue("gui_specular_brightness", 0.0f) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault());
}

void OpenGLImageEditor::applySmallDetailsFilter(QOpenGLFramebufferObject *inputFBO,
                                                QOpenGLFramebufferObject *auxFBO,
                                                QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_gauss_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );
#endif

    GLCHK( program->setUniformValue("gui_depth", BasicProp.DetailDepth) );
    GLCHK( program->setUniformValue("gui_gauss_radius", int(3.0)) );
    GLCHK( program->setUniformValue("gui_gauss_w", float(3.0)) );

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

#ifdef USE_OPENGL_330
    program = filter_programs["mode_dgaussians_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_dgaussians_filter"]) );
#endif

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

#ifdef USE_OPENGL_330
    program = filter_programs["mode_small_details_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_small_details_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

void OpenGLImageEditor::applyMediumDetailsFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *auxFBO,
                                                 QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_gauss_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );
#endif

    GLCHK( program->setUniformValue("gui_depth", BasicProp.DetailDepth) );
    GLCHK( program->setUniformValue("gui_gauss_radius", int(15.0)) );
    GLCHK( program->setUniformValue("gui_gauss_w", float(15.0)) );

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

#ifdef USE_OPENGL_330
    program = filter_programs["mode_medium_details_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_medium_details_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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
    copyFBO(auxFBO,outputFBO);
}

void OpenGLImageEditor::applyGrayScaleFilter(QOpenGLFramebufferObject *inputFBO,
                                             QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_gray_scale_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gray_scale_filter"]) );
#endif

    // There is a change if gray scale filter is used for convertion from diffuse
    // texture to others in any other case this filter works just a simple gray scale filter.
    // Check if the baseMapToOthers is enabled, if yes check is min and max values are defined.
    GLCHK( program->setUniformValue("gui_gray_scale_max_color_defined",false) );
    GLCHK( program->setUniformValue("gui_gray_scale_min_color_defined",false) );

    if(activeImage->bConversionBaseMap)
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

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

    GLCHK( program->setUniformValue("gui_gray_scale_preset",QVector3D(BasicProp.GrayScale.GrayScaleR,
                                                                      BasicProp.GrayScale.GrayScaleG,
                                                                      BasicProp.GrayScale.GrayScaleB)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyInvertComponentsFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_invert_components_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_invert_components_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

    GLCHK( program->setUniformValue("gui_inverted_components", QVector3D(BasicProp.ColorComponents.InvertRed,
                                                                           BasicProp.ColorComponents.InvertGreen,
                                                                           BasicProp.ColorComponents.InvertBlue)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applySharpenBlurFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *auxFBO,
                                               QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_sharpen_blur"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_sharpen_blur"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applyNormalsStepFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normals_step_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normals_step_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("gui_normals_step", BasicProp.NormalsStep) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyNormalMixerFilter(QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_mixer_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_mixer_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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
    GLCHK( glBindTexture(GL_TEXTURE_2D, activeImage->getNormalMixerInputTexture()->textureId()) );

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

void OpenGLImageEditor::applyPreSmoothFilter(  QOpenGLFramebufferObject *inputFBO,
                                               QOpenGLFramebufferObject *auxFBO,
                                               QOpenGLFramebufferObject *outputFBO,
                                               BaseMapConvLevelProperties& convProp)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_gauss_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_gauss_filter"]) );
#endif

    GLCHK( program->setUniformValue("gui_gauss_radius", int(convProp.conversionBaseMapPreSmoothRadius)) );
    GLCHK( program->setUniformValue("gui_gauss_w", float(convProp.conversionBaseMapPreSmoothRadius)) );

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

void OpenGLImageEditor::applySobelToNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                                 QOpenGLFramebufferObject *outputFBO,
                                                 BaseMapConvLevelProperties& convProp)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_sobel_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_sobel_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("gui_basemap_amp", convProp.conversionBaseMapAmplitude) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyNormalToHeight(Image *image,
                                            QOpenGLFramebufferObject *normalFBO,
                                            QOpenGLFramebufferObject *heightFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
    applyGrayScaleFilter(normalFBO,heightFBO);

#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_to_height"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_to_height"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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
    if(activeImage->bConversionBaseMap)
    {
        GLCHK( glActiveTexture(GL_TEXTURE2) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO4->texture()) );
    }

    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    for(int i = 0; i < image->getProperties()->NormalHeightConv.Huge ; i++)
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

    for(int i = 0; i < image->getProperties()->NormalHeightConv.VeryLarge ; i++)
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

    for(int i = 0; i < image->getProperties()->NormalHeightConv.Large ; i++)
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

    for(int i = 0; i < image->getProperties()->NormalHeightConv.Medium; i++)
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

    for(int i = 0; i < image->getProperties()->NormalHeightConv.Small; i++)
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

    for(int i = 0; i < image->getProperties()->NormalHeightConv.VerySmall; i++)
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

void OpenGLImageEditor::applyNormalAngleCorrectionFilter(QOpenGLFramebufferObject *inputFBO,
                                                         QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_angle_correction_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_angle_correction_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("base_map_angle_correction"  , BaseMapToOthersProp.AngleCorrection/180.0f*3.1415926f) );
    GLCHK( program->setUniformValue("base_map_angle_weight"      , BaseMapToOthersProp.AngleWeight) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyNormalExpansionFilter(QOpenGLFramebufferObject *inputFBO,
                                                   QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_expansion_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_expansion_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyMixNormalLevels(GLuint level0,
                                             GLuint level1,
                                             GLuint level2,
                                             GLuint level3,
                                             QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_mix_normal_levels_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_mix_normal_levels_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

    GLCHK( program->setUniformValue("gui_base_map_w0"  , activeImage->getProperties()->BaseMapToOthers.WeightSmall ) );
    GLCHK( program->setUniformValue("gui_base_map_w1"  , activeImage->getProperties()->BaseMapToOthers.WeightMedium ) );
    GLCHK( program->setUniformValue("gui_base_map_w2"  , activeImage->getProperties()->BaseMapToOthers.WeightBig) );
    GLCHK( program->setUniformValue("gui_base_map_w3"  , activeImage->getProperties()->BaseMapToOthers.WeightHuge ) );

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

void OpenGLImageEditor::applyCPUNormalizationFilter(QOpenGLFramebufferObject *inputFBO,
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
    if(Image::currentMaterialIndex != MATERIALS_DISABLED)
    {
        QImage maskImage = targetImageMaterial->getFBOImage();
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

#ifdef USE_OPENGL_330
    program = filter_programs["mode_normalize_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normalize_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

    GLCHK( outputFBO->bind() );
    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( program->setUniformValue("min_color",QVector3D(min[0],min[1],min[2])) );
    GLCHK( program->setUniformValue("max_color",QVector3D(max[0],max[1],max[2])) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );

    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyAddNoiseFilter(QOpenGLFramebufferObject *inputFBO,
                                            QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_add_noise_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_add_noise_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("gui_add_noise_amp"  , float(targetImageHeight->getProperties()->NormalHeightConv.NoiseLevel/100.0) ));

    GLCHK( glViewport(0,0,inputFBO->width(),inputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( outputFBO->bindDefault() );
}

void OpenGLImageEditor::applyBaseMapConversion(QOpenGLFramebufferObject *baseMapFBO,
                                               QOpenGLFramebufferObject *auxFBO,
                                               QOpenGLFramebufferObject *outputFBO,
                                               BaseMapConvLevelProperties& convProp)
{
    applyGrayScaleFilter(baseMapFBO,outputFBO);
    applySobelToNormalFilter(outputFBO,auxFBO,convProp);
    applyInvertComponentsFilter(auxFBO,baseMapFBO);
    applyPreSmoothFilter(baseMapFBO,auxFBO,outputFBO,convProp);

#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_expansion_filter"];
    program->bind();
#endif

    GLCHK( program->setUniformValue("gui_combine_normals" , 0) );
    GLCHK( program->setUniformValue("gui_filter_radius" , convProp.conversionBaseMapFilterRadius) );
    GLCHK( program->setUniformValue("gui_normal_flatting" , convProp.conversionBaseMapFlatness) );

    for(int i = 0; i < convProp.conversionBaseMapNoIters ; i ++)
    {
        copyFBO(outputFBO,auxFBO);
        applyNormalExpansionFilter(auxFBO,outputFBO);
    }

#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_expansion_filter"];
    program->bind();
#endif

    GLCHK( program->setUniformValue("gui_combine_normals" , 1) );
    GLCHK( program->setUniformValue("gui_mix_normals"   , convProp.conversionBaseMapMixNormals) );
    GLCHK( program->setUniformValue("gui_blend_normals" , convProp.conversionBaseMapBlending) );

    copyFBO(outputFBO,auxFBO);
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, baseMapFBO->texture()) );

    applyNormalExpansionFilter(auxFBO,outputFBO);
    GLCHK( glActiveTexture(GL_TEXTURE0) );

#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_expansion_filter"];
    program->bind();
#endif

    GLCHK( program->setUniformValue("gui_combine_normals" , 0 ) );
}

void OpenGLImageEditor::applyOcclusionFilter(GLuint height_tex,GLuint normal_tex,
                                             QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_occlusion_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_occlusion_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applyHeightProcessingFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_height_processing_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_processing_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

void OpenGLImageEditor::applyCombineNormalHeightFilter(QOpenGLFramebufferObject *normalFBO,
                                                       QOpenGLFramebufferObject *heightFBO,
                                                       QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_combine_normal_height_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_combine_normal_height_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applyRoughnessFilter(QOpenGLFramebufferObject *inputFBO,
                                             QOpenGLFramebufferObject *auxFBO,
                                             QOpenGLFramebufferObject *outputFBO)
{
    applyGaussFilter(inputFBO,auxFBO,outputFBO,int(RMFilterProp.NoiseFilter.Depth));
    copyFBO(outputFBO,auxFBO);

#ifdef USE_OPENGL_330
    program = filter_programs["mode_roughness_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_roughness_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applyRoughnessColorFilter(QOpenGLFramebufferObject *inputFBO,
                                                  QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_roughness_color_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_roughness_color_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

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

void OpenGLImageEditor::copyFBO(QOpenGLFramebufferObject *src,
                                QOpenGLFramebufferObject *dst)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );
#endif

    GLCHK( dst->bind() );
    GLCHK( glViewport(0,0,dst->width(),dst->height()) );
    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, src->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    src->bindDefault();
}

void OpenGLImageEditor::copyTex2FBO(GLuint src_tex_id,
                                    QOpenGLFramebufferObject *dst)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_normal_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_filter"]) );
#endif

    GLCHK( dst->bind() );
    GLCHK( glViewport(0,0,dst->width(),dst->height()) );

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, src_tex_id) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    dst->bindDefault();
}

void OpenGLImageEditor::applyAllUVsTransforms(QOpenGLFramebufferObject *inoutFBO)
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

void OpenGLImageEditor::applyGrungeImageFilter (QOpenGLFramebufferObject *inputFBO,
                                                QOpenGLFramebufferObject *outputFBO,
                                                QOpenGLFramebufferObject *grungeFBO)
{
    // Grunge is treated differently in normal texture.
    if(activeImage->getTextureType() == NORMAL_TEXTURE)
    {
#ifdef USE_OPENGL_330
        program = filter_programs["mode_height_to_normal"];
        program->bind();
        updateProgramUniforms(0);
#else
        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_height_to_normal"]) );
#endif

        GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
        GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );

        GLCHK( program->setUniformValue("gui_hn_conversion_depth", 2*GrungeOnImageProp.GrungeWeight * GrungeProp.OverallWeight) );
        GLCHK( glViewport(0,0,auxFBO3->width(),auxFBO3->height()) );
        GLCHK( auxFBO3->bind() );
        GLCHK( glBindTexture(GL_TEXTURE_2D, grungeFBO->texture()) );
        GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
        GLCHK( auxFBO3->bindDefault() );

#ifdef USE_OPENGL_330
        program = filter_programs["mode_normal_mixer_filter"];
        program->bind();
        updateProgramUniforms(0);
#else
        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_normal_mixer_filter"]) );
#endif

        GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
        GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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
#ifdef USE_OPENGL_330
        program = filter_programs["mode_grunge_filter"];
        program->bind();
        updateProgramUniforms(0);
#else
        GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_grunge_filter"]) );
#endif

        GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
        GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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

void OpenGLImageEditor::applyGrungeRandomizationFilter(QOpenGLFramebufferObject *inputFBO,
                                                       QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_grunge_randomization_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_grunge_randomization_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
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
    GLCHK( glBindTexture(GL_TEXTURE_2D, targetImageGrunge->getFBO()->texture()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGLImageEditor::applyGrungeWarpNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                                    QOpenGLFramebufferObject *outputFBO)
{
#ifdef USE_OPENGL_330
    program = filter_programs["mode_grunge_normal_warp_filter"];
    program->bind();
    updateProgramUniforms(0);
#else
    GLCHK( glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &subroutines["mode_grunge_normal_warp_filter"]) );
#endif

    GLCHK( program->setUniformValue("quad_scale", QVector2D(1.0,1.0)) );
    GLCHK( program->setUniformValue("quad_pos", QVector2D(0.0,0.0)) );
    GLCHK( program->setUniformValue("grunge_normal_warp"  , GrungeProp.NormalWarp) );

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    GLCHK( outputFBO->bind() );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, inputFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, targetImageNormal->getTexture()->textureId()) );
    GLCHK( glDrawElements(GL_TRIANGLES, 3*2, GL_UNSIGNED_INT, 0) );
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
}

void OpenGLImageEditor::updateProgramUniforms(int step)
{
    switch(step)
    {
    case(0):
        GLCHK( program->setUniformValue("gui_image_type", openGL330ForceTexType) );
        GLCHK( program->setUniformValue("gui_depth", float(1.0)) );
        GLCHK( program->setUniformValue("gui_mode_dgaussian", 1) );
        GLCHK( program->setUniformValue("material_id", int(activeImage->currentMaterialIndex) ) );

        if(activeImage->getTextureType() == MATERIAL_TEXTURE)
        {
            GLCHK( program->setUniformValue("material_id", int(-1) ) );
        }
        break;
    default:
        break;
    };
}

void OpenGLImageEditor::makeScreenQuad()
{
    int size = 2;
    QVector<QVector2D> texCoords = QVector<QVector2D>(size*size);
    QVector<QVector3D> vertices  = QVector<QVector3D>(size*size);
    int iter = 0;
    for (int i = 0; i < size; ++i)
    {
        for (int j = 0; j < size; ++j)
        {
            float offset = 0.5;
            float x = i/(size-1.0);
            float y = j/(size-1.0);
            vertices[iter]  = (QVector3D(x-offset,y-offset,0));
            texCoords[iter] = (QVector2D(x,y));
            iter++;
        }
    }

    glGenBuffers(3, &vbos[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float)*3, vertices.constData(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(float)*3,(void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(float)*2, texCoords.constData(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(float)*2,(void*)0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[2]);

    int no_triangles = 2*(size - 1)*(size - 1);
    QVector<GLuint> indices(no_triangles*3);
    iter = 0;
    for(int i = 0 ; i < size -1 ; i++)
    {
        for(int j = 0 ; j < size -1 ; j++)
        {
            GLuint i1 = i + j*size;
            GLuint i2 = i + (j+1)*size;
            GLuint i3 = i+1 + j*size;
            GLuint i4 = i+1 + (j+1)*size;
            indices[iter++] = (i1);
            indices[iter++] = (i3);
            indices[iter++] = (i2);
            indices[iter++] = (i2);
            indices[iter++] = (i3);
            indices[iter++] = (i4);
        }
    }

    GLCHK( glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * no_triangles * 3 , indices.constData(), GL_STATIC_DRAW) );
}


void OpenGLImageEditor::updateMousePosition()
{
    QPoint p = mapFromGlobal(QCursor::pos());
    cursorPhysicalXPosition =       double(p.x())/width() * orthographicProjWidth  - xTranslation;
    cursorPhysicalYPosition = (1.0-double(p.y())/height())* orthographicProjHeight - yTranslation;
    bSkipProcessing = true;
}

void OpenGLImageEditor::wheelEvent(QWheelEvent *event)
{
    if( event->delta() > 0)
        zoom-=0.1;
    else
        zoom+=0.1;
    if(zoom < -0.90)
        zoom = -0.90;

    updateMousePosition();

    //resizeGL(width(),height());
    windowRatio = float(width())/height();
    if (isValid())
    {
        GLCHK( glViewport(0, 0, width(), height()) );

        if (activeImage && activeImage->getFBO())
        {
            fboRatio = float(activeImage->getFBO()->width())/activeImage->getFBO()->height();
            orthographicProjHeight = (1+zoom)/windowRatio;
            orthographicProjWidth = (1+zoom)/fboRatio;
        }
        else
        {
            qWarning() << Q_FUNC_INFO;
            if (!activeImage)
                qWarning() << "  activeImage is null";
            else if (!activeImage->getFBO())
                qWarning() << "  activeImage->getFBO() is null";
        }
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

void OpenGLImageEditor::relativeMouseMoveEvent(int dx, int dy, bool* wrapMouse, Qt::MouseButtons buttons)
{
    if(activeImage->getTextureType() != GRUNGE_TEXTURE)
    {
        if(Image::currentMaterialIndex != MATERIALS_DISABLED &&
                buttons & Qt::LeftButton)
        {
            QMessageBox msgBox;
            msgBox.setText("Warning!");
            msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping when materials textures are enabled.");
            msgBox.setStandardButtons(QMessageBox::Cancel);
            msgBox.exec();
            return;
        }
    }

    if(activeImage->getTextureType() == MATERIAL_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of materials texture. This texture is static.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    if(activeImage->getTextureType() == GRUNGE_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of Grunge texture. Try Diffuse or height texture.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    if(activeImage->getTextureType() == OCCLUSION_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of occlusion texture. Try Diffuse or height texture.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    if(activeImage->getTextureType() == NORMAL_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of normal texture. Try Diffuse or height texture.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    if(activeImage->getTextureType() == METALLIC_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of metallic texture. Try Diffuse or height texture.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    if(activeImage->getTextureType() == ROUGHNESS_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of roughness texture. Try Diffuse or height texture.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    if(activeImage->getTextureType() == SPECULAR_TEXTURE &&
            buttons & Qt::LeftButton)
    {
        QMessageBox msgBox;
        msgBox.setText("Warning!");
        msgBox.setInformativeText("Sorry, but you cannot modify UV's mapping of specular texture. Try Diffuse or height texture.");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        return;
    }

    // Default position of corners.
    QVector2D defCorners[4];
    defCorners[0] = QVector2D(0,0) ;
    defCorners[1] = QVector2D(1,0) ;
    defCorners[2] = QVector2D(1,1) ;
    defCorners[3] = QVector2D(0,1) ;
    QVector2D mpos((cursorPhysicalXPosition+0.5), (cursorPhysicalYPosition+0.5));

    // Manipulate UV coordinates based on chosen method.
    switch(uvManilupationMethod)
    {
    case(UV_TRANSLATE):
        if(buttons & Qt::LeftButton)
        {
            // Drag image.
            setCursor(Qt::SizeAllCursor);
            // Move all corners.
            QVector2D averagePos(0.0,0.0);
            QVector2D dmouse = QVector2D(-dx*(float(orthographicProjWidth)/width()),dy*(float(orthographicProjHeight)/height()));
            for(int i = 0; i < 4 ; i++)
            {
                averagePos += cornerPositions[i]*0.25;
                if(activeImage->getTextureType() == GRUNGE_TEXTURE)
                    grungeCornerPositions[i] += dmouse;
                else
                    cornerPositions[i] += dmouse;
            }
            repaint();
        }
        break;
        // Grab corners in perspective correction tool.
    case(UV_GRAB_CORNERS):
        if(activeImage->getTextureType() == GRUNGE_TEXTURE)
            break;
        if(draggingCorner == -1)
        {
            setCursor(Qt::OpenHandCursor);
            for(int i = 0; i < 4 ; i++)
            {
                float dist = (mpos - defCorners[i]).length();
                if(dist < 0.2)
                {
                    setCursor(cornerCursors[i]);
                }
            }
        }

        if(buttons & Qt::LeftButton)
        {
            // Calculate distance from corners.
            if(draggingCorner == -1)
            {
                for(int i = 0; i < 4 ; i++)
                {
                    float dist = (mpos - defCorners[i]).length();
                    if(dist < 0.2)
                    {
                        draggingCorner = i;
                    }
                }
            }

            if(draggingCorner >=0 && draggingCorner < 4)
            {
                cornerPositions[draggingCorner] += QVector2D(-dx*(float(orthographicProjWidth)/width()),dy*(float(orthographicProjHeight)/height()));
            }
            repaint();
        }
        break;
    case(UV_SCALE_XY):
        if(activeImage->getTextureType() == GRUNGE_TEXTURE)
            break;
        setCursor(Qt::OpenHandCursor);

        if(buttons & Qt::LeftButton)
        {
            // Drag image.
            setCursor(Qt::SizeAllCursor);
            QVector2D dmouse = QVector2D(-dx*(float(orthographicProjWidth)/width()),dy*(float(orthographicProjHeight)/height()));
            cornerWeights.setX(cornerWeights.x()-dmouse.x());
            cornerWeights.setY(cornerWeights.y()-dmouse.y());
            repaint();
        }
        break;
    default:;
    }

    if (buttons & Qt::RightButton)
    {
        xTranslation += dx*(float(orthographicProjWidth)/width());
        yTranslation -= dy*(float(orthographicProjHeight)/height());
        setCursor(Qt::ClosedHandCursor);
    }

    // Mouse looping in 2D view window.
    *wrapMouse = (buttons & Qt::RightButton || buttons & Qt::LeftButton );

    updateMousePosition();
    if(bToggleColorPicking)
    {
        setCursor(Qt::UpArrowCursor);
    }

    update();
}

void OpenGLImageEditor::mousePressEvent(QMouseEvent *event)
{
    OpenGLWidgetBase::mousePressEvent(event);

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
        std::vector< unsigned char > pixels( 1 * 1 * 4 );
        glReadPixels(event->pos().x(), height()-event->pos().y(), 1, 1,GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
        QVector4D color(pixels[0],pixels[1],pixels[2],pixels[3]);
        qDebug() << "Picked pixel (" << event->pos().x() << " , " << height()-event->pos().y() << ") with color:" << color;

        QColor qcolor = QColor(pixels[0],pixels[1],pixels[2]);
        if(ptr_ABColor != NULL)
            ptr_ABColor->setValue(qcolor);
        toggleColorPicking(false);
    }
}

void OpenGLImageEditor::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::OpenHandCursor);
    draggingCorner = -1;
    event->accept();
    bSkipProcessing = true;
    repaint();
}

void OpenGLImageEditor::toggleColorPicking(bool toggle)
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

void OpenGLImageEditor::pickImageColor( QtnPropertyABColor* property)
{
    bool toggle = true;
    bToggleColorPicking = toggle;
    ptr_ABColor = property;
    if(toggle)
    {
        setCursor(Qt::UpArrowCursor);
    }
    else
    {
        setCursor(Qt::PointingHandCursor);
    }
    repaint();
}

void OpenGLImageEditor::copyRenderToPaintFBO()
{
    GLCHK(OpenGLFramebufferObject::resize(paintFBO,renderFBO->width(),renderFBO->height()));
    GLCHK( program->setUniformValue("material_id", int(-1)) );
    copyFBO(renderFBO,paintFBO);
    bRendering = false;
}
