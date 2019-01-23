#ifndef OPENGLIMAGEEDITOR_H
#define OPENGLIMAGEEDITOR_H

#include <QOpenGLFunctions_4_5_Core>
#include "openglwidgetbase.h"

#include "openglframebufferobjectproperties.h"

#include <QtOpenGL>
#include <math.h>
#include <map>

#define BasicProp activeImage->properties->Basic
#define RemoveShadingProp activeImage->properties->RemoveShading
#define ColorLevelsProp activeImage->properties->ColorLevels
#define SurfaceDetailsProp activeImage->properties->SurfaceDetails
#define AOProp activeImage->properties->AO
#define GrungeProp targetImageGrunge->properties->Grunge
#define GrungeOnImageProp activeImage->properties->GrungeOnImage
#define NormalMixerProp activeImage->properties->NormalsMixer
#define BaseMapToOthersProp activeImage->properties->BaseMapToOthers
#define RMFilterProp activeImage->properties->RMFilter

enum ConversionType
{
    CONVERT_NONE = 0,
    CONVERT_FROM_H_TO_N,
    CONVERT_FROM_N_TO_H,
    CONVERT_FROM_D_TO_O,    // Diffuse to others.
    CONVERT_FROM_HN_TO_OC,
    CONVERT_RESIZE
};

enum UVManipulationMethods
{
    UV_TRANSLATE = 0,
    UV_GRAB_CORNERS,
    UV_SCALE_XY
};

class OpenGLImageEditor : public OpenGLWidgetBase, protected QOpenGLFunctions_4_5_Core
{
    Q_OBJECT

public:
    OpenGLImageEditor(QWidget *parent = 0);
    ~OpenGLImageEditor();

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    void cleanup();
    void setActiveImage(OpenGLFramebufferObjectProperties* ptr);
    OpenGLFramebufferObjectProperties* getActiveImage(){return activeImage;}
    void enableShadowRender(bool enable);
    void setConversionType(ConversionType conversionType);
    ConversionType getConversionType();
    void updateCornersPosition(QVector2D dc1, QVector2D dc2, QVector2D dc3, QVector2D dc4);
    void render();

    OpenGLFramebufferObjectProperties* targetImageDiffuse;
    OpenGLFramebufferObjectProperties* targetImageNormal;
    OpenGLFramebufferObjectProperties* targetImageHeight;
    OpenGLFramebufferObjectProperties* targetImageSpecular;
    OpenGLFramebufferObjectProperties* targetImageOcclusion;
    OpenGLFramebufferObjectProperties* targetImageRoughness;
    OpenGLFramebufferObjectProperties* targetImageMetallic;
    OpenGLFramebufferObjectProperties* targetImageGrunge;
    OpenGLFramebufferObjectProperties* targetImageMaterial;

signals:
    void rendered();
    void readyGL();
    void colorPicked(QVector4D color);

public slots:
    void resizeFBO(int width, int height);
    void imageChanged();
    void resetView();
    void selectPerspectiveTransformMethod(int method);
    void selectUVManipulationMethod(UVManipulationMethods method);
    void updateCornersWeights(float w1, float w2, float w3, float w4);
    void selectSeamlessMode(SeamlessMode mode);
    void toggleColorPicking(bool toggle);
    void pickImageColor(QtnPropertyABColor *property);
    // When image is rendered copy it to paintFBO in order to display result.
    void copyRenderToPaintFBO();

protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void relativeMouseMoveEvent(int dx, int dy, bool *wrapMouse, Qt::MouseButtons buttons);
    void wheelEvent(QWheelEvent *event);
    void updateMousePosition();
    void showEvent(QShowEvent* event);

    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

public:
    void applyNormalFilter(QOpenGLFramebufferObject *inputFBO,
                           QOpenGLFramebufferObject *outputFBO);
    void applyHeightToNormal(QOpenGLFramebufferObject *inputFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyPerspectiveTransformFilter(QOpenGLFramebufferObject *inputFBO,
                                         QOpenGLFramebufferObject *outputFBO);
    void applySeamlessLinearFilter(QOpenGLFramebufferObject *inputFBO,
                                   QOpenGLFramebufferObject *outputFBO);
    void applySeamlessFilter(QOpenGLFramebufferObject *inputFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyOcclusionFilter(GLuint height_tex,
                              GLuint normal_tex,
                              QOpenGLFramebufferObject *outputFBO);
    void applyNormalToHeight(OpenGLFramebufferObjectProperties *image,
                             QOpenGLFramebufferObject *normalFBO,
                             QOpenGLFramebufferObject *heightFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyCPUNormalizationFilter(QOpenGLFramebufferObject *inputFBO,
                                     QOpenGLFramebufferObject *outputFBO);
    void applyAddNoiseFilter(QOpenGLFramebufferObject *inputFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyGaussFilter(QOpenGLFramebufferObject *sourceFBO, QOpenGLFramebufferObject *auxFBO,
                          QOpenGLFramebufferObject *outputFBO, int no_iter, float w =0);
    void applyGrayScaleFilter(QOpenGLFramebufferObject *inputFBO,
                              QOpenGLFramebufferObject *outputFBO);
    void applyInvertComponentsFilter(QOpenGLFramebufferObject *inputFBO,
                                    QOpenGLFramebufferObject *outputFBO);
    void applyColorHueFilter(QOpenGLFramebufferObject *inputFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyRoughnessFilter(QOpenGLFramebufferObject *inputFBO,
                              QOpenGLFramebufferObject *auxFBO,
                              QOpenGLFramebufferObject *outputFBO);

    void applyDGaussiansFilter(QOpenGLFramebufferObject *inputFBO,
                             QOpenGLFramebufferObject *auxFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyContrastFilter(QOpenGLFramebufferObject *inputFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyHeightProcessingFilter( QOpenGLFramebufferObject *inputFBO,
                                      QOpenGLFramebufferObject *outputFBO);
    void applyRemoveLowFreqFilter(QOpenGLFramebufferObject *inputFBO,
                                  QOpenGLFramebufferObject *auxFBO,
                                  QOpenGLFramebufferObject *outputFBO);
    void applyInverseColorFilter(QOpenGLFramebufferObject *inputFBO,
                                 QOpenGLFramebufferObject *outputFBO);
    void applyOverlayFilter( QOpenGLFramebufferObject *layerAFBO,
                             QOpenGLFramebufferObject *layerBFBO,
                             QOpenGLFramebufferObject *outputFBO);
    void applyRemoveShadingFilter(QOpenGLFramebufferObject *inputFBO,
                                   QOpenGLFramebufferObject *aoMaskFBO, QOpenGLFramebufferObject *refFBO,
                                   QOpenGLFramebufferObject *outputFBO);
    void applySmallDetailsFilter(QOpenGLFramebufferObject *inputFBO,
                                 QOpenGLFramebufferObject *auxFBO,
                                 QOpenGLFramebufferObject *outputFBO);
    void applyMediumDetailsFilter(QOpenGLFramebufferObject *inputFBO,
                                          QOpenGLFramebufferObject *auxFBO,
                                 QOpenGLFramebufferObject *outputFBO);
    void applySharpenBlurFilter(QOpenGLFramebufferObject *inputFBO,
                                QOpenGLFramebufferObject *auxFBO,
                                QOpenGLFramebufferObject *outputFBO);
    void applyNormalFilter(QOpenGLFramebufferObject *inputFBO);
    void applyNormalsStepFilter(QOpenGLFramebufferObject *inputFBO,
                                QOpenGLFramebufferObject *outputFBO);
    void applyNormalMixerFilter(QOpenGLFramebufferObject *inputFBO,
                                QOpenGLFramebufferObject *outputFBO);
    void applySobelToNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                  QOpenGLFramebufferObject *outputFBO, BaseMapConvLevelProperties &convProp);
    void applyNormalAngleCorrectionFilter(QOpenGLFramebufferObject *inputFBO,
                                          QOpenGLFramebufferObject *outputFBO);
    void applyNormalExpansionFilter(QOpenGLFramebufferObject *inputFBO,
                                    QOpenGLFramebufferObject *outputFBO);
    void applyMixNormalLevels(GLuint level0,
                              GLuint level1,
                              GLuint level2,
                              GLuint level3,
                              QOpenGLFramebufferObject *outputFBO);
    void applyBaseMapConversion(QOpenGLFramebufferObject *baseMapFBO,
                                QOpenGLFramebufferObject *auxFBO,
                                QOpenGLFramebufferObject *outputFBO, BaseMapConvLevelProperties &convProp);
    void applyPreSmoothFilter(QOpenGLFramebufferObject *inputFBO,
                              QOpenGLFramebufferObject *auxFBO,
                             QOpenGLFramebufferObject *outputFBO, BaseMapConvLevelProperties &convProp);
    void applyCombineNormalHeightFilter(QOpenGLFramebufferObject *normalFBO,
                                        QOpenGLFramebufferObject *heightFBO,
                                        QOpenGLFramebufferObject *outputFBO);
    void applyRoughnessColorFilter(QOpenGLFramebufferObject *inputFBO,
                                   QOpenGLFramebufferObject *outputFBO);
    void applyGrungeImageFilter (QOpenGLFramebufferObject *inputFBO,
                                 QOpenGLFramebufferObject *outputFBO,
                                 QOpenGLFramebufferObject *grungeFBO);
    void applyGrungeRandomizationFilter(QOpenGLFramebufferObject *inputFBO,
                                        QOpenGLFramebufferObject *outputFBO);
    void applyGrungeWarpNormalFilter(QOpenGLFramebufferObject *inputFBO,
                                     QOpenGLFramebufferObject *outputFBO);

    void applyAllUVsTransforms(QOpenGLFramebufferObject *inoutFBO);

    void copyFBO(QOpenGLFramebufferObject *src,QOpenGLFramebufferObject *dst);
    void copyTex2FBO(GLuint src_tex_id,QOpenGLFramebufferObject *dst);

    void updateProgramUniforms(int step);

private:
    void makeScreenQuad();

    QOpenGLShaderProgram *program;
    OpenGLFramebufferObjectProperties* activeImage;
    // Small FBO used for calculation of average color.
    QOpenGLFramebufferObject *averageColorFBO;
    // FBO with size 1024x1024.
    QOpenGLFramebufferObject *samplerFBO1;
    // FBO with size 1024x1024 used for different processing.
    QOpenGLFramebufferObject *samplerFBO2;
    // FBOs used in image processing.
    QOpenGLFramebufferObject *auxFBO1;
    QOpenGLFramebufferObject *auxFBO2;
    QOpenGLFramebufferObject *auxFBO3;
    QOpenGLFramebufferObject *auxFBO4;

    // 2 times smaller. 4 and 8
    QOpenGLFramebufferObject *auxFBO1BMLevels[3];
    QOpenGLFramebufferObject *auxFBO2BMLevels[3];
    QOpenGLFramebufferObject *auxFBO0BMLevels[3];

    // Used for painting texture.
    QOpenGLFramebufferObject *paintFBO;
    // Used for rendering to it.
    QOpenGLFramebufferObject *renderFBO;

    std::map<std::string,GLuint> subroutines;
    // All filters in one array.
    std::map<std::string,QOpenGLShaderProgram*> filter_programs;

    GLuint vbos[3];
    ConversionType conversionType;
    bool bShadowRender;
    // Draw quad but skip all the processing step (using during mouse interaction).
    bool bSkipProcessing;
    // Window width-height ratio.
    float windowRatio;
    // Active fbo width-height ratio.
    float fboRatio;

    // Image resize
    int resize_width;
    int resize_height;

    // Image translation and physical cursor position.
    // x position of the image in the window.
    double xTranslation;
    // y position.
    double yTranslation;
    // Physical cursor X position in window.
    double cursorPhysicalXPosition;
    double cursorPhysicalYPosition;

    // Magnification of the image.
    double zoom;
    double orthographicProjHeight;
    double orthographicProjWidth;

    // Perspective transformation.
    // Position of four corner used to perform perspective transformation of quad.
    QVector2D cornerPositions[4];
    QVector2D grungeCornerPositions[4];

    QVector4D cornerWeights;
    // Contains Id of current corner dragged or -1.
    int draggingCorner;
    QCursor cornerCursors[4];
    // Choose proper interpolation method.
    int gui_perspective_mode;
    // If 0 standard blending, if 1 mirror mode.
    int gui_seamless_mode;

    bool bToggleColorPicking;
    QtnPropertyABColor* ptr_ABColor;

    // UV manipulations method.
    UVManipulationMethods uvManilupationMethod;

    // OpenGL 330 variables.
    TextureTypes openGL330ForceTexType;

    // Rendering variables
    // Define if OpenGL is currently rendering some textures.
    bool bRendering;
};

#endif // OPENGLIMAGEEDITOR_H
