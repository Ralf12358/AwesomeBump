/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>
#include <QtOpenGL>
#include <qmath.h>

#include "display3dsettings.h"
#include "camera.h"
#include "utils/Mesh.hpp"
#include "utils/qglbuffers.h"
#include "glwidgetbase.h"
#include "glimageeditor.h"
#include "properties/Dialog3DGeneralSettings.h"
#include "utils/glslshaderparser.h"

#define settings3D      Dialog3DGeneralSettings::settings3D
#define currentShader   Dialog3DGeneralSettings::currentRenderShader
#define glslShadersList Dialog3DGeneralSettings::glslParsedShaders

#ifdef USE_OPENGL_330
#include <QOpenGLFunctions_3_3_Core>
#define OPENGL_FUNCTIONS QOpenGLFunctions_3_3_Core
#else
#include <QOpenGLFunctions_4_0_Core>
#define OPENGL_FUNCTIONS QOpenGLFunctions_4_0_Core
#endif

class GLWidget : public GLWidgetBase , protected OPENGL_FUNCTIONS
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent = 0 , QGLWidget * shareWidget  = 0);
    ~GLWidget();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;
    void setPointerToTexture(QGLFramebufferObject **pointer, TextureTypes type);

public slots:
    void toggleDiffuseView(bool);
    void toggleSpecularView(bool);
    void toggleOcclusionView(bool);
    void toggleNormalView(bool);
    void toggleHeightView(bool);
    void toggleRoughnessView(bool);
    void toggleMetallicView(bool);
    void setCameraMouseSensitivity(int value);
    void resetCameraPosition();
    void cleanup();

    // Mesh functions.
    void loadMeshFromFile();//opens file dialog
    bool loadMeshFile(const QString &fileName,bool bAddExtension = false);
    void chooseMeshFile(const QString &fileName);

    // PBR functions.
    void chooseSkyBox(QString cubeMapName, bool bFirstTime = false);
    void updatePerformanceSettings(Display3DSettings settings);
    // Read and compile custom fragment shader again, can be called from 3D settings GUI.
    void recompileRenderShader();

signals:
    void renderGL();
    void readyGL();
    // Emit material index color.
    void materialColorPicked(QColor);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void relativeMouseMoveEvent(int dx, int dy, bool *wrapMouse, Qt::MouseButtons buttons);
    void wheelEvent(QWheelEvent *event);
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);

    // Post processing tools.
    void resizeFBOs();
    void deleteFBOs();
    void applyNormalFilter(GLuint input_tex);
    void copyTexToFBO(GLuint input_tex,QGLFramebufferObject* dst);
    void applyGaussFilter(GLuint input_tex,
                          QGLFramebufferObject* auxFBO,
                          QGLFramebufferObject* outputFBO, float radius = 10.0);
    void applyDofFilter(GLuint input_tex,
                        QGLFramebufferObject* outputFBO);
    void applyGlowFilter(QGLFramebufferObject* outputFBO);
    void applyToneFilter(GLuint input_tex,QGLFramebufferObject* outputFBO);
    void applyLensFlaresFilter(GLuint input_tex,QGLFramebufferObject* outputFBO);

private:
    QPointF pixelPosToViewPos(const QPointF& p);
    int glhUnProjectf(float &winx, float &winy, float &winz,
                      QMatrix4x4 &modelview, QMatrix4x4 &projection,
                      QVector4D& objectCoordinate);
    // Calculate prefiltered enviromental map.
    void bakeEnviromentalMaps();
    // Same as "program" but instead of triangles lines are used
    QOpenGLShaderProgram *line_program;
    QOpenGLShaderProgram *skybox_program;
    QOpenGLShaderProgram *env_program;

    QGLFramebufferObject**  fboIdPtrs[8];

    bool bToggleDiffuseView;
    bool bToggleSpecularView;
    bool bToggleOcclusionView;
    bool bToggleNormalView;
    bool bToggleHeightView;
    bool bToggleRoughnessView;
    bool bToggleMetallicView;

    Display3DSettings display3Dparameters;

    // 3D view parameters.
    QMatrix4x4 projectionMatrix;
    QMatrix4x4 modelViewMatrix;
    QMatrix3x3 NormalMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 objectMatrix;

    QVector4D lightPosition;
    QVector4D cursorPositionOnPlane;

    float ratio;
    float zoom;
    // Light used for standard phong shading.
    AwesomeCamera camera;
    // To make smooth linear interpolation between two views.
    AwesomeCamera newCamera;

    double cameraInterpolation;
    //Second light - use camera class to rotate light
    AwesomeCamera lightDirection;
    QCursor lightCursor;

    // Displayed 3D mesh.
    Mesh* mesh;
    // Sky box cube.
    Mesh* skybox_mesh;
    // One trinagle used for calculation of prefiltered environment map.
    Mesh* env_mesh;

    // Orginal cube map.
    GLTextureCube* m_env_map;
    // Filtered lambertian cube map.
    GLTextureCube* m_prefiltered_env_map;
    // Control calculating diffuse environment map.
    bool bDiffuseMapBaked;

    GLImage* glImagePtr;

    // Post-processing variables.
    // All post processing functions
    std::map<std::string,QOpenGLShaderProgram*> post_processing_programs;
    // Quad mesh used for post processing
    Mesh* quad_mesh;
    // Holds pointer to current post-processing program
    QOpenGLShaderProgram *filter_program;
    GLFrameBufferObject* colorFBO;
    GLFrameBufferObject* outputFBO;
    GLFrameBufferObject* auxFBO;
    // Glow FBOs.
    GLFrameBufferObject* glowInputColor[4];
    GLFrameBufferObject* glowOutputColor[4];
    // Tone mapping mipmaps FBOS.
    GLFrameBufferObject* toneMipmaps[10];

    GLuint lensFlareColorsTexture;
    GLuint lensDirtTexture;
    GLuint lensStarTexture;

public:
    static QDir* recentMeshDir;
};

#endif
