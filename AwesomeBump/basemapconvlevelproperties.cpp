#include "basemapconvlevelproperties.h"

BaseMapConvLevelProperties::BaseMapConvLevelProperties()
{
    conversionBaseMapAmplitude       = 0;
    conversionBaseMapFlatness        = 0.5;
    conversionBaseMapNoIters         = 0;
    conversionBaseMapFilterRadius    = 3;
    conversionBaseMapMixNormals      = 1.0;
    conversionBaseMapPreSmoothRadius = 0;
    conversionBaseMapBlending        = 1.0;
}

void BaseMapConvLevelProperties::fromProperty(QtnPropertySetConvertsionBaseMapLevelProperty& level)
{
    conversionBaseMapAmplitude       = level.Amplitude;
    conversionBaseMapFlatness        = level.Flatness;
    conversionBaseMapNoIters         = level.NumIters;
    conversionBaseMapFilterRadius    = level.FilterRadius;
    conversionBaseMapMixNormals      = level.Edges;
    conversionBaseMapPreSmoothRadius = level.PreSmoothRadius;
    conversionBaseMapBlending        = level.Blending;
}
