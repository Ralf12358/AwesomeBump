#include "opengl3dimagewidget.h"

#include <QColor>
#include <QtWidgets>
#include <QtOpenGL>
#include <QtMath>

#include "image.h"
#include "openglerrorcheck.h"

OpenGL3DImageWidget::OpenGL3DImageWidget(QWidget* parent, OpenGL2DImageWidget* openGL2DImageWidget) :
    QOpenGLWidget(parent), openGL2DImageWidget(openGL2DImageWidget),
    meshArray(0), skyboxMeshArray(0), envMeshArray(0), quadMeshArray(0),
    skyBoxTextureCube(0),
    mouseUpdateIsQueued(false),
    blockMouseMovement(false),
    keyPressed((Qt::Key)0)
{
    centerCamCursor = QCursor(QPixmap(":/resources/cursors/centerCamCursor.png"));
    wrapMouse = true;
    setAcceptDrops(true);
    zoom                    = 60;
    lightPosition           = QVector4D(0,0.0,5.0,1);

    bToggleDiffuseView      = true;
    bToggleSpecularView     = true;
    bToggleOcclusionView    = true;
    bToggleHeightView       = true;
    bToggleNormalView       = true;
    bToggleRoughnessView    = true;
    bToggleMetallicView     = true;

    setCursor(Qt::PointingHandCursor);
    lightCursor = QCursor(QPixmap(":/resources/cursors/lightCursor.png"));

    // Post processing variables:
    colorFBO = NULL;
    outputFBO= NULL;
    auxFBO   = NULL;
    for(int i = 0; i < 4; i++)
    {
        glowInputColor[i]  = NULL;
        glowOutputColor[i] = NULL;
    }
    // initializing tone mapping FBOs with nulls
    for(int i = 0; i < 10; i++)
    {
        toneMipmaps[i] = NULL;
    }
    cameraInterpolation = 1.0;

    QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setSizePolicy(sizePolicy);

    settingsDialog = new Dialog3DGeneralSettings(this);
    connect(settingsDialog, SIGNAL (signalPropertyChanged()), this, SLOT (repaint()));
}

OpenGL3DImageWidget::~OpenGL3DImageWidget()
{
    makeCurrent();
    deleteFBOs();

    typedef std::map<std::string,QOpenGLShaderProgram*>::iterator it_type;
    qDebug() << "Removing filters:";
    for(it_type iterator = post_processing_programs.begin(); iterator != post_processing_programs.end(); iterator++)
    {
        qDebug() << "Removing program:" << QString(iterator->first.c_str());
        delete iterator->second;
    }
    delete lensFlareColorsTexture;
    delete lensDirtTexture;
    delete lensStarTexture;

    delete renderProgram;
    delete line_program;
    delete skybox_program;
    delete env_program;
    delete mesh;
    delete skybox_mesh;
    delete env_mesh;
    delete quad_mesh;
    delete skyBoxTextureCube;
    delete meshArray;
    delete skyboxMeshArray;
    delete envMeshArray;
    delete quadMeshArray;

    doneCurrent();
}

QSize OpenGL3DImageWidget::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize OpenGL3DImageWidget::sizeHint() const
{
    return QSize(500, 400);
}

void OpenGL3DImageWidget::show3DGeneralSettingsDialog()
{
    settingsDialog->show();
}

void OpenGL3DImageWidget::toggleDiffuseView(bool enable)
{
    bToggleDiffuseView = enable;
    update();
}

void OpenGL3DImageWidget::toggleSpecularView(bool enable)
{
    bToggleSpecularView = enable;
    update();
}

void OpenGL3DImageWidget::toggleOcclusionView(bool enable)
{
    bToggleOcclusionView = enable;
    update();
}

void OpenGL3DImageWidget::toggleNormalView(bool enable)
{
    bToggleNormalView = enable;
    update();
}

void OpenGL3DImageWidget::toggleHeightView(bool enable)
{
    bToggleHeightView = enable;
    update();
}

void OpenGL3DImageWidget::toggleRoughnessView(bool enable)
{
    bToggleRoughnessView = enable;
    update();

}
void OpenGL3DImageWidget::toggleMetallicView(bool enable)
{
    bToggleMetallicView = enable;
    update();
}

void OpenGL3DImageWidget::setCameraMouseSensitivity(int value)
{
    camera.setMouseSensitivity(value);
}

void OpenGL3DImageWidget::resetCameraPosition()
{
    camera.reset();
    newCamera.reset();
    cameraInterpolation = 1.0;
    emit changeCamPositionApplied(false);
    update();
}

void OpenGL3DImageWidget::toggleChangeCamPosition(bool toggle)
{
    if(!toggle)
    {
        setCursor(Qt::PointingHandCursor);
        keyPressed = (Qt::Key)0;
    }
    else
    {
        keyPressed = Qt::Key_Shift;
        setCursor(centerCamCursor);
    }
    update();
}

void OpenGL3DImageWidget::toggleMouseWrap(bool toggle)
{
    wrapMouse = toggle;
}

void OpenGL3DImageWidget::loadMeshFromFile()
{
    QString meshPath = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation).first();
    if (meshPath.isEmpty()) meshPath = QDir::currentPath();

    QFileDialog dialog(
                this,
                tr("Open Mesh File"),
                meshPath,
                tr("OBJ file format (*.obj *.OBJ );;"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadMeshFile(dialog.selectedFiles().first()))
    {}
}

bool OpenGL3DImageWidget::loadMeshFile(const QString& fileName)
{
    // Load new mesh.
    Mesh* newMesh = new Mesh(fileName);

    if(newMesh->isLoaded())
    {
        if(mesh) delete mesh;
        mesh = newMesh;
        if(meshArray) delete meshArray;
        meshArray = createVertexArray(mesh);
    }
    else
    {
        QMessageBox msgBox;
        msgBox.setText("Unable to load object file!");
        msgBox.setInformativeText("There was a problem loading the object file.\n"
                "Loader message:\n" + newMesh->getMeshLog());
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        delete newMesh;
    }

    update();
    return true;
}
void OpenGL3DImageWidget::chooseMeshFile(const QString& fileName)
{
    loadMeshFile(":/resources/meshes/" + fileName + ".obj");
}

void OpenGL3DImageWidget::chooseSkyBox(QString cubeMapName,bool bFirstTime)
{
    QStringList list;
    makeCurrent();
    list << QString(":/resources/skyboxes/" + cubeMapName + "/posx.jpg")
         << QString(":/resources/skyboxes/" + cubeMapName + "/negx.jpg")
         << QString(":/resources/skyboxes/" + cubeMapName + "/posy.jpg")
         << QString(":/resources/skyboxes/" + cubeMapName + "/negy.jpg")
         << QString(":/resources/skyboxes/" + cubeMapName + "/posz.jpg")
         << QString(":/resources/skyboxes/" + cubeMapName + "/negz.jpg");

    qDebug() << "Reading new cube map:" << list;
    bDiffuseMapBaked = false;

    if(skyBoxTextureCube) delete skyBoxTextureCube;
    skyBoxTextureCube = createTextureCube(list);

    if(!skyBoxTextureCube->isCreated())
    {
        qWarning() << "Cannot load cube map: check if images listed above exist.";
    }
    // Skip when loading first cube map.
    if(!bFirstTime)
        update();
    else
        qDebug() << "Skipping OpenGLWidget repainting during first Env. maps. load.";
}

void OpenGL3DImageWidget::updatePerformanceSettings(Display3DSettings settings)
{
    display3Dparameters = settings;
    update();
}

void OpenGL3DImageWidget::initializeGL()
{
    qDebug() << Q_FUNC_INFO;

    initializeOpenGLFunctions();

    GLCHK( glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS) );
    makeCurrent();

    QColor clearColor = QColor::fromCmykF(0.79, 0.79, 0.79, 0.0).dark();
    glClearColor(clearColor.redF(),
                 clearColor.greenF(),
                 clearColor.blackF(),
                 clearColor.alpha());

    GLCHK( glEnable(GL_DEPTH_TEST) );
    GLCHK( glEnable(GL_MULTISAMPLE) );
    GLCHK( glEnable(GL_DEPTH_TEST) );

    QOpenGLShader *vshader  = NULL;
    QOpenGLShader *fshader  = NULL;
    QOpenGLShader *tcshader = NULL;
    QOpenGLShader *teshader = NULL;
    QOpenGLShader *gshader  = NULL;

    qDebug() << "Loading quad (geometry shader)";
    gshader = new QOpenGLShader(QOpenGLShader::Geometry, this);
    QFile gFile(":/resources/shaders/plane.geom");
    gFile.open(QFile::ReadOnly);
    QTextStream in(&gFile);
    QString shaderCode = in.readAll();
    QString preamble = "#version 330 core\n"
                        "layout(triangle_strip, max_vertices = 3) out;\n";
    gshader->compileSourceCode(preamble+shaderCode);
    if (!gshader->log().isEmpty()) qDebug() << gshader->log();
    else qDebug() << "done";

    qDebug() << "Loading quad (vertex shader)";
    vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile(":/resources/shaders/plane.vert");
    if (!vshader->log().isEmpty()) qDebug() << vshader->log();
    else qDebug() << "done";

    qDebug() << "Loading quad (tessellation control shader)";
    tcshader = new QOpenGLShader(QOpenGLShader::TessellationControl, this);
    tcshader->compileSourceFile(":/resources/shaders/plane.tcs.vert");
    if (!tcshader->log().isEmpty()) qDebug() << tcshader->log();
    else qDebug() << "done";

    qDebug() << "Loading quad (tessellation evaluation shader)";
    teshader = new QOpenGLShader(QOpenGLShader::TessellationEvaluation, this);
    teshader->compileSourceFile(":/resources/shaders/plane.tes.vert");
    if (!teshader->log().isEmpty()) qDebug() << teshader->log();
    else qDebug() << "done";

    // Load parsed shader.
    renderProgram = new QOpenGLShaderProgram(this);

    // Load custom fragment shader.
    qDebug() << "Loading parsed glsl fragment shader";
    QOpenGLShader* pfshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    pfshader->compileSourceFile(":/resources/shaders/awesombump.frag");
    if (!pfshader->log().isEmpty())
        qDebug() << pfshader->log();
    else
        qDebug() << "done";

    renderProgram->addShader(tcshader);
    renderProgram->addShader(teshader);
    renderProgram->addShader(vshader);
    renderProgram->addShader(pfshader);
    renderProgram->addShader(gshader);
    renderProgram->bindAttributeLocation("FragColor",     0);
    renderProgram->bindAttributeLocation("FragNormal",    1);
    renderProgram->bindAttributeLocation("FragGlowColor", 2);
    renderProgram->bindAttributeLocation("FragPosition",  3);
    GLCHK(renderProgram->link());

    delete pfshader;

    GLCHK(renderProgram->bind());
    renderProgram->setUniformValue("texDiffuse",           0);
    renderProgram->setUniformValue("texNormal",            1);
    renderProgram->setUniformValue("texSpecular",          2);
    renderProgram->setUniformValue("texHeight",            3);
    renderProgram->setUniformValue("texSSAO",              4);
    renderProgram->setUniformValue("texRoughness",         5);
    renderProgram->setUniformValue("texMetallic",          6);
    renderProgram->setUniformValue("texMaterial",          7);
    renderProgram->setUniformValue("texPrefilteredEnvMap", 8);
    renderProgram->setUniformValue("texSourceEnvMap",      9);

    GLCHK(renderProgram->release());

    // Lines shader
    qDebug() << "Compiling lines program...";
    preamble = QString("#version 330 core\n") +
            "layout(line_strip, max_vertices = 3) out;\n";
    gshader->compileSourceCode(preamble+shaderCode);
    if (!gshader->log().isEmpty())
        qDebug() << gshader->log();
    else
        qDebug() << "done";

    line_program = new QOpenGLShaderProgram(this);
    line_program->addShader(vshader);
    line_program->addShader(fshader);
    line_program->addShader(tcshader);
    line_program->addShader(teshader);
    line_program->addShader(gshader);
    line_program->bindAttributeLocation("FragColor",     0);
    line_program->bindAttributeLocation("FragNormal",    1);
    line_program->bindAttributeLocation("FragGlowColor", 2);
    line_program->bindAttributeLocation("FragPosition",  3);
    GLCHK(line_program->link());

    GLCHK(line_program->bind());
    line_program->setUniformValue("texDiffuse"  ,         0);
    line_program->setUniformValue("texNormal"   ,         1);
    line_program->setUniformValue("texSpecular" ,         2);
    line_program->setUniformValue("texHeight"   ,         3);
    line_program->setUniformValue("texSSAO"     ,         4);
    line_program->setUniformValue("texRoughness",         5);
    line_program->setUniformValue("texMetallic",          6);
    line_program->setUniformValue("texMaterial",          7);
    line_program->setUniformValue("texPrefilteredEnvMap", 8);
    line_program->setUniformValue("texSourceEnvMap",      9);

    if(vshader  != NULL) delete vshader;
    if(fshader  != NULL) delete fshader;
    if(tcshader != NULL) delete tcshader;
    if(teshader != NULL) delete teshader;
    if(gshader  != NULL) delete gshader;

    // Loading sky box shader.
    qDebug() << "Loading skybox shader (vertex shader)";
    vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile(":/resources/shaders/skybox.vert.glsl");
    if (!vshader->log().isEmpty())
        qDebug() << vshader->log();
    else
        qDebug() << "done";

    qDebug() << "Loading skybox shader (fragment shader)";
    fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceFile(":/resources/shaders/skybox.frag.glsl");
    if (!fshader->log().isEmpty())
        qDebug() << fshader->log();
    else
        qDebug() << "done";

    skybox_program = new QOpenGLShaderProgram(this);
    skybox_program->addShader(vshader);
    skybox_program->addShader(fshader);
    skybox_program->bindAttributeLocation("FragColor",     0);
    skybox_program->bindAttributeLocation("FragNormal",    1);
    skybox_program->bindAttributeLocation("FragGlowColor", 2);
    skybox_program->bindAttributeLocation("FragPosition",  3);
    skybox_program->link();
    skybox_program->bind();
    skybox_program->setUniformValue("texEnv" , 0);

    // Loading enviromental shader.
    qDebug() << "Loading enviromental shader (vertex shader)";
    vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile(":/resources/shaders/env.vert");
    if (!vshader->log().isEmpty())
        qDebug() << vshader->log();
    else
        qDebug() << "done";

    qDebug() << "Loading enviromental shader (geometry shader)";
    gshader = new QOpenGLShader(QOpenGLShader::Geometry, this);
    gshader->compileSourceFile(":/resources/shaders/env.geom");
    if (!gshader->log().isEmpty())
        qDebug() << gshader->log();
    else
        qDebug() << "done";

    qDebug() << "Loading enviromental shader (fragment shader)";
    fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceFile(":/resources/shaders/env.frag");
    if (!fshader->log().isEmpty())
        qDebug() << fshader->log();
    else
        qDebug() << "done";

    env_program = new QOpenGLShaderProgram(this);
    env_program->addShader(vshader);
    env_program->addShader(gshader);
    env_program->addShader(fshader);

    env_program->link();
    env_program->bind();
    env_program->setUniformValue("texEnv" , 0);

    if(vshader != NULL) delete vshader;
    if(fshader != NULL) delete fshader;
    if(gshader != NULL) delete gshader;

    // Loading post processing filters
    qDebug() << "Loading post-processing shader (vertex shader)";
    vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceFile(":/resources/shaders/filters_3d.vert");
    if (!vshader->log().isEmpty())
        qDebug() << vshader->log();
    else
        qDebug() << "done";

    qDebug() << "Loading post-processing shaders (fragment shader)";
    QFile fFile(":/resources/shaders/filters_3d.frag");
    fFile.open(QFile::ReadOnly);
    QTextStream inf(&fFile);
    shaderCode = inf.readAll();

    QVector<QString> filters_list;
    filters_list.push_back("NORMAL_FILTER");
    filters_list.push_back("GAUSSIAN_BLUR_FILTER");
    filters_list.push_back("BLOOM_FILTER");
    filters_list.push_back("DOF_FILTER");
    filters_list.push_back("TONE_MAPPING_FILTER");
    filters_list.push_back("LENS_FLARES_FILTER");

    for(int filter = 0 ; filter < filters_list.size() ; filter++ )
    {
        qDebug() << "Compiling filter:" << filters_list[filter];
        QString preambule = "#version 330 core\n"
                            "#define "+filters_list[filter]+";\n" ;

        fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
        fshader->compileSourceCode(preambule + shaderCode);
        if (!fshader->log().isEmpty())
            qDebug() << fshader->log();

        filter_program = new QOpenGLShaderProgram(this);
        filter_program->addShader(vshader);
        filter_program->addShader(fshader);
        filter_program->bindAttributeLocation("positionIn", 0);
        filter_program->link();

        filter_program->bind();
        filter_program->setUniformValue("layerA", 0);
        filter_program->setUniformValue("layerB", 1);
        filter_program->setUniformValue("layerC", 2);
        filter_program->setUniformValue("layerD", 3);
        filter_program->setUniformValue("layerE", 4);
        filter_program->setUniformValue("layerF", 5);
        filter_program->setUniformValue("layerG", 6);

        post_processing_programs[filters_list[filter].toStdString()] = filter_program;
        filter_program->release();
        delete fshader;
    }
    if(vshader != NULL)
        delete vshader;

    lensFlareColorsTexture = new QOpenGLTexture(QImage(":/resources/textures/lenscolor.png"));
    lensDirtTexture = new QOpenGLTexture(QImage(":/resources/textures/lensdirt.png"));
    lensStarTexture = new QOpenGLTexture(QImage(":/resources/textures/lensstar.png"));

    camera.position.setZ( -0 );
    camera.toggleFreeCamera(false);
    newCamera.toggleFreeCamera(false);

    lightDirection.position  = QVector3D(0.0,0.0,0.0);
    // Set the light infront of the camera.
    lightDirection.direction = QVector3D(0.0,0.0,-1.0);
    lightDirection.toggleFreeCamera(false);
    lightDirection.radius = 1;

    mesh            = new Mesh(":/resources/meshes/Cube.obj");
    meshArray       = createVertexArray(mesh);
    skybox_mesh     = new Mesh(":/resources/meshes/sky_cube.obj");
    skyboxMeshArray = createVertexArray(skybox_mesh);
    env_mesh        = new Mesh(":/resources/meshes/sky_cube_env.obj");
    envMeshArray    = createVertexArray(env_mesh);
    quad_mesh       = new Mesh(":/resources/meshes/quad.obj");
    quadMeshArray   = createVertexArray(quad_mesh);

    chooseSkyBox("1SaintLazarusChurch", true);

    resizeFBOs();
    emit readyGL();
}

void OpenGL3DImageWidget::paintGL()
{
    // Generating an error because buffer does not exist.
    //GLCHK( glReadBuffer(GL_BACK) );

    // Drawing environment.
    bakeEnviromentalMaps();
    colorFBO->bindDefault();
    GLCHK( glViewport(0, 0, width(), height()) );

    if(cameraInterpolation < 1.0)
    {
        double w = cameraInterpolation;
        camera.position = camera.position*(1-w) + newCamera.position * w;
        cameraInterpolation += 0.01;
    }

    // Set the camera viewpoint.
    viewMatrix = camera.updateCamera();

    colorFBO->bind();

    GLCHK( glDisable(GL_CULL_FACE) );
    projectionMatrix.setToIdentity();
    projectionMatrix.perspective(zoom,ratio,0.1,350.0);

    // Set to which FBO result will be drawn.
    GLuint attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 , GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4,  attachments);
    GLCHK( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );
    GLCHK( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );

    // Drawing skybox
    skybox_program->bind();

    objectMatrix.setToIdentity();
    if(skybox_mesh->isLoaded())
    {
        objectMatrix.translate(camera.position);
        objectMatrix.scale(150.0);
    }
    modelViewMatrix = viewMatrix * objectMatrix;
    NormalMatrix    = modelViewMatrix.normalMatrix();

    // Disable depth.
    GLCHK( glDisable(GL_DEPTH_TEST) );

    skybox_program->setUniformValue("ModelViewMatrix", modelViewMatrix);
    skybox_program->setUniformValue("NormalMatrix", NormalMatrix);
    skybox_program->setUniformValue("ModelMatrix", objectMatrix);
    skybox_program->setUniformValue("ProjectionMatrix", projectionMatrix);
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    skyBoxTextureCube->bind();
    drawTriangles(skyboxMeshArray, skybox_mesh->getVertices().count());

    // Drawing model
    QOpenGLShaderProgram* program_ptrs[2] = {renderProgram,line_program};
    GLCHK( glEnable(GL_CULL_FACE) );
    GLCHK( glEnable(GL_DEPTH_TEST) );
    GLCHK( glCullFace(GL_BACK) );
    GLCHK( glDisable(GL_POLYGON_OFFSET_LINE) );
    GLCHK( glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) );

    for(int pindex = 0 ; pindex < 2 ; pindex ++)
    {
        QOpenGLShaderProgram* program_ptr = program_ptrs[pindex];
        GLCHK( program_ptr->bind() );

        program_ptr->setUniformValue("ProjectionMatrix", projectionMatrix);

        objectMatrix.setToIdentity();
        float fboRatio = float(openGL2DImageWidget->getTextureWidth(DIFFUSE_TEXTURE)) /
                               openGL2DImageWidget->getTextureHeight(DIFFUSE_TEXTURE);
        objectMatrix.scale(fboRatio,1,fboRatio);

        if(mesh->isLoaded())
        {

            objectMatrix.scale(0.5/mesh->getRadius());
            objectMatrix.translate(-mesh->getCentreOfMass());
        }
        modelViewMatrix = viewMatrix*objectMatrix;
        NormalMatrix = modelViewMatrix.normalMatrix();
        float mesh_scale = 0.5;// /mesh->radius;

        program_ptr->setUniformValue("ModelViewMatrix", modelViewMatrix);
        program_ptr->setUniformValue("NormalMatrix", NormalMatrix);
        program_ptr->setUniformValue("ModelMatrix", objectMatrix);
        program_ptr->setUniformValue("meshScale", mesh_scale);
        program_ptr->setUniformValue("lightPos",lightPosition);
        program_ptr->setUniformValue("lightDirection", lightDirection.direction);
        program_ptr->setUniformValue("cameraPos", camera.get_position());
        program_ptr->setUniformValue("gui_depthScale", display3Dparameters.depthScale);
        program_ptr->setUniformValue("gui_uvScale", display3Dparameters.uvScale);
        program_ptr->setUniformValue("gui_uvScaleOffset", display3Dparameters.uvOffset);
        program_ptr->setUniformValue("gui_bSpecular", bToggleSpecularView);
        program_ptr->setUniformValue("gui_bDiffuse", bToggleDiffuseView);
        program_ptr->setUniformValue("gui_bOcclusion", bToggleOcclusionView);
        program_ptr->setUniformValue("gui_bHeight", bToggleHeightView);
        program_ptr->setUniformValue("gui_bNormal", bToggleNormalView);
        program_ptr->setUniformValue("gui_bRoughness", bToggleRoughnessView);
        program_ptr->setUniformValue("gui_bMetallic", bToggleMetallicView);
        program_ptr->setUniformValue("gui_shading_type", display3Dparameters.shadingType);
        program_ptr->setUniformValue("gui_shading_model", display3Dparameters.shadingModel);
        program_ptr->setUniformValue("gui_SpecularIntensity", display3Dparameters.specularIntensity);
        program_ptr->setUniformValue("gui_DiffuseIntensity", display3Dparameters.diffuseIntensity);
        program_ptr->setUniformValue("gui_LightPower", display3Dparameters.lightPower);
        program_ptr->setUniformValue("gui_LightRadius", display3Dparameters.lightRadius);
        // Number of mipmaps.
        program_ptr->setUniformValue("num_mipmaps", skyBoxTextureCube->mipLevels());
        // 3D settings.
        program_ptr->setUniformValue("gui_bUseCullFace", display3Dparameters.bUseCullFace);
        program_ptr->setUniformValue("gui_bUseSimplePBR", display3Dparameters.bUseSimplePBR);
        program_ptr->setUniformValue("gui_noTessSub", display3Dparameters.noTessSubdivision);
        program_ptr->setUniformValue("gui_noPBRRays", display3Dparameters.noPBRRays);

        if(display3Dparameters.bShowTriangleEdges && pindex == 0)
        {
            GLCHK( glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) );
            GLCHK( glEnable(GL_POLYGON_OFFSET_FILL) );
            GLCHK( glPolygonOffset(1.0f, 1.0f) );
            program_ptr->setUniformValue("gui_bShowTriangleEdges", true);
            program_ptr->setUniformValue("gui_bMaterialsPreviewEnabled", true);
        }
        else
        {
            if(display3Dparameters.bShowTriangleEdges)
            {
                GLCHK( glDisable(GL_POLYGON_OFFSET_FILL) );
                GLCHK( glEnable(GL_POLYGON_OFFSET_LINE) );
                GLCHK( glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) );
                GLCHK( glPolygonOffset(-2.0f, -2.0f) );
                GLCHK( glLineWidth(1.0f) );
            }
            program_ptr->setUniformValue("gui_bShowTriangleEdges", display3Dparameters.bShowTriangleEdges);

            // Material preview: M key : when triangles are disabled
            if(!display3Dparameters.bShowTriangleEdges)
                program_ptr->setUniformValue("gui_bMaterialsPreviewEnabled", bool(keyPressed == KEY_SHOW_MATERIALS));
        }

        int tindex = 0;

        // Skip grunge texture (not used in 3D view).
        for(; tindex <= MATERIAL_TEXTURE; tindex++)
        {
            TextureType textureType = static_cast<TextureType>(tindex);
            GLCHK( glActiveTexture(GL_TEXTURE0 + tindex) );
            GLCHK( glBindTexture(GL_TEXTURE_2D, openGL2DImageWidget->getTextureId(textureType)) );
        }
        GLCHK( glActiveTexture(GL_TEXTURE0 + tindex) );

        tindex++;
        GLCHK( glActiveTexture(GL_TEXTURE0 + tindex) );
        skyBoxTextureCube->bind();
        drawTriangles(meshArray, mesh->getVertices().count(), true);
        // Set default active texture.
        GLCHK( glActiveTexture(GL_TEXTURE0) );

        if(!display3Dparameters.bShowTriangleEdges)
            break;
    } // End of loop over triangles.

    GLCHK( glPolygonMode(GL_FRONT_AND_BACK, GL_FILL) );
    GLCHK( glDisable(GL_POLYGON_OFFSET_LINE) );
    // Return to standard settings
    GLCHK( glDisable(GL_CULL_FACE) );
    GLCHK( glDisable(GL_DEPTH_TEST) );

    // Set to which FBO result will be drawn.
    GLuint attachments2[1] = { GL_COLOR_ATTACHMENT0 };
    GLCHK( glDrawBuffers(1, attachments2) );

    colorFBO->bindDefault();

    GLCHK( glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT) );

    // Do post processing if materials are not shown.
    if(keyPressed != KEY_SHOW_MATERIALS)
    {
        copyTexToFBO(colorFBO->texture(), outputFBO);

        // Post processing:
        // 1. Bloom effect (can be disabled/enabled by gui).
        if(display3Dparameters.bBloomEffect)
            applyGlowFilter(outputFBO);
        // 2. DOF (can be disabled/enabled by gui).
        if(display3Dparameters.bDofEffect)
            applyDofFilter(colorFBO->texture(),outputFBO);
        // 3. Lens Flares (can be disabled/enabled by gui).
        if(display3Dparameters.bLensFlares)
            applyLensFlaresFilter(colorFBO->texture(),outputFBO);

        applyToneFilter(colorFBO->texture(),outputFBO);
        applyNormalFilter(outputFBO->texture());
    }
    else
    {
        // Show materials.
        applyNormalFilter(colorFBO->texture());
    }

    filter_program->release();
}

void OpenGL3DImageWidget::resizeGL(int width, int height)
{
    ratio = float(width)/height;
    resizeFBOs();

    GLCHK( glViewport(0, 0, width, height) );
}

void OpenGL3DImageWidget::mousePressEvent(QMouseEvent *event)
{
    //OpenGLWidgetBase::mousePressEvent(event);
    lastCursorPos = event->pos();

    // Reset the mouse handling state with, to avoid a bad state
    blockMouseMovement = false;
    mouseUpdateIsQueued = false;

    setCursor(Qt::ClosedHandCursor);
    if (event->buttons() & Qt::RightButton)
    {
        setCursor(Qt::SizeAllCursor);
    }
    else if(event->buttons() & Qt::MiddleButton)
    {
        setCursor(lightCursor);
    }
    else if((event->buttons() & Qt::LeftButton) && (keyPressed == Qt::Key_Shift) )
    {
        colorFBO->bind();
        // NormalFBO contains World Space position.
        GLCHK( glReadBuffer(GL_COLOR_ATTACHMENT1) );
        vector< float > pixels( 1 * 1 * 4 );
        GLCHK( glReadPixels(event->pos().x(), height()-event->pos().y(), 1, 1,GL_RGBA, GL_FLOAT, &pixels[0]) );
        QVector3D position = QVector3D(pixels[0],pixels[1],pixels[2]);

        // When clicked on mesh other wise it has to be skybox.
        if(position.length() < 50.0)
        {
            qDebug() << "Picked position pixel (" << event->pos().x() << " , " << height()-event->pos().y() << ") with position:" << position;
            colorFBO->bindDefault();
            QVector3D curr_pos = camera.position - camera.radius * camera.direction;
            QVector3D new_dir = (position - curr_pos);
            new_dir.normalize();
            // Reset camera interpolation 'clock'.
            cameraInterpolation = 0 ;
            // Update new Camera position.
            newCamera.position  = position;
            newCamera.direction = new_dir;
            newCamera.radius    =-QVector3D::dotProduct(curr_pos - position,new_dir);
            GLCHK( glReadBuffer(GL_BACK) );
            keyPressed = (Qt::Key)0;
            newCamera.side_direction = QVector3D(newCamera.direction.z(),0,-newCamera.direction.x());
            newCamera.rotateView(0,0);
        }
        emit changeCamPositionApplied(false);
    }

    update();

    // Capture the pixel color if material preview is enabled.
    if((event->buttons() & Qt::LeftButton) && keyPressed == KEY_SHOW_MATERIALS)
    {
        vector< unsigned char > pixels( 1 * 1 * 4 );

        GLCHK( glReadPixels(event->pos().x(), height()-event->pos().y(), 1, 1,GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]) );
        QColor color = QColor(pixels[0],pixels[1],pixels[2],pixels[3]);

        qDebug() << "Picked material pixel (" << event->pos().x() << " , " << height()-event->pos().y() << ") with color:" << color;
        emit materialColorPicked(color);
    }
}

void OpenGL3DImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    setCursor(Qt::PointingHandCursor);
    event->accept();
}

void OpenGL3DImageWidget::relativeMouseMoveEvent(int dx, int dy, bool* wrapMouse, Qt::MouseButtons buttons)
{
    if ((buttons & Qt::LeftButton) && (buttons & Qt::RightButton))
    {}
    else if (buttons & Qt::LeftButton)
    {
        camera.rotateView(dx/1.0,dy/1.0);
    }
    else if (buttons & Qt::RightButton)
    {
        camera.position +=QVector3D(dx/500.0,dy/500.0,0)*camera.radius;
    }
    else if (buttons & Qt::MiddleButton)
    {
        lightPosition += QVector4D(0.05*dx,-0.05*dy,-0,0);
        if(lightPosition.x() > +10.0)
            lightPosition.setX(+10.0);
        if(lightPosition.x() < -10.0)
            lightPosition.setX(-10.0);
        if(lightPosition.y() > +10.0)
            lightPosition.setY(+10.0);
        if(lightPosition.y() < -10.0)
            lightPosition.setY(-10.0);
        lightDirection.rotateView(-2*dx/1.0,2*dy/1.0);
    }
    else
    {
        *wrapMouse = false;
    }
}

void OpenGL3DImageWidget::wheelEvent(QWheelEvent *event)
{
    int numDegrees = -event->delta();
    camera.mouseWheelMove((numDegrees));

    update();
}

void OpenGL3DImageWidget::dropEvent(QDropEvent *event)
{

    QList<QUrl> droppedUrls = event->mimeData()->urls();
    int i = 0;
    QString localPath = droppedUrls[i].toLocalFile();
    QFileInfo fileInfo(localPath);

    loadMeshFile(fileInfo.absoluteFilePath());

    event->acceptProposedAction();
}

void OpenGL3DImageWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasText())
    {
        event->acceptProposedAction();
    }
}

void OpenGL3DImageWidget::resizeFBOs()
{
    QOpenGLFramebufferObjectFormat format;
    format.setInternalTextureFormat(GL_RGB16F);
    format.setTextureTarget(GL_TEXTURE_2D);
    format.setMipmap(true);
    format.setAttachment(QOpenGLFramebufferObject::Depth);

    if(colorFBO != NULL)
        delete colorFBO;
    colorFBO = new QOpenGLFramebufferObject(width(), height(), format);
    setFBOTextureParameters(colorFBO);
    addTexture(GL_COLOR_ATTACHMENT1);
    addTexture(GL_COLOR_ATTACHMENT2);
    addTexture(GL_COLOR_ATTACHMENT3);

    if(outputFBO != NULL)
        delete outputFBO;
    outputFBO = new QOpenGLFramebufferObject(width(), height(), format);
    setFBOTextureParameters(outputFBO);

    if(auxFBO != NULL)
        delete auxFBO;
    auxFBO = new QOpenGLFramebufferObject(width(), height(), format);
    setFBOTextureParameters(auxFBO);

    // Initialize/resize glow FBOS.
    for(int i = 0; i < 4 ; i++)
    {
        if(glowInputColor[i]  != NULL)
            delete glowInputColor[i];
        if(glowOutputColor[i] != NULL)
            delete glowOutputColor[i];
        glowInputColor[i] =  new QOpenGLFramebufferObject(width() / pow(2.0, i + 1), height() / pow(2.0, i + 1), format);
        setFBOTextureParameters(glowInputColor[i]);
        glowOutputColor[i] = new QOpenGLFramebufferObject(width() / pow(2.0, i + 1), height() / pow(2.0, i + 1), format);
        setFBOTextureParameters(glowOutputColor[i]);
    }

    // Initialize/resize tone mapping FBOs.
    for(int i = 0; i < 10 ; i++)
    {
        if(toneMipmaps[i] != NULL)
            delete toneMipmaps[i];
        toneMipmaps[i] = new QOpenGLFramebufferObject(qMax(width() / pow(2.0, i + 1), 1.0), qMax(height() / pow(2.0, i + 1), 1.0));
        setFBOTextureParameters(toneMipmaps[i]);
    }
}

void OpenGL3DImageWidget::deleteFBOs()
{
    colorFBO->bindDefault();
    delete colorFBO;
    delete outputFBO;
    delete auxFBO;
    for(int i = 0; i < 4 ; i++)
    {
        if(glowInputColor[i]  != NULL)
            delete glowInputColor[i];
        if(glowOutputColor[i] != NULL)
            delete glowOutputColor[i];
    }
    for(int i = 0; i < 10 ; i++)
    {
        if(toneMipmaps[i] != NULL)
            delete toneMipmaps[i];
    }
}

void OpenGL3DImageWidget::applyNormalFilter(GLuint input_tex)
{
    filter_program = post_processing_programs["NORMAL_FILTER"];
    filter_program->bind();
    GLCHK( glViewport(0,0,width(),height()) );
    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos"  , QVector2D(0.0,0.0));
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, input_tex) );
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
}

void OpenGL3DImageWidget::copyTexToFBO(GLuint input_tex,QOpenGLFramebufferObject* dst)
{
    filter_program = post_processing_programs["NORMAL_FILTER"];
    filter_program->bind();
    dst->bind();
    GLCHK( glViewport(0,0,dst->width(),dst->height()) );
    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos",   QVector2D(0.0,0.0));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, input_tex);
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    dst->bindDefault();
}

void OpenGL3DImageWidget::applyGaussFilter(
        GLuint input_tex,
        QOpenGLFramebufferObject* auxFBO,
        QOpenGLFramebufferObject* outputFBO, float radius)
{
    filter_program = post_processing_programs["GAUSSIAN_BLUR_FILTER"];
    filter_program->bind();

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    filter_program->setUniformValue("quad_scale",       QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos",         QVector2D(0.0,0.0));
    filter_program->setUniformValue("gui_gauss_w",      (float)radius);
    filter_program->setUniformValue("gui_gauss_radius", (float)radius);
    filter_program->setUniformValue("gui_gauss_mode",   0);
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, input_tex) );

    auxFBO->bind();
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    auxFBO->bindDefault();

    outputFBO->bind();
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    filter_program->setUniformValue("gui_gauss_mode", 1);
    GLCHK( glBindTexture(GL_TEXTURE_2D, auxFBO->texture()) );
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    outputFBO->bindDefault();
}

void OpenGL3DImageWidget::applyDofFilter(
        GLuint input_tex,
        QOpenGLFramebufferObject* outputFBO)
{
    // Skip processing if effect is disabled.
    if(!settingsDialog->filters3DProperties->DOF.EnableEffect) return;

    filter_program = post_processing_programs["DOF_FILTER"];
    filter_program->bind();

    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos", QVector2D(0.0,0.0));
    filter_program->setUniformValue("dof_FocalLenght", (float)settingsDialog->filters3DProperties->DOF.FocalLength);
    filter_program->setUniformValue("dof_FocalDepht", settingsDialog->filters3DProperties->DOF.FocalDepth);
    filter_program->setUniformValue("dof_FocalStop", settingsDialog->filters3DProperties->DOF.FocalStop);
    filter_program->setUniformValue("dof_NoSamples", settingsDialog->filters3DProperties->DOF.NoSamples);
    filter_program->setUniformValue("dof_NoRings", settingsDialog->filters3DProperties->DOF.NoRings);
    filter_program->setUniformValue("dof_bNoise", settingsDialog->filters3DProperties->DOF.Noise);
    filter_program->setUniformValue("dof_Coc", settingsDialog->filters3DProperties->DOF.Coc);
    filter_program->setUniformValue("dof_Threshold", settingsDialog->filters3DProperties->DOF.Threshold);
    filter_program->setUniformValue("dof_Gain", settingsDialog->filters3DProperties->DOF.Gain);
    filter_program->setUniformValue("dof_BokehBias", settingsDialog->filters3DProperties->DOF.BokehBias);
    filter_program->setUniformValue("dof_BokehFringe", settingsDialog->filters3DProperties->DOF.BokehFringe);
    filter_program->setUniformValue("dof_DitherAmount", (float)settingsDialog->filters3DProperties->DOF.DitherAmount);

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, input_tex) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, getAttachedTexture(2)) );

    outputFBO->bind();
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    outputFBO->bindDefault();

    copyTexToFBO(outputFBO->texture(),colorFBO);
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

void OpenGL3DImageWidget::applyGlowFilter(QOpenGLFramebufferObject* outputFBO)
{
    // Skip processing if effect is disabled.
    if(!settingsDialog->filters3DProperties->Bloom.EnableEffect) return;

    applyGaussFilter(getAttachedTexture(1),glowInputColor[0],glowOutputColor[0]);
    applyGaussFilter(glowOutputColor[0]->texture(),glowInputColor[0],glowOutputColor[0]);
    applyGaussFilter(glowOutputColor[0]->texture(),glowInputColor[1],glowOutputColor[1]);
    applyGaussFilter(glowOutputColor[1]->texture(),glowInputColor[1],glowOutputColor[1]);
    applyGaussFilter(glowOutputColor[1]->texture(),glowInputColor[2],glowOutputColor[2]);
    applyGaussFilter(glowOutputColor[2]->texture(),glowInputColor[2],glowOutputColor[2]);
    applyGaussFilter(glowOutputColor[2]->texture(),glowInputColor[3],glowOutputColor[3]);

    filter_program = post_processing_programs["BLOOM_FILTER"];
    filter_program->bind();

    outputFBO->bind();
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );

    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos", QVector2D(0.0,0.0));
    filter_program->setUniformValue("bloom_WeightA", settingsDialog->filters3DProperties->Bloom.WeightA);
    filter_program->setUniformValue("bloom_WeightB", settingsDialog->filters3DProperties->Bloom.WeightB);
    filter_program->setUniformValue("bloom_WeightB", settingsDialog->filters3DProperties->Bloom.WeightC);
    filter_program->setUniformValue("bloom_WeightC", settingsDialog->filters3DProperties->Bloom.WeightD);
    filter_program->setUniformValue("bloom_Vignette", settingsDialog->filters3DProperties->Bloom.Vignette);
    filter_program->setUniformValue("bloom_VignetteAtt", settingsDialog->filters3DProperties->Bloom.VignetteAtt);

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, colorFBO->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[0]->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE2) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[1]->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE3) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[2]->texture()) );
    GLCHK( glActiveTexture(GL_TEXTURE4) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[3]->texture()) );
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    outputFBO->bindDefault();
    // Copy obtained result to main FBO.
    copyTexToFBO(outputFBO->texture(),colorFBO);
}

void OpenGL3DImageWidget::applyToneFilter(GLuint input_tex,QOpenGLFramebufferObject* outputFBO)
{
    // Skip processing if effect is disabled.
    if(!settingsDialog->filters3DProperties->ToneMapping.EnableEffect) return;

    filter_program = post_processing_programs["TONE_MAPPING_FILTER"];
    filter_program->bind();

    // Calculate luminance of each pixel.
    outputFBO->bind();
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );

    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos", QVector2D(0.0,0.0));
    filter_program->setUniformValue("tm_step", 1);
    filter_program->setUniformValue("tm_Delta", settingsDialog->filters3DProperties->ToneMapping.Delta);
    filter_program->setUniformValue("tm_Scale", settingsDialog->filters3DProperties->ToneMapping.Scale);
    filter_program->setUniformValue("tm_LumMaxWhite", settingsDialog->filters3DProperties->ToneMapping.LumMaxWhite);
    filter_program->setUniformValue("tm_GammaCorrection", settingsDialog->filters3DProperties->ToneMapping.GammaCorrection);
    filter_program->setUniformValue("tm_BlendingWeight", settingsDialog->filters3DProperties->ToneMapping.BlendingWeight);

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, input_tex) );

    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    outputFBO->bindDefault();

    // Caclulate averaged luminance of image.
    GLuint averagedTexID = outputFBO->texture();
    filter_program->setUniformValue("tm_step", 2);
    for(int i = 0 ; i < 10 ; i++)
    {
        toneMipmaps[i]->bind();
        GLCHK( glViewport(0,0,toneMipmaps[i]->width(),toneMipmaps[i]->height()) );
        GLCHK( glActiveTexture(GL_TEXTURE0) );
        GLCHK( glBindTexture(GL_TEXTURE_2D, averagedTexID) );

        drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
        averagedTexID = toneMipmaps[i]->texture();
    }

    outputFBO->bind();
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos", QVector2D(0.0,0.0));
    filter_program->setUniformValue("tm_step", 3);

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, averagedTexID) );

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, input_tex) );

    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    outputFBO->bindDefault();
    GLCHK( glActiveTexture(GL_TEXTURE0) );

    // Copy result to Color FBO.
    copyTexToFBO(outputFBO->texture(),colorFBO);
}

void OpenGL3DImageWidget::applyLensFlaresFilter(GLuint input_tex,QOpenGLFramebufferObject* outputFBO)
{
    // Skip processing if effect is disabled.
    if(!settingsDialog->filters3DProperties->Flares.EnableEffect) return;

    // Based on: http://john-chapman-graphics.blogspot.com/2013/02/pseudo-lens-flare.html
    // Prepare mask image
    if(!display3Dparameters.bBloomEffect || !settingsDialog->filters3DProperties->Bloom.EnableEffect)
    {
        applyGaussFilter(getAttachedTexture(1),glowInputColor[0],glowOutputColor[0]);
        applyGaussFilter(glowOutputColor[0]->texture(),glowInputColor[0],glowOutputColor[0]);
    }

    filter_program = post_processing_programs["LENS_FLARES_FILTER"];
    filter_program->bind();

    // Prepare threshold image.
    filter_program->setUniformValue("lf_NoSamples", settingsDialog->filters3DProperties->Flares.NoSamples);
    filter_program->setUniformValue("lf_Dispersal", settingsDialog->filters3DProperties->Flares.Dispersal);
    filter_program->setUniformValue("lf_HaloWidth", settingsDialog->filters3DProperties->Flares.HaloWidth);
    filter_program->setUniformValue("lf_Distortion", settingsDialog->filters3DProperties->Flares.Distortion);
    filter_program->setUniformValue("lf_weightLF", settingsDialog->filters3DProperties->Flares.weightLF);

    glowInputColor[0]->bind();
    GLCHK( glViewport(0,0,glowInputColor[0]->width(), glowInputColor[0]->height()) );
    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos", QVector2D(0.0,0.0));
    // Threshold step
    filter_program->setUniformValue("lf_step", int(0));

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[0]->texture()) );
    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());

    // Create ghosts and halos.

    glowOutputColor[0]->bind();
    GLCHK( glViewport(0,0,glowOutputColor[0]->width(),glowOutputColor[0]->height()) );

    //II step
    filter_program->setUniformValue("lf_step", int(1));

    GLCHK( glActiveTexture(GL_TEXTURE0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, input_tex) );
    GLCHK( glActiveTexture(GL_TEXTURE2) );
    lensFlareColorsTexture->bind();

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowInputColor[0]->texture()) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT) );

    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT) );
    outputFBO->bindDefault();

    float camrot = QVector3D::dotProduct(camera.side_direction,QVector3D(1,0,0)) +
            QVector3D::dotProduct(camera.updown_direction,QVector3D(0,1,0));

    QMatrix4x4 scaleBias1 = QMatrix4x4(
                2.0f,   0.0f,  -1.0f, 0.0f,
                0.0f,   2.0f,  -1.0f, 0.0f,
                0.0f,   0.0f,   1.0f, 0.0f,
                0.0f,   0.0f,   0.0f, 1.0f
                );
    QMatrix4x4 rotation = QMatrix4x4(
                cos(camrot), -sin(camrot), 0.0f, 0.0f,
                sin(camrot), cos(camrot),  0.0f, 0.0f,
                0.0f,        0.0f,         1.0f, 0.0f,
                0.0f,   0.0f,   0.0f, 1.0f
                );
    QMatrix4x4 scaleBias2 = QMatrix4x4(
                0.5f,   0.0f,   0.5f, 0.0f,
                0.0f,   0.5f,   0.5f, 0.0f,
                0.0f,   0.0f,   1.0f, 0.0f,
                0.0f,   0.0f,   0.0f, 1.0f
                );

    QMatrix4x4 uLensStarMatrix = scaleBias2 * rotation * scaleBias1;

    // Blend images (orginal, halos, dirt texture, and star texture)
    outputFBO->bind();
    GLCHK( glViewport(0,0,outputFBO->width(),outputFBO->height()) );
    filter_program->setUniformValue("quad_scale", QVector2D(1.0,1.0));
    filter_program->setUniformValue("quad_pos", QVector2D(0.0,0.0));
    filter_program->setUniformValue("lf_step", int(2));
    filter_program->setUniformValue("lf_starMatrix", uLensStarMatrix);

    GLCHK( glActiveTexture(GL_TEXTURE1) );
    // Ghost texture.
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[0]->texture()) );
    // Dirt texture.
    GLCHK( glActiveTexture(GL_TEXTURE2) );
    lensDirtTexture->bind();
    // Star texture.
    GLCHK( glActiveTexture(GL_TEXTURE3) );
    lensStarTexture->bind();
    // Exposure reference
    GLCHK( glActiveTexture(GL_TEXTURE4) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, glowOutputColor[3]->texture()) );

    drawTriangles(quadMeshArray, quad_mesh->getVertices().count());
    copyTexToFBO(outputFBO->texture(),colorFBO);
    GLCHK( glActiveTexture(GL_TEXTURE0) );
}

QPointF OpenGL3DImageWidget::pixelPosToViewPos(const QPointF& p)
{
    return QPointF(2.0 * float(p.x()) / width() - 1.0,
                   1.0 - 2.0 * float(p.y()) / height());
}

int OpenGL3DImageWidget::glhUnProjectf(float& winx, float& winy, float& winz,
                            QMatrix4x4& modelview, QMatrix4x4& projection,
                            QVector4D& objectCoordinate)
{
    //Transformation matrices
    QVector4D in,out;

    // Compute projection x modelview.
    QMatrix4x4  A = projection * modelview;
    // Compute the inverse of matrix A
    QMatrix4x4  m = A.inverted();

    // Transform normalized coordinates between -1 and 1
    in[0]=(winx)/(float)width()*2.0-1.0;
    in[1]=(1-(winy)/(float)height())*2.0-1.0;
    in[2]=2.0*winz-1.0;
    in[3]=1.0;
    // Object's coordinates
    out = m * in;

    if(out[3]==0.0)
        return 0;
    out[3]=1.0/out[3];
    objectCoordinate[0]=out[0]*out[3];
    objectCoordinate[1]=out[1]*out[3];
    objectCoordinate[2]=out[2]*out[3];
    return 1;
}

void OpenGL3DImageWidget::bakeEnviromentalMaps()
{
    if(bDiffuseMapBaked) return;
    bDiffuseMapBaked = true;

    // Drawing env - one pass method
    env_program->bind();
    //m_prefiltered_env_map->bindFBO();
    GLCHK( glViewport(0, 0, 512, 512) );

    objectMatrix.setToIdentity();
    objectMatrix.scale(1.0);
    objectMatrix.rotate(90.0,1,0,0);

    modelViewMatrix = viewMatrix * objectMatrix;
    NormalMatrix    = modelViewMatrix.normalMatrix();

    env_program->setUniformValue("ModelViewMatrix",  modelViewMatrix);
    env_program->setUniformValue("NormalMatrix",     NormalMatrix);
    env_program->setUniformValue("ModelMatrix",      objectMatrix);
    env_program->setUniformValue("ProjectionMatrix", projectionMatrix);
    GLCHK( glActiveTexture(GL_TEXTURE0) );
    skyBoxTextureCube->bind();
    drawTriangles(envMeshArray, env_mesh->getVertices().count());

    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
    GLCHK( glViewport(0, 0, width(), height()) );
}

bool OpenGL3DImageWidget::addTexture(GLenum COLOR_ATTACHMENTn)
{
    GLuint tex[1];
    GLCHK( glGenTextures(1, &tex[0]) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, tex[0]) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT) );

    GLCHK( glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width(), height(), 0,GL_RGB, GL_UNSIGNED_BYTE, 0) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, 0) );
    if(!glIsTexture(tex[0]))
    {
        qDebug() << "Error: Cannot create additional texture. Process stopped." << endl;
        return false;
    }
    colorFBO->bind();

    GLCHK( glFramebufferTexture2D(GL_FRAMEBUFFER, COLOR_ATTACHMENTn,GL_TEXTURE_2D, tex[0], 0) );

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE){
        qDebug() << "Cannot add new texture to current FBO! FBO is incomplete.";
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // Switch back to window-system-provided framebuffer.
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );

    attachments.push_back(tex[0]);
    return true;
}

const GLuint& OpenGL3DImageWidget::getAttachedTexture(GLuint index)
{
    return attachments[index];
}

void OpenGL3DImageWidget::setFBOTextureParameters(QOpenGLFramebufferObject *fbo)
{
    GLCHK( glBindTexture(GL_TEXTURE_2D, fbo->texture()) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT) );
    GLCHK( glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR) );
    GLCHK( glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR) );
    GLCHK( glBindTexture(GL_TEXTURE_2D, 0) );
    GLCHK( glBindFramebuffer(GL_FRAMEBUFFER, 0) );
}

QOpenGLVertexArrayObject* OpenGL3DImageWidget::createVertexArray(Mesh* mesh)
{
    QOpenGLVertexArrayObject* vertexArray = new QOpenGLVertexArrayObject();
    vertexArray->create();
    vertexArray->bind();

    QOpenGLBuffer vertexBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vertexBuffer.allocate(mesh->getVertices().constData(), sizeof(QVector3D) *
                          mesh->getVertices().count());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(0);

    QOpenGLBuffer textureCoordBuffer(QOpenGLBuffer::VertexBuffer);
    textureCoordBuffer.create();
    textureCoordBuffer.bind();
    textureCoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    textureCoordBuffer.allocate(mesh->getTextureCoordinates().constData(), sizeof(QVector3D) *
                                mesh->getTextureCoordinates().count());
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(1);

    QOpenGLBuffer normalsBuffer(QOpenGLBuffer::VertexBuffer);
    normalsBuffer.create();
    normalsBuffer.bind();
    normalsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    normalsBuffer.allocate(mesh->getNormals().constData(), sizeof(QVector3D) *
                           mesh->getNormals().count());
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(2);

    QOpenGLBuffer tangetsBuffer(QOpenGLBuffer::VertexBuffer);
    tangetsBuffer.create();
    tangetsBuffer.bind();
    tangetsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    tangetsBuffer.allocate(mesh->getTangents().constData(), sizeof(QVector3D) *
                           mesh->getTangents().count());
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(3);

    QOpenGLBuffer bitangentsBuffer(QOpenGLBuffer::VertexBuffer);
    bitangentsBuffer.create();
    bitangentsBuffer.bind();
    bitangentsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    bitangentsBuffer.allocate(mesh->getBitangents().constData(), sizeof(QVector3D) *
                              mesh->getBitangents().count());
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(4);

    QOpenGLBuffer smoothedNormalsBuffer(QOpenGLBuffer::VertexBuffer);
    smoothedNormalsBuffer.create();
    smoothedNormalsBuffer.bind();
    smoothedNormalsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    smoothedNormalsBuffer.allocate(mesh->getSmoothedNormals().constData(), sizeof(QVector3D) *
                                   mesh->getSmoothedNormals().count());
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(5);

    vertexArray->release();
    return vertexArray;
}

void OpenGL3DImageWidget::drawTriangles(QOpenGLVertexArrayObject* vertexArray, unsigned int vertexCount, bool usePatches)
{
    vertexArray->bind();
    if (usePatches)
    {
        GLCHK( glPatchParameteri(GL_PATCH_VERTICES, 3) );
        GLCHK( glDrawArrays(GL_PATCHES, 0, vertexCount) );
    }
    else
    {
        GLCHK( glDrawArrays(GL_TRIANGLES, 0, vertexCount) );
    }
    vertexArray->release();
}

QOpenGLTexture* OpenGL3DImageWidget::createTextureCube(const QStringList& fileNames)
{
    QOpenGLTexture* texture = new QOpenGLTexture(QOpenGLTexture::TargetCubeMap);

    QImage image[6];
    int images = 0;
    int size = 0;
    for (int i = 0; i < 6; i++)
    {
        ++images;
        image[i] = QImage(fileNames[i]);
        if (size <= 0)
            size = image[i].width();
        if (image[i].width() != size || image[i].height() != size)
            image[i] = image[i].scaled(size, size, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    texture->setSize(size, size);
    texture->setFormat(QOpenGLTexture::RGB32F);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    texture->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
    texture->setMipLevels(floor(log2(size)));
    texture->allocateStorage();

    texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveX, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[0].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeX, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[1].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveY, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[2].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeY, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[3].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapPositiveZ, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[4].bits());
    texture->setData(0, 0, QOpenGLTexture::CubeMapNegativeZ, QOpenGLTexture::BGRA, QOpenGLTexture::UInt8, image[5].bits());

    return texture;
}
