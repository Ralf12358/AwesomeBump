#include "mesh.h"

#include <QDebug>
#include <QFileInfo>
#include <QOpenGLBuffer>
#include <QVector2D>
#include <QVector3D>

#include "openglerrorcheck.h"

Material::Material() :
    shininess(1.0f),
    ior(1.0f),
    dissolve(1.0f),
    illum(0)
{}

Shape::Shape(const QString& shapeName,
             const QString &materialName,
             const QVector<QVector3D>& geometricVertices,
             const QVector<QVector3D> &textureCoordinates,
             const QVector<QVector3D>& vertexNormals,
             const QVector<Face>& faces) :
    shapeName(shapeName),
    materialName(materialName)
{
    for (const Face& face : faces)
    {
        // Convert polygons to triangles.
        FaceElement firstElement = face[0];
        FaceElement thirdElement = face[1];
        for (int vertex = 2; vertex < face.count(); ++vertex)
        {
            FaceElement secondElement= thirdElement;
            thirdElement = face[vertex];
            createTriangle(geometricVertices, textureCoordinates, vertexNormals,
                           firstElement, secondElement, thirdElement);
        }
    }
}

const QString& Shape::getShapeName()
{
    return shapeName;
}

const QString& Shape::getMaterialName()
{
    return materialName;
}

const QVector<QVector3D>& Shape::getTriangleVertices()
{
    return triangleVertices;
}

const QVector<QVector3D>& Shape::getTriangleTextureCoordinates()
{
    return triangleTextureCoordinates;
}

const QVector<QVector3D>& Shape::getTriangleNormals()
{
    return triangleNormals;
}

void Shape::createTriangle(
        const QVector<QVector3D>& geometricVertices,
        const QVector<QVector3D> &textureCoordinates,
        const QVector<QVector3D>& vertexNormals,
        const FaceElement& firstElement,
        const FaceElement& secondElement,
        const FaceElement& thirdElement)
{
    // Add geometric vertices.
    triangleVertices.append(geometricVertices[firstElement[0] - 1]);
    triangleVertices.append(geometricVertices[secondElement[0] - 1]);
    triangleVertices.append(geometricVertices[thirdElement[0] - 1]);
    // Add texture coordinates.
    triangleTextureCoordinates.append(textureCoordinates[firstElement[1] - 1]);
    triangleTextureCoordinates.append(textureCoordinates[secondElement[1] - 1]);
    triangleTextureCoordinates.append(textureCoordinates[thirdElement[1] - 1]);
    // Add normals.
    triangleNormals.append(vertexNormals[firstElement[2] - 1]);
    triangleNormals.append(vertexNormals[secondElement[2] - 1]);
    triangleNormals.append(vertexNormals[thirdElement[2] - 1]);
}

Mesh::Mesh(const QString& objFilename, const QString& mtlDirectory) :
    filename(objFilename), materialDirectory(mtlDirectory),
    radius(0), loaded(false)
{
    QFile objFile(objFilename);
    if (!objFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qDebug() << "Could not open: " << objFilename;
        meshLog.append("Could not open: " + objFilename + '\n');
        return;
    }

    LoadObject(objFile, mtlDirectory);

    if(shapes.count() == 0)
    {
        qDebug() << "No shapes found in " << objFilename;
        meshLog.append("No shapes found in " + objFilename + '\n');
        return;
    }

    if (geometricVertices.count() == 0)
    {
        qDebug() << objFilename << " has no geometric vertics.";
        meshLog.append(objFilename + " has no geometric vertics." + '\n');
    }
    if (textureCoordinates.count() == 0)
    {
        qDebug() << objFilename << " has no tecture coordinates.";
        meshLog.append(objFilename + " has no tecture coordinates." + '\n');
    }
    if (vertexNormals.count() == 0)
    {
        qDebug() << objFilename << " has no vertex normals.";
        meshLog.append(objFilename + " has no vertex normals." + '\n');
    }
    if (geometricVertices.count() == 0 ||
            textureCoordinates.count() == 0 ||
            vertexNormals.count() == 0) return;

    for (Shape shape : shapes)
    {
        if (shape.getTriangleVertices().count() == 0)
        {
            qDebug() << shape.getShapeName() << " has no geometric vertics.";
            meshLog.append(shape.getShapeName() + " has no geometric vertics." + '\n');
        }
        if (shape.getTriangleTextureCoordinates().count() == 0)
        {
            qDebug() << shape.getShapeName() << " has no tecture coordinates.";
            meshLog.append(shape.getShapeName() + " has no tecture coordinates." + '\n');
        }
        if (shape.getTriangleNormals().count() == 0)
        {
            qDebug() << shape.getShapeName() << " has no vertex normals.";
            meshLog.append(shape.getShapeName() + " has no vertex normals." + '\n');
        }
        if (shape.getTriangleVertices().count() == 0 ||
                shape.getTriangleTextureCoordinates().count() == 0 ||
                shape.getTriangleNormals().count() == 0) continue;

        triangleVertices.append(shape.getTriangleVertices());
        triangleTextureCoordinates.append(shape.getTriangleTextureCoordinates());
        triangleNormals.append(shape.getTriangleNormals());
    }

    centreOfMass /= geometricVertices.count();
    for(QVector3D position : geometricVertices)
    {
        float distanceToCentre = (centreOfMass - position).length();
        if(distanceToCentre > radius) radius = distanceToCentre;
    }

    calculateTangents();
    calculateSmoothNormals();

    loaded = true;
}

Mesh::~Mesh()
{
}

const QVector<QVector3D>& Mesh::getVertices()
{
    return triangleVertices;
}

const QVector<QVector3D>& Mesh::getTextureCoordinates()
{
    return triangleTextureCoordinates;
}

const QVector<QVector3D>& Mesh::getNormals()
{
    return triangleNormals;
}

const QVector<QVector3D>& Mesh::getSmoothedNormals()
{
    return triangleSmoothedNormals;
}

const QVector<QVector3D>& Mesh::getTangents()
{
    return triangleTangents;
}

const QVector<QVector3D>& Mesh::getBitangents()
{
    return triangleBitangents;
}

bool Mesh::isLoaded()
{
    return loaded;
}

const QString Mesh::getMeshLog()
{
    return meshLog;
}

QVector3D Mesh::getCentreOfMass()
{
    return centreOfMass;
}

float Mesh::getRadius()
{
    return radius;
}

void Mesh::LoadObject(QFile& objFile, QString mtlDirirectory)
{
    if (mtlDirirectory.isEmpty())
        mtlDirirectory = QFileInfo(objFile).absolutePath();
    if (!mtlDirirectory.endsWith('/'))
        mtlDirirectory += '/';

    QTextStream inStream(&objFile);

    QString objectName;
    QString groupName;
    QString materialName;
    QVector<Face> faces;

    while (!inStream.atEnd()) {
        QString line = inStream.readLine().trimmed();

        if (line.isEmpty()) continue;

        // Comment line.
        if (line.startsWith("//") || line.startsWith('#')) continue;

        // Vertex normal.
        if (line.startsWith("vn"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D normal;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> normal[index];
            if (lineStream.status() == QTextStream::Ok)
                vertexNormals.append(normal.normalized());
            continue;
        }

        // Texture Coordinate.
        if (line.startsWith("vt"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D coordinate;
            for (unsigned int index = 0; index < 2; ++index)
                lineStream >> coordinate[index];
            if (lineStream.status() == QTextStream::Ok)
                textureCoordinates.append(coordinate);
            continue;
        }

        // Vertex.
        if (line.startsWith('v'))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D position;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> position[index];
            if (lineStream.status() == QTextStream::Ok)
            {
                geometricVertices.append(position);
                centreOfMass += position;
            }
            continue;
        }

        // Face
        if (line.startsWith('f'))
        {
            line = line.remove(0, 1).trimmed();
            QStringList faceElementStrings = line.split(QRegExp("\\s"));
            Face face;
            for (QString faceElementString : faceElementStrings)
            {
                QStringList elementStrings = faceElementString.split('/');
                FaceElement faceElement(3);
                for (int elementIndex = 0; elementIndex < elementStrings.count() && elementIndex < 3; ++elementIndex)
                {
                    QString elementString = elementStrings[elementIndex];
                    QTextStream elementStream(&elementString);
                    elementStream >> faceElement[elementIndex];
                }
                face.append(faceElement);
            }
            faces.append(face);
            continue;
        }

        // Use material
        if (line.startsWith("usemtl"))
        {
            materialName = line.remove(0,6).trimmed();
            if (!materials.contains(materialName))
            {
                qDebug() << "Material " << materialName << " not found!";
                meshLog.append("Material " + materialName + " not found!" + '\n');
            }
            continue;
        }

        // Load materials
        if (line.startsWith("mtllib"))
        {
            QString mtlFileName = mtlDirirectory + line.remove(0,6).trimmed();
            QFile mtlFile(mtlFileName);
            if (mtlFile.open(QIODevice::ReadOnly | QIODevice::Text))
                LoadMaterial(mtlFile);
            else
            {
                qDebug() << "Could not open material file: " << mtlFileName;
                meshLog.append("Could not open material file: " + mtlFileName + '\n');
            }
            continue;
        }

        // Group name
        if (line.startsWith('g'))
        {

            if (!faces.empty())
            {
                QString name = groupName.length() > 0 ? groupName : objectName;
                Shape shape(name, materialName, geometricVertices, textureCoordinates, vertexNormals, faces);
                shapes.insert(name, shape);
                faces.clear();
            }
            groupName = line.remove(0, 1).trimmed();
            continue;
        }

        // Object name
        if (line.startsWith('o'))
        {
            if (!faces.empty())
            {
                QString name = groupName.length() > 0 ? groupName : objectName;
                Shape shape(name, materialName, geometricVertices, textureCoordinates, vertexNormals, faces);
                shapes.insert(name, shape);
                faces.clear();
            }
            objectName = line.remove(0, 1).trimmed();
            groupName = "";
            continue;
        }
    }

    if (!faces.empty())
    {
        QString name = groupName.length() > 0 ? groupName : objectName;
        Shape shape(name, materialName, geometricVertices, textureCoordinates, vertexNormals, faces);
        shapes.insert(name, shape);
        faces.clear();
    }
}

void Mesh::LoadMaterial(QFile& mtlFile)
{
    QTextStream inStream(&mtlFile);

    Material material;

    while (!inStream.atEnd()) {
        QString line = inStream.readLine().trimmed();

        if (line.isEmpty()) continue;

        // Comment line.
        if (line.startsWith("//") || line.startsWith('#')) continue;

        // Ambient
        if (line.startsWith("Ka"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D colour;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> colour[index];
            if (lineStream.status() == QTextStream::Ok)
                material.ambient = colour;
            continue;
        }

        // Diffuse
        if (line.startsWith("Kd"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D colour;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> colour[index];
            if (lineStream.status() == QTextStream::Ok)
                material.diffuse = colour;
            continue;
        }

        // Specular
        if (line.startsWith("Ks"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D colour;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> colour[index];
            if (lineStream.status() == QTextStream::Ok)
                material.specular = colour;
            continue;
        }

        // Transmittance
        if (line.startsWith("Kt"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D colour;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> colour[index];
            if (lineStream.status() == QTextStream::Ok)
                material.transmittance = colour;
            continue;
        }

        // Emission
        if (line.startsWith("Ke"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            QVector3D colour;
            for (unsigned int index = 0; index < 3; ++index)
                lineStream >> colour[index];
            if (lineStream.status() == QTextStream::Ok)
                material.emission = colour;
            continue;
        }

        // Shininess
        if (line.startsWith("Ns"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            float shininess;
            lineStream >> shininess;
            if (lineStream.status() == QTextStream::Ok)
                material.shininess = shininess;
            continue;
        }

        // Index of Refraction
        if (line.startsWith("Ni"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            float ior;
            lineStream >> ior;
            if (lineStream.status() == QTextStream::Ok)
                material.ior = ior;
            continue;
        }

        // Dissolve
        if (line.startsWith('d'))
        {
            line = line.remove(0, 1);
            QTextStream lineStream(&line);
            float dissolve;
            lineStream >> dissolve;
            if (lineStream.status() == QTextStream::Ok)
                material.dissolve = dissolve;
            continue;
        }
        if (line.startsWith("Tr"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            float transparency;
            lineStream >> transparency;
            if (lineStream.status() == QTextStream::Ok)
                material.dissolve = 1 - transparency;
            continue;
        }

        // Illumination model
        if (line.startsWith("illum"))
        {
            line = line.remove(0, 5);
            QTextStream lineStream(&line);
            int illum;
            lineStream >> illum;
            if (lineStream.status() == QTextStream::Ok)
                material.illum = illum;
            continue;
        }

        // Ambient texture map
        if (line.startsWith("map_Ka"))
        {
            material.ambientTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // Diffuse texture map
        if (line.startsWith("map_Kd"))
        {
            material.diffuseTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // Specular texture map
        if (line.startsWith("map_Ks"))
        {
            material.specularTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // Normal texture map
        if (line.startsWith("map_Ns"))
        {
            material.normalTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // New material.
        if (line.startsWith("newmtl"))
        {
            // Save previous material.
            if (!material.name.isEmpty())
                materials.insert(material.name, material);

            // Create new material.
            material = Material();
            material.name = line.remove(0,6).trimmed();
            continue;
        }

        // Unknown parameter
        qDebug() << "Unknown parameter in material file " << mtlFile.fileName() << ": " << line;
        meshLog.append("Unknown parameter in material file " + mtlFile.fileName() + ": " + line + '\n');
    }

    // Save material.
    if (!material.name.isEmpty())
        materials.insert(material.name, material);
}

// Calculation based on article:
// Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”.
// Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
void Mesh::calculateTangents()
{
    int vertexCount = triangleVertices.count();

    QVector3D *tan1 = new QVector3D[vertexCount * 2];
    QVector3D *tan2 = tan1 + vertexCount;

    for (int a = 0; a < vertexCount / 3; a++)
    {
        long i1 = 3 * a + 0;
        long i2 = 3 * a + 1;
        long i3 = 3 * a + 2;

        const QVector3D& v1 = triangleVertices[i1];
        const QVector3D& v2 = triangleVertices[i2];
        const QVector3D& v3 = triangleVertices[i3];

        const QVector3D& w1 = triangleTextureCoordinates[i1];
        const QVector3D& w2 = triangleTextureCoordinates[i2];
        const QVector3D& w3 = triangleTextureCoordinates[i3];

        float x1 = v2.x() - v1.x();
        float x2 = v3.x() - v1.x();
        float y1 = v2.y() - v1.y();
        float y2 = v3.y() - v1.y();
        float z1 = v2.z() - v1.z();
        float z2 = v3.z() - v1.z();

        float s1 = w2.x() - w1.x();
        float s2 = w3.x() - w1.x();
        float t1 = w2.y() - w1.y();
        float t2 = w3.y() - w1.y();

        float r = 1.0F / (s1 * t2 - s2 * t1);
        QVector3D sdir((t2 * x1 - t1 * x2) * r,
                       (t2 * y1 - t1 * y2) * r,
                       (t2 * z1 - t1 * z2) * r);
        QVector3D tdir((s1 * x2 - s2 * x1) * r,
                       (s1 * y2 - s2 * y1) * r,
                       (s1 * z2 - s2 * z1) * r);

        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan1[i3] += sdir;

        tan2[i1] += tdir;
        tan2[i2] += tdir;
        tan2[i3] += tdir;
    }

    for (int a = 0; a < vertexCount; a++)
    {
        const QVector3D& normal = triangleNormals[a];
        const QVector3D& tangent1 = tan1[a].normalized();
        // Gram-Schmidt orthogonalize
        QVector3D tangent = tangent1 - normal * QVector3D::dotProduct(normal, tangent1);
        tangent.normalize();
        // Calculate handedness
        float handedness =(QVector3D::dotProduct(QVector3D::crossProduct(normal, tangent1), tan2[a]) < 0.0F) ? -1.0F : 1.0F;
        triangleTangents.append(tangent);
        QVector3D bitangent = QVector3D::crossProduct(normal, tangent1) * handedness;
        bitangent.normalize();
        triangleBitangents.append(bitangent);
    }
    delete[] tan1;
}

void Mesh::calculateSmoothNormals()
{
    triangleSmoothedNormals.resize(triangleNormals.size());

    QVector3D minPosition, maxPosition;
    for(QVector3D position : triangleVertices)
    {
        if(position.x() < minPosition.x())
            minPosition.setX(position.x());
        if(position.y() < minPosition.y())
            minPosition.setY(position.y());
        if(position.z() < minPosition.z())
            minPosition.setZ(position.z());

        if(position.x() > maxPosition.x())
            maxPosition.setX(position.x());
        if(position.y() > maxPosition.y())
            maxPosition.setY(position.y());
        if(position.z() > maxPosition.z())
            maxPosition.setZ(position.z());
    }

    QVector<int> vertexGroup[100][100][100];
    float length = (maxPosition - minPosition).length();

    for(int index = 0; index < triangleVertices.count(); ++index)
    {
        const QVector3D& position = triangleVertices[index];
        QVector3D pos = (position - minPosition) / length * 100;
        vertexGroup[(unsigned int)pos.x()][(unsigned int)pos.y()][(unsigned int)pos.z()].append(index);
    }

    for(int x = 0; x < 100; x++) for(int y = 0; y < 100; y++) for(int z = 0; z < 100; z++)
    {
        for(int indexa : vertexGroup[x][y][z])
        {
            QVector3D smoothed;
            for(int indexb : vertexGroup[x][y][z])
            {
                if((triangleVertices[indexa] - triangleVertices[indexb]).length() / radius < 1.0E-5)
                    smoothed += triangleNormals[indexb];
            }
            triangleSmoothedNormals[indexa] = smoothed.normalized();
        }
    }
}
