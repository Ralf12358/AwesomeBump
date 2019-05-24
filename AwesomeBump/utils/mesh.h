#ifndef MESH_H
#define	MESH_H

#include <QOpenGLFunctions_4_0_Core>
#include <QtOpenGL>
#include <QString>
#include <QDebug>
#include <QVector>
#include <QVector3D>
#include <QOpenGLVertexArrayObject>
#include <iostream>
#include "../openglerrorcheck.h"
#include "tinyobj/tiny_obj_loader.h"

class Mesh : public QOpenGLFunctions_4_0_Core
{
public:
    Mesh(QString dir, QString name);
    virtual ~Mesh();

    void initializeMesh();
    void drawMesh(bool bUseArrays = false);
    bool isLoaded();
    QString& getMeshLog();

    // Helps to aling mesh to the center of the screen.
    QVector3D centre_of_mass;
    // Maxiumum distance from centre of mass to some vertex.
    float radius;

private:       
    void calculateTangents();
    //bool hasCommonEdge(int i, int j);

    QString mesh_path;
    bool bLoaded;

    // Arrays
    QVector<QVector3D> gl_vertices;
    QVector<QVector3D> gl_normals;
    QVector<QVector3D> gl_smoothed_normals;
    QVector<QVector3D> gl_texcoords;
    QVector<QVector3D> gl_tangents;
    QVector<QVector3D> gl_bitangents;

    // Vertex Array Object
    QOpenGLVertexArrayObject *vertexArray;

    QString mesh_log;
};

#endif	// MESH_H
