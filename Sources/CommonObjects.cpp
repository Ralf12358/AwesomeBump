#include "CommonObjects.h"

SeamlessMode FBOImageProperties::seamlessMode                 = SEAMLESS_NONE;
float FBOImageProperties::seamlessSimpleModeRadius            = 0.5;
float FBOImageProperties::seamlessContrastPower               = 0.0;
float FBOImageProperties::seamlessContrastStrenght            = 0.0;
int FBOImageProperties::seamlessSimpleModeDirection           = 0; // xy
SourceImageType FBOImageProperties::seamlessContrastInputType = INPUT_FROM_HEIGHT_INPUT;
bool FBOImageProperties::bSeamlessTranslationsFirst           = true;
int FBOImageProperties::seamlessMirroModeType                 = 0;
bool FBOImageProperties::bConversionBaseMap                   = false;
bool FBOImageProperties::bConversionBaseMapShowHeightTexture  = false;
int FBOImageProperties::currentMaterialIndeks                 = MATERIALS_DISABLED;
RandomTilingMode FBOImageProperties::seamlessRandomTiling     = RandomTilingMode();

float Display3DSettings::openGLVersion = 3.3;

QString  PostfixNames::diffuseName   = "_d";
QString  PostfixNames::normalName    = "_n";
QString  PostfixNames::specularName  = "_s";
QString  PostfixNames::heightName    = "_h";
QString  PostfixNames::occlusionName = "_o";
QString  PostfixNames::roughnessName = "_r";
QString  PostfixNames::metallicName  = "_m";
QString  PostfixNames::outputFormat  = ".png";

bool FBOImages::bUseLinearInterpolation = true;
