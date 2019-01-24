#ifndef GLSLSHADERPARSER_H
#define GLSLSHADERPARSER_H

#include <QOpenGLFunctions_3_3_Core>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QOpenGLShaderProgram>
#include <QDebug>
#include <QDir>

enum UniformDataType
{
    UNIFORM_TYPE_INT = 0,
    UNIFORM_TYPE_FLOAT
};

struct UniformData
{
    UniformDataType type;
    float value;
    float min;
    float max;
    float step;
    QString varName;
    QString name;
    QString description;
    UniformData()
    {
        value = 50.0;
        min   = 0.0;
        max   = 100.0;
        step  = 1.0;
        name = "param#";
        description = "None";
    }
    QString toString()
    {
        QString uniformString =
                "Uniform    :" + name + "\n" +
                "Type       :" + QString::number(type) + "\n" +
                "Data       :" + QString::number(value) +
                " range=[" +
                    QString::number(min) + "," +
                    QString::number(max) + ";" +
                    QString::number(step)+ "]\n" +
                "Description:" + description;
        return uniformString;
    }
};

class GLSLShaderParser: public QOpenGLFunctions_3_3_Core
{
public:
    GLSLShaderParser();
    bool parseShader(QString path);
    void reparseShader();
    void setParsedUniforms();
    ~GLSLShaderParser();

    QString shaderName;
    // Contains all editable parsed uniforms.
    QVector<UniformData> uniforms;
    QString shaderPath;
    // GLSL shader
    QOpenGLShaderProgram *program;

private:
    void cleanup();
    void parseLine(const QString& line);
    void parseUniformParameters(UniformData &uniform, const QString &line, const QString& param);

    // List of uniforms names which will be not parsed.
    QStringList reservedNames;
    // List of types (int, float, ...) which can be parsed.
    QStringList supportedTypes;
    // List of regular expresions to obtain the maxium, minium etc parameters of the uniform.
    QStringList supportedParams;
};

#endif // GLSLSHADERPARSER_H
