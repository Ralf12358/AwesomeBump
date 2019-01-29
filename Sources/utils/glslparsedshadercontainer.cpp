#include "glslparsedshadercontainer.h"

extern QString getDataDirectory(const QString& base);

GLSLParsedShaderContainer::GLSLParsedShaderContainer()
{
    qDebug() << "Parsing shaders in Render folder:";

    QDir currentDir(getDataDirectory(QString(RESOURCE_BASE) + "Core/Render"));
    currentDir.setFilter(QDir::Files);
    QStringList entries = currentDir.entryList();

    for(QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry)
    {
        QString filename = "Core/Render/" + *entry;
        qDebug() << "Parsing:" << filename;
        GLSLShaderParser *parser = new GLSLShaderParser;
        parser->parseShader(filename);
        glslParsedShaders.push_back(parser);
    }
}

GLSLParsedShaderContainer::~GLSLParsedShaderContainer()
{
    for(int i = 0 ; i < glslParsedShaders.size() ; ++i)
    {
        delete glslParsedShaders[i];
    }
    glslParsedShaders.clear();
}
