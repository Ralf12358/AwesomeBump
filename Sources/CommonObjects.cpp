#include "CommonObjects.h"

SeamlessMode FBOImageProporties::seamlessMode                 = SEAMLESS_NONE;
float FBOImageProporties::seamlessSimpleModeRadius            = 0.5;
float FBOImageProporties::seamlessContrastPower               = 0.0;
float FBOImageProporties::seamlessContrastStrenght            = 0.0;
int FBOImageProporties::seamlessSimpleModeDirection           = 0; // xy
SourceImageType FBOImageProporties::seamlessContrastInputType = INPUT_FROM_HEIGHT_INPUT;
bool FBOImageProporties::bSeamlessTranslationsFirst           = true;
int FBOImageProporties::seamlessMirroModeType                 = 0;
bool FBOImageProporties::bConversionBaseMap                   = false;
bool FBOImageProporties::bConversionBaseMapShowHeightTexture  = false;
int FBOImageProporties::currentMaterialIndeks                 = MATERIALS_DISABLED;
RandomTilingMode FBOImageProporties::seamlessRandomTiling     = RandomTilingMode();

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
