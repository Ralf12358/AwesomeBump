#include "glslshaderparser.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QStringListIterator>
#include <QTextStream>

#include "openglerrorcheck.h"

GLSLShaderParser::GLSLShaderParser() :
    program(0)
{
    parseShader(":/resources/shaders/awesombump.frag");
}

GLSLShaderParser::~GLSLShaderParser()
{
    cleanup();
}

bool GLSLShaderParser::parseShader(QString filename)
{
    QFile file(filename);
    shaderFilename = filename;

    if(!file.exists())
    {
        qDebug() << "GLSL Parser::File:" << filename << " does not exist.";
        return false;
    }
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "GLSL Parser::File:" << filename << " cannot be opened.";
        return false;
    }

    QFileInfo fileInfo(file);
    shaderBaseName = fileInfo.baseName();
    qDebug() << "GLSL Parser::Shader file:" << shaderBaseName;

    // Parse the whole file.
    QTextStream fileTextStream(&file);
    while (!fileTextStream.atEnd())
    {
        QString line = fileTextStream.readLine();
        parseLine(line);
    }
    file.close();

    return true;
}

void GLSLShaderParser::reparseShader()
{
    uniforms.clear();
    parseShader(shaderFilename);
}

void GLSLShaderParser::setParsedUniforms()
{
    for(int u = 0; u < uniforms.size(); u++)
    {
        UniformData& uniform = uniforms[u];
        switch (uniform.type)
        {
        case UNIFORM_TYPE_FLOAT:
            program->setUniformValue(
                        uniform.varName.toStdString().c_str(),
                        (float)uniform.value);
            break;
        case UNIFORM_TYPE_INT:
            program->setUniformValue(
                        uniform.varName.toStdString().c_str(),
                        (int)uniform.value);
            break;
        }
    }
}

void GLSLShaderParser::cleanup()
{
    if(program != NULL)
        delete program;
}

void GLSLShaderParser::parseLine(const QString& line)
{
    QStringList lineList = line.split(" ", QString::SkipEmptyParts);

    // Only parse lines with uniform key and 3 elements.
    if (lineList.length() < 3 ||
            lineList.at(0).compare("uniform", Qt::CaseSensitive) != 0)
        return;

    // Type should be second after "uniform" key.
    QString uType = lineList.at(1);

    // Get name part.
    QString uName = lineList.at(2);
    // Remove semicolon from name part.
    uName = uName.split(";", QString::SkipEmptyParts).at(0);

    // Ensure type is supported and name is not reseved.
    if(!supportedTypes.contains(uType, Qt::CaseSensitive) ||
            reservedNames.contains(uName, Qt::CaseSensitive))
        return;

    // Create uniform data.
    UniformData uniformData;
    if(uType.compare("int"))
        uniformData.type = UNIFORM_TYPE_INT;
    if(uType == "float")
        uniformData.type = UNIFORM_TYPE_FLOAT;
    uniformData.varName = uName;

    QStringListIterator supportedParameter(supportedParams);
    while (supportedParameter.hasNext())
        parseUniformParameters(uniformData, line, supportedParameter.next());

    qDebug() << uniformData.toString();
    uniforms.push_back(uniformData);
}

void GLSLShaderParser::parseUniformParameters(
        UniformData& uniformData,
        const QString& line,
        const QString& supportedParameter)
{
    QRegularExpressionMatch supportedParameterMatch =
            QRegularExpression(supportedParameter).match(line);

    // Ensure match was found.
    if (!supportedParameterMatch.hasMatch())
        return;

    QString match = supportedParameterMatch.captured(0);
    QStringList matchParts = match.split("=", QString::SkipEmptyParts);

    if(matchParts[0].contains("min"))
    {
        uniformData.min = QVariant(matchParts[1]).toFloat();
    }
    else if(matchParts[0].contains("max"))
    {
        uniformData.max = QVariant(matchParts[1]).toFloat();
    }
    else if(matchParts[0].contains("value"))
    {
        uniformData.value = QVariant(matchParts[1]).toFloat();
    }
    else if(matchParts[0].contains("step"))
    {
        uniformData.step = QVariant(matchParts[1]).toFloat();
    }
    else if(matchParts[0].contains("name"))
    {
        uniformData.name = (matchParts[1]);
    }
    else if(matchParts[0].contains("description"))
    {
        uniformData.description = (matchParts[1]);
    }
}

QStringList GLSLShaderParser::reservedNames(
{
            "num_mipmaps",
            "gui_bSpecular",
            "bOcclusion",
            "gui_bHeight",
            "gui_bDiffuse",
            "gui_bOcclusion",
            "gui_bNormal",
            "gui_bRoughness",
            "gui_bMetallic",
            "gui_LightPower",
            "gui_LightRadius",
            "gui_noPBRRays",
            "gui_bUseSimplePBR",
            "gui_bMaterialsPreviewEnabled",
            "gui_bShowTriangleEdges",
            "gui_depthScale",
            "gui_SpecularIntensity",
            "gui_DiffuseIntensity",
            "gui_shading_type",
            "gui_shading_model",
            "cameraPos",
            "lightDirection",
            "lightPos",
            "ModelMatrix",
            "ModelViewMatrix"
        });

QStringList GLSLShaderParser::supportedTypes(
{
            "int",
            "float"
        });

QStringList GLSLShaderParser::supportedParams(
{
            "[ ]*max[ ]*=[ ]*[+-]?[0-9]*\\.?[0-9]+",
            "[ ]*min[ ]*=[ ]*[+-]?[0-9]*\\.?[0-9]+",
            "[ ]*value[ ]*=[ ]*[+-]?[0-9]*\\.?[0-9]+",
            "[ ]*step[ ]*=[ ]*[+-]?[0-9]*\\.?[0-9]+",
            "[ ]*name[ ]*=[ ]*\"[a-zA-Z0-9 -+'()]*\"",
            "[ ]*description[ ]*=[ ]*\"[a-zA-Z0-9 -+'()]*\""
        });
