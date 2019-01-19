#ifndef POSTFIXNAMES_H
#define POSTFIXNAMES_H

#include <QString>

enum TextureTypes
{
    DIFFUSE_TEXTURE = 0,
    NORMAL_TEXTURE ,
    SPECULAR_TEXTURE,
    HEIGHT_TEXTURE,
    OCCLUSION_TEXTURE,
    ROUGHNESS_TEXTURE,
    METALLIC_TEXTURE,
    MATERIAL_TEXTURE,
    GRUNGE_TEXTURE,
    MAX_TEXTURES_TYPE
};

class PostfixNames
{
public:
    static QString getPostfix(TextureTypes tType);
    static QString getTextureName(TextureTypes tType);

    static QString diffuseName;
    static QString normalName;
    static QString specularName;
    static QString heightName;
    static QString occlusionName;
    static QString roughnessName;
    static QString metallicName;
    static QString outputFormat;
};

#endif // POSTFIXNAMES_H
