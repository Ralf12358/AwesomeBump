#ifndef DISPLAY3DSETTINGS_H
#define DISPLAY3DSETTINGS_H

#include <QVector2D>

enum ShadingType
{
    SHADING_RELIEF_MAPPING = 0,
    SHADING_PARALLAX_NORMAL_MAPPING,
    SHADING_TESSELATION
};

enum ShadingModel
{
    SHADING_MODEL_PBR = 0,
    SHADING_MODEL_BUMP_MAPPING
};

class Display3DSettings
{
public:
    Display3DSettings();

    float     depthScale;
    float     uvScale;
    QVector2D uvOffset;
    float     specularIntensity;
    float     diffuseIntensity;
    float     lightPower ;
    float     lightRadius;
    ShadingType  shadingType;
    ShadingModel shadingModel;

    // Rendering quality settings.
    bool bUseCullFace;
    bool bUseSimplePBR;
    int  noTessSubdivision;
    int  noPBRRays;
    bool bBloomEffect;
    bool bDofEffect;
    bool bShowTriangleEdges;
    bool bLensFlares;
    static float openGLVersion;
};

#endif // DISPLAY3DSETTINGS_H
