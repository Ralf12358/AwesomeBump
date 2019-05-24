#ifndef GLSLPARSEDSHADERCONTAINER_H
#define GLSLPARSEDSHADERCONTAINER_H

#include <QVector>

#include "glslshaderparser.h"

class GLSLParsedShaderContainer
{
public:
    GLSLParsedShaderContainer();
    ~GLSLParsedShaderContainer();
    QVector<GLSLShaderParser*> glslParsedShaders;
};

#endif // GLSLPARSEDSHADERCONTAINER_H
