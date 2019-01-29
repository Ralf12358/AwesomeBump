#ifndef GLSLSHADERPARSER_H
#define GLSLSHADERPARSER_H

#include <QOpenGLShaderProgram>
#include <QString>
#include <QStringList>
#include <QVector>

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

class GLSLShaderParser
{
public:
    GLSLShaderParser();
    ~GLSLShaderParser();

    bool parseShader(QString filename);
    void reparseShader();
    void setParsedUniforms();

    // Contains all editable parsed uniforms.
    QVector<UniformData> uniforms;
    // GLSL shader
    QOpenGLShaderProgram *program;

    QString shaderBaseName;
    QString shaderFilename;

private:
    void cleanup();
    void parseLine(const QString& line);
    void parseUniformParameters(UniformData& uniformData,
            const QString& line,
            const QString& param);

    // List of uniforms names which will be not parsed.
    static QStringList reservedNames;
    // List of types (int, float, etc.) which can be parsed.
    static QStringList supportedTypes;
    // List of regular expresions to obtain the maximum, minimum etc. parameters of the uniform.
    static QStringList supportedParams;
};

#endif // GLSLSHADERPARSER_H
