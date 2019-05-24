#include "postfixnames.h"

QString  PostfixNames::diffuseName   = "_d";
QString  PostfixNames::normalName    = "_n";
QString  PostfixNames::specularName  = "_s";
QString  PostfixNames::heightName    = "_h";
QString  PostfixNames::occlusionName = "_o";
QString  PostfixNames::roughnessName = "_r";
QString  PostfixNames::metallicName  = "_m";
QString  PostfixNames::outputFormat  = ".png";

QString PostfixNames::getPostfix(TextureType tType)
{
    switch(tType)
    {
    case(DIFFUSE_TEXTURE):
        return diffuseName;
        break;
    case(NORMAL_TEXTURE):
        return normalName;
        break;
    case(SPECULAR_TEXTURE):
        return specularName;
        break;
    case(HEIGHT_TEXTURE):
        return heightName;
        break;
    case(OCCLUSION_TEXTURE):
        return occlusionName;
        break;
    case(ROUGHNESS_TEXTURE):
        return roughnessName;
        break;
    case(METALLIC_TEXTURE) :
        return metallicName;
        break;
    default:
        return diffuseName;
        break;
    }
}

QString PostfixNames::getTextureName(TextureType tType)
{
    switch(tType)
    {
    case(DIFFUSE_TEXTURE):
        return "diffuse";
        break;
    case(NORMAL_TEXTURE):
        return "normal";
        break;
    case(SPECULAR_TEXTURE):
        return "specular";
        break;
    case(HEIGHT_TEXTURE):
        return "height";
        break;
    case(OCCLUSION_TEXTURE):
        return "occlusion";
        break;
    case(ROUGHNESS_TEXTURE):
        return "roughness";
        break;
    case(METALLIC_TEXTURE):
        return "metallic";
        break;
    case(MATERIAL_TEXTURE):
        return "material";
    case(GRUNGE_TEXTURE):
        return "grunge";
        break;
    default:
        return "default-diffuse";
        break;
    }
}
