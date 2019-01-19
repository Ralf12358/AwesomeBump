#include "display3dsettings.h"

float Display3DSettings::openGLVersion = 3.3;

Display3DSettings::Display3DSettings()
{
    depthScale = 0.1;
    uvScale    = 1.0;
    uvOffset   = QVector2D(0.0,0.0);
    specularIntensity = 1.0;
    diffuseIntensity  = 1.0;
    lightPower        = 0.1;
    lightRadius       = 0.1;
    shadingType       = SHADING_RELIEF_MAPPING;
    shadingModel      = SHADING_MODEL_PBR;

    bUseCullFace  = false;
    bUseSimplePBR = false;
    noTessSubdivision  = 16;
    noPBRRays          = 15;
    bBloomEffect       = true;
    bDofEffect         = true;
    bShowTriangleEdges = false;
    bLensFlares        = true;
}
