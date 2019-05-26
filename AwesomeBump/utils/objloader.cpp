#include "objloader.h"

#include <QFileInfo>
#include <QDebug>

struct vertex_index
{
    int v_idx, vt_idx, vn_idx;
    vertex_index(){};
    vertex_index(int idx) : v_idx(idx), vt_idx(idx), vn_idx(idx){};
    vertex_index(int vidx, int vtidx, int vnidx)
        : v_idx(vidx), vt_idx(vtidx), vn_idx(vnidx){};
};

static inline bool operator<(const vertex_index &a, const vertex_index &b) {
  if (a.v_idx != b.v_idx)   return (a.v_idx < b.v_idx);
  if (a.vn_idx != b.vn_idx) return (a.vn_idx < b.vn_idx);
  if (a.vt_idx != b.vt_idx) return (a.vt_idx < b.vt_idx);
  return false;
}

static unsigned int updateVertex(
        QMap<vertex_index, unsigned int> &vertexCache,
        QVector<float> &positions, QVector<float> &normals,
        QVector<float> &texcoords,
        const QVector<float> &in_positions,
        const QVector<float> &in_normals,
        const QVector<float> &in_texcoords, const vertex_index &i)
{
    const QMap<vertex_index, unsigned int>::iterator it = vertexCache.find(i);

    if (it != vertexCache.end())
    {
        return it.value();
    }

    assert(in_positions.size() > (int)(3 * (i.v_idx - 1) + 2));

    positions.push_back(in_positions[3 * (i.v_idx - 1) + 0]);
    positions.push_back(in_positions[3 * (i.v_idx - 1) + 1]);
    positions.push_back(in_positions[3 * (i.v_idx - 1) + 2]);

    if (i.vn_idx >= 0)
    {
        normals.push_back(in_normals[3 * (i.vn_idx - 1) + 0]);
        normals.push_back(in_normals[3 * (i.vn_idx - 1) + 1]);
        normals.push_back(in_normals[3 * (i.vn_idx - 1) + 2]);
    }

    if (i.vt_idx >= 0)
    {
        texcoords.push_back(in_texcoords[2 * (i.vt_idx - 1) + 0]);
        texcoords.push_back(in_texcoords[2 * (i.vt_idx - 1) + 1]);
    }

    unsigned int idx = positions.size() / 3 - 1;
    vertexCache[i] = idx;

    return idx;
}

bool exportFacesToShape(
        Shape &shape, QMap<vertex_index, unsigned int> vertexCache,
        const QVector<float> &in_positions,
        const QVector<float> &in_normals,
        const QVector<float> &in_texcoords,
        const QVector<QVector<vertex_index> > &faces,
        const int material_id, const QString &name, bool clearCache)
{
    if (faces.empty()) return false;

    // Flatten vertices and indices
    for (int i = 0; i < faces.size(); i++)
    {
        const QVector<vertex_index> &face = faces[i];

        vertex_index i0 = face[0];
        vertex_index i1(-1);
        vertex_index i2 = face[1];

        size_t npolys = face.size();

        // Polygon -> triangle fan conversion
        for (size_t k = 2; k < npolys; k++)
        {
            i1 = i2;
            i2 = face[k];

            unsigned int v0 = updateVertex(
                        vertexCache, shape.mesh.positions, shape.mesh.normals,
                        shape.mesh.texcoords, in_positions, in_normals, in_texcoords, i0);
            unsigned int v1 = updateVertex(
                        vertexCache, shape.mesh.positions, shape.mesh.normals,
                        shape.mesh.texcoords, in_positions, in_normals, in_texcoords, i1);
            unsigned int v2 = updateVertex(
                        vertexCache, shape.mesh.positions, shape.mesh.normals,
                        shape.mesh.texcoords, in_positions, in_normals, in_texcoords, i2);

            shape.mesh.indices.push_back(v0);
            shape.mesh.indices.push_back(v1);
            shape.mesh.indices.push_back(v2);

            shape.mesh.material_ids.push_back(material_id);
        }
    }

    shape.name = name;

    if (clearCache) vertexCache.clear();

    return true;
}

Material::Material() :
    shininess(1.0f),
    ior(1.0f),
    dissolve(1.0f),
    illum(0)
{}

bool parseFloats(QTextStream& textStream, float* floatArray, unsigned int length)
{
    float* readFloats = new float[length];
    for (unsigned int index = 0; index < length; ++index)
    {
        textStream >> readFloats[index];
    }
    if (textStream.status() != QTextStream::Ok) return false;
    {
        for (unsigned int index = 0; index < length; ++index)
        {
            floatArray[index] = readFloats[index];
        }
    }
    delete[] readFloats;
    return true;
}

void LoadObject(
        QVector<Shape>& shapes,
        QVector<Material>& materials,
        QFile& objFile)
{
    QTextStream inStream(&objFile);
    QString path = QFileInfo(objFile).absolutePath() + '/';

    QVector<float> vertices;
    QVector<float> vertexNormals;
    QVector<float> textureCoordinates;
    QVector<QVector<vertex_index> > faces;
    QString name;

    // Material
    QMap<QString, int> materialMap;
    QMap<vertex_index, unsigned int> vertexCache;
    int material = -1;

    Shape shape;

    while (!inStream.atEnd()) {
        QString line = inStream.readLine().trimmed();
        QTextStream lineStream(&line);

        if (line.isEmpty()) continue;

        // Comment line.
        if (line.startsWith("//") || line.startsWith('#')) continue;

        // Vertex normal.
        if (line.startsWith("vn"))
        {
            char vChar;
            char nChar;
            lineStream >> vChar;
            lineStream >> nChar;
            float coordinate[3];
            if (!parseFloats(lineStream, coordinate, 3)) continue;
            for (unsigned int axis = 0; axis < 3; ++axis)
            {
                vertexNormals.push_back(coordinate[axis]);
            }
            continue;
        }

        // Texture Coordinate.
        if (line.startsWith("vt"))
        {
            char vChar;
            char tChar;
            lineStream >> vChar;
            lineStream >> tChar;
            float coordinate[2];
            if (!parseFloats(lineStream, coordinate, 2)) continue;
            for (unsigned int axis = 0; axis < 2; ++axis)
            {
                textureCoordinates.push_back(coordinate[axis]);
            }
            continue;
        }

        // Vertex.
        if (line.startsWith('v'))
        {
            char vChar;
            lineStream >> vChar;
            float coordinate[3];
            if (!parseFloats(lineStream, coordinate, 3)) continue;
            for (unsigned int axis = 0; axis < 3; ++axis)
            {
                vertices.push_back(coordinate[axis]);
            }
            continue;
        }

        // Face
        if (line.startsWith('f'))
        {
            line = line.remove(0,1).trimmed();
            QStringList vertexIndexStrings = line.split(QRegExp("\\s"));
            QVector<vertex_index> face;
            for (QString vertexIndexString : vertexIndexStrings)
            {
                QStringList indexStrings = vertexIndexString.split('/');
                int vertexIndices[3] {};
                unsigned int index = 0;
                for (QString indexString : indexStrings)
                {
                    QTextStream indexStream(&indexString);
                    int thisIndex;
                    indexStream >> thisIndex;
                    vertexIndices[index++] = thisIndex;
                    if (indexStream.status() != QTextStream::Ok || index >= 3) continue;
                }
                vertex_index vertexIndex(vertexIndices[0], vertexIndices[1], vertexIndices[2]);
                face.push_back(vertexIndex);
            }
            faces.push_back(face);
            continue;
        }

        // Use material
        if (line.startsWith("usemtl"))
        {
            QString materialName = line.remove(0,6).trimmed();

            // Create face group per material.
            bool ret = exportFacesToShape(shape, vertexCache, vertices, vertexNormals, textureCoordinates,
                                              faces, material, name, true);
            if (ret) {
                faces.clear();
            }

            if (materialMap.find(materialName) != materialMap.end()) {
                material = materialMap[materialName];
            } else {
                // { error!! material not found }
                material = -1;
            }
            continue;
        }

        // Load material
        if (line.startsWith("mtllib"))
        {
            QString materialName = line.remove(0,6).trimmed();
            QFile mtlFile(path + materialName);
            if (!mtlFile.open(QIODevice::ReadOnly | QIODevice::Text))
                qDebug() << Q_FUNC_INFO << "Could not open: " << path + materialName;
            LoadMaterial(materialMap, materials, mtlFile);
            continue;
        }

        // Group name
        if (line.startsWith('g'))
        {
            // Flush previous face group.
            bool ret = exportFacesToShape(shape, vertexCache, vertices, vertexNormals, textureCoordinates,
                                              faces, material, name, true);
            if (ret) {
                shapes.push_back(shape);
            }

            shape = Shape();

            // material = -1;
            faces.clear();

            name = line.remove(0,1).trimmed();
            continue;
        }

        // Object name
        if (line.startsWith('o'))
        {
            // Flush previous face group.
            bool ret = exportFacesToShape(shape, vertexCache, vertices, vertexNormals, textureCoordinates,
                                              faces, material, name, true);
            if (ret) {
                shapes.push_back(shape);
            }

            shape = Shape();

            // material = -1;
            faces.clear();

            name = line.remove(0,1).trimmed();
            continue;
        }
    }

    bool ret = exportFacesToShape(shape, vertexCache, vertices, vertexNormals, textureCoordinates, faces,
                                      material, name, true);
    if (ret) {
        shapes.push_back(shape);
    }
    faces.clear(); // for safety
}

void LoadMaterial (
        QMap<QString, int>& materialMap,
        QVector<Material>& materials,
        QFile& mtlFile)
{
    materialMap.clear();

    Material material;

    QTextStream inStream(&mtlFile);

    while (!inStream.atEnd()) {
        QString line = inStream.readLine().trimmed();

        if (line.isEmpty()) continue;

        // Comment line.
        if (line.startsWith("//") || line.startsWith('#')) continue;

        // New material.
        if (line.startsWith("newmtl"))
        {
            // Flush previous material.
            if (!material.name.isEmpty())
            {
                materialMap.insert(material.name, materials.size());
                materials.push_back(material);
            }
            material = Material();

            // Set new material name.
            material.name = line.remove(0,6).trimmed();
            continue;
        }

        // Ambient texture
        if (line.startsWith("map_Ka"))
        {
            material.ambientTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // Diffuse texture
        if (line.startsWith("map_Kd"))
        {
            material.diffuseTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // Specular texture
        if (line.startsWith("map_Ks"))
        {
            material.specularTexture = line.remove(0, 6).trimmed();
            continue;
        }

        // Normal texture
        if (line.startsWith("map_Ns"))
        {
            material.normalTexture = line.remove(0, 6).trimmed();
            continue;
        }
        // Ambient
        if (line.startsWith("Ka"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            parseFloats(lineStream, material.ambient, 3);
            continue;
        }

        // Diffuse
        if (line.startsWith("Kd"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            parseFloats(lineStream, material.diffuse, 3);
            continue;
        }

        // Specular
        if (line.startsWith("Ks"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            parseFloats(lineStream, material.specular, 3);
            continue;
        }

        // Transmittance
        if (line.startsWith("Kt"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            parseFloats(lineStream, material.transmittance, 3);
            continue;
        }

        // Emission
        if (line.startsWith("Ke"))
        {
            line = line.remove(0, 2);
            QTextStream lineStream(&line);
            parseFloats(lineStream, material.emission, 3);
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
            float dissolve;
            lineStream >> dissolve;
            if (lineStream.status() == QTextStream::Ok)
                material.dissolve = dissolve;
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

        // Unknown parameter
        QStringList components = line.split(QRegExp("\\s"));
        if (components.length() > 1)
        material.unknownParameters.insert(components[0], components[1]);
    }

    // Flush last material.
    materialMap.insert(material.name, materials.size());
    materials.push_back(material);
}
