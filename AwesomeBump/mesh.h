#ifndef MESH_H
#define MESH_H

#include <QFile>
#include <QHash>
#include <QString>
#include <QTextStream>
#include <QVector>
#include <QVector3D>
#include <QVector4D>

class Material
{
public:
    Material();

    QString name;
    QString ambientTexture;
    QString diffuseTexture;
    QString specularTexture;
    QString normalTexture;
    QVector3D ambient;
    QVector3D diffuse;
    QVector3D specular;
    QVector3D transmittance;
    QVector3D emission;
    float shininess;
    // Index of refraction
    float ior;
    // 0: Fully transparent - 1: Opaque
    float dissolve;
    // Illumination model (see http://www.fileformat.info/format/material/)
    int illum;
};

typedef QVector<int> FaceElement;
typedef QVector<FaceElement> Face;

class Shape
{
public:
    Shape(const QString& shapeName,
            const QString& materialName,
            const QVector<QVector3D>& geometricVertices,
            const QVector<QVector3D>& textureCoordinates,
            const QVector<QVector3D>& vertexNormals,
            const QVector<Face>& faces);

    const QString& getShapeName();
    const QString& getMaterialName();
    const QVector<QVector3D>& getTriangleVertices();
    const QVector<QVector3D>& getTriangleTextureCoordinates();
    const QVector<QVector3D>& getTriangleNormals();

private:
    void createTriangle(
            const QVector<QVector3D> &geometricVertices,
            const QVector<QVector3D> &textureCoordinates,
            const QVector<QVector3D> &vertexNormals,
            const FaceElement &firstElement,
            const FaceElement &secondElement,
            const FaceElement &thirdElement);

    QString shapeName;
    QString materialName;
    QVector<QVector3D> triangleVertices;
    QVector<QVector3D> triangleTextureCoordinates;
    QVector<QVector3D> triangleNormals;
};

class Mesh
{
public:
    Mesh(const QString& objFilename, const QString& mtlDirectory = "");
    virtual ~Mesh();

    const QVector<QVector3D>& getVertices();
    const QVector<QVector3D>& getTextureCoordinates();
    const QVector<QVector3D>& getNormals();
    const QVector<QVector3D>& getSmoothedNormals();
    const QVector<QVector3D>& getTangents();
    const QVector<QVector3D>& getBitangents();

    bool isLoaded();
    const QString getMeshLog();
    QVector3D getCentreOfMass();
    float getRadius();

private:
    //Loads shapes and materials from an .obj and .mtl file.
    void LoadObject(QFile& objFile, QString mtlDirirectory);
    //Loads materials from an .mtl file.
    void LoadMaterial(QFile &mtlFile);
    void calculateTangents();
    void calculateSmoothNormals();

    const QString filename;
    const QString materialDirectory;

    // Helps to align mesh to the center of the screen.
    QVector3D centreOfMass;
    // Maxiumum distance from centre of mass to some vertex.
    float radius;

    // Object File Components
    QVector<QVector3D> geometricVertices;
    QVector<QVector3D> textureCoordinates;
    QVector<QVector3D> vertexNormals;
    QHash<QString, Shape> shapes;
    QHash<QString, Material> materials;

    // OpenGL Components
    QVector<QVector3D> triangleVertices;
    QVector<QVector3D> triangleTextureCoordinates;
    QVector<QVector3D> triangleNormals;
    QVector<QVector3D> triangleSmoothedNormals;
    QVector<QVector3D> triangleTangents;
    QVector<QVector3D> triangleBitangents;

    bool loaded;
    QString meshLog;
};

#endif // MESH_H
