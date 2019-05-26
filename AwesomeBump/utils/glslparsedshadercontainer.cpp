#include "glslparsedshadercontainer.h"

#include <QDir>
#include <QString>
#include <QStringList>

extern QString getDataDirectory(const QString& base);

GLSLParsedShaderContainer::GLSLParsedShaderContainer()
{
    qDebug() << "Parsing shaders";

    QDir currentDir(":/resources/renderers");
    currentDir.setFilter(QDir::Files);
    QStringList entries = currentDir.entryList();

    for(QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry)
    {
        QString filename = ":/resources/renderers/" + *entry;
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
