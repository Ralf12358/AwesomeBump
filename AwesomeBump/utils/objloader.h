/*
 * Based on original Tiny Obj Loader:
 * Copyright 2012-2015, Syoyo Fujita.
 * Licensed under 2-clause BSD liecense.
 */

#ifndef OBJLOADER_H
#define OBJLOADER_H

#include <QFile>
#include <QMap>
#include <QString>
#include <QTextStream>
#include <QVector>

class Material
{
public:
    Material();

    QString name;
    QString ambientTexture;
    QString diffuseTexture;
    QString specularTexture;
    QString normalTexture;

    float ambient[3]{};
    float diffuse[3]{};
    float specular[3]{};
    float transmittance[3]{};
    float emission[3]{};
    float shininess;
    // Index of refraction
    float ior;
    // 0: Fully transparent - 1: Opaque
    float dissolve;
    // Illumination model (see http://www.fileformat.info/format/material/)
    int illum;

    QMap<QString, QString> unknownParameters;
};

typedef struct
{
    QVector<float>          positions;
    QVector<float>          normals;
    QVector<float>          texcoords;
    QVector<unsigned int>   indices;
    QVector<int>            material_ids;
} MeshStruc;

typedef struct
{
    QString name;
    MeshStruc mesh;
} Shape;

class MaterialReader
{
public:
    MaterialReader(){}
    virtual ~MaterialReader(){}

    virtual QString operator() (
            const QString& matId,
            QVector<Material>& materials,
            QMap<QString, int>& matMap) = 0;
};

class MaterialFileReader:
        public MaterialReader
{
public:
    MaterialFileReader(const QString& mtl_basepath): m_mtlBasePath(mtl_basepath) {}
    virtual ~MaterialFileReader() {}
    virtual QString operator() (
            const QString& matId,
            QVector<Material>& materials,
            QMap<QString, int>& matMap);

private:
    QString m_mtlBasePath;
};

/*
 * Loads shapes and materials from an .obj and .mtl file.
 */
void LoadObject(
        QVector<Shape>& shapes,
        QVector<Material>& materials,
        QFile& objFile);

/*
 * Loads materials from an .mtl file.
 */
void LoadMaterial (
        QMap<QString, int>& materialMap,
        QVector<Material>& materials,
        QFile& mtlFile);

#endif // OBJLOADER_H
