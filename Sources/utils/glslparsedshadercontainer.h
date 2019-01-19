#ifndef GLSLPARSEDSHADERCONTAINER_H
#define GLSLPARSEDSHADERCONTAINER_H

#include "glslshaderparser.h"

#define RENDER_FOLDER "Core/Render/"

class GLSLParsedShaderContainer
{
public:
    GLSLParsedShaderContainer();
    ~GLSLParsedShaderContainer();
    QVector<GLSLShaderParser*> glslParsedShaders;
};

#endif // GLSLPARSEDSHADERCONTAINER_H
