#ifndef BASEMAPCONVLEVELPROPERTIES_H
#define BASEMAPCONVLEVELPROPERTIES_H

#include "properties/ImageProperties.peg.h"

class BaseMapConvLevelProperties
{
public:
    BaseMapConvLevelProperties();

    void fromProperty(QtnPropertySetConvertsionBaseMapLevelProperty& level);

    float conversionBaseMapAmplitude;
    float conversionBaseMapFlatness;
    int   conversionBaseMapNoIters;
    float conversionBaseMapFilterRadius;
    float conversionBaseMapMixNormals;
    float conversionBaseMapPreSmoothRadius;
    float conversionBaseMapBlending;
};

#endif // BASEMAPCONVLEVELPROPERTIES_H
