#include "mesh.h"

Mesh::Mesh(QString dir, QString name) :
    mesh_path(name), vertexArray(0)
{
    qDebug() << Q_FUNC_INFO << "Loading new mesh:" << dir + mesh_path ;

    mesh_log = QString("");
    bLoaded = false;

    std::string inputfile = (dir + mesh_path).toStdString();
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err = tinyobj::LoadObj(shapes, materials, inputfile.c_str());

    if (!err.empty())
    {
        qDebug() << Q_FUNC_INFO << "Loading mesh file failed:" << dir + mesh_path ;
        mesh_log += "Loading mesh file failed:" + dir + mesh_path + "\n";
        return;
    }

    if(shapes.size() == 0)
    {
        qDebug() << "Woops:: This model has no shapes, so it cannot be loaded." ;
        mesh_log += "Woops:: This model has no shapes, so it cannot be loaded.\n";
        return;
    }

    centre_of_mass = QVector3D(0, 0, 0);
    int max_wrong_shapes = 10;
    int no_wrong_shapes  = 0;

    //qDebug() << "List of problematic shapes:";
    //mesh_log += "List of problematic shapes:\n";

    for (size_t i = 0; i < shapes.size(); i++)
    {
        bool problemWith[3] = {false, false, false};
        if(shapes[i].mesh.texcoords.size() == 0)
        {
            problemWith[0] = true;
        }
        if(shapes[i].mesh.normals.size() == 0)
        {
            problemWith[1] = true;
        }
        if(shapes[i].mesh.positions.size() == 0)
        {
            problemWith[2] = true;
        }

        // Check shapes.
        bool shapeTest = problemWith[0] || problemWith[1] || problemWith[2];
        if(shapeTest)
        {
            if(no_wrong_shapes < max_wrong_shapes)
            {
                QString doesNotHave =
                        QString(problemWith[0] ? " UVs " : "") +
                        QString(problemWith[0] ? " normals " : "") +
                        QString(problemWith[0] ? " positions " : "");
                QString message = "[" +
                        QString::number(i+1) + "] " +
                        QString(shapes[i].name.c_str()) +
                        " has no: " + doesNotHave;
                qDebug() << message;
                mesh_log += message + "\n";
            }
            no_wrong_shapes++;
            continue;
        }

        for (size_t f = 0; f < shapes[i].mesh.indices.size() / 3; f++)
        {
            for(size_t d = 0 ; d < 3 ; d++)
            {
                unsigned int index = shapes[i].mesh.indices[3*f+d];
                QVector3D pos(
                        shapes[i].mesh.positions[3 * index + 0],
                        shapes[i].mesh.positions[3 * index + 1],
                        shapes[i].mesh.positions[3 * index + 2]);
                gl_vertices.push_back(pos);
                centre_of_mass += pos;

                QVector3D uv(
                        shapes[i].mesh.texcoords[2 * index + 0],
                        shapes[i].mesh.texcoords[2 * index + 1],
                        0);
                gl_texcoords.push_back(uv);

                QVector3D normal(
                        shapes[i].mesh.normals[3 * index + 0],
                        shapes[i].mesh.normals[3 * index + 1],
                        shapes[i].mesh.normals[3 * index + 2]);

                normal.normalize();
                gl_smoothed_normals.push_back(normal);
                gl_normals.push_back(normal);
            } // End of for face loop.
        } // End of shape indices loop.
    } // End of for shape loop.

    if(gl_vertices.size() == 0 || gl_normals.size() == 0 || gl_texcoords.size() == 0)
    {
        if(no_wrong_shapes >= max_wrong_shapes)
        {
            mesh_log += "Total number of problematic shapes is:" +
                    QString::number(no_wrong_shapes) +
                    ". Listed only first ten of them.\n" ;
        }
        qDebug() << "Conclusion: Mesh has no vertices or normals or UVs, so it cannot be loaded.";
        mesh_log += "Conclusion: Mesh has no vertices or normals or UVs, so it cannot be loaded.\n";
        return;
    }

    centre_of_mass /= gl_vertices.size();
    radius = 0;
    for(int i = 0 ; i < gl_vertices.size() ; i++ )
    {
        float dist = QVector3D(centre_of_mass - gl_vertices[i]).length();
        if(dist > radius) radius = dist;
    }

    //qDebug() << "mesh radius=" << radius;
    //qDebug() << "mesh center=" << centre_of_mass;
    calculateTangents();

    QVector3D minPos, maxPos;
    for(int i = 0; i < gl_vertices.size() ; i++)
    {
        if(minPos.x() > gl_vertices[i].x())
            minPos.setX(gl_vertices[i].x());
        if(minPos.y() > gl_vertices[i].y())
            minPos.setY(gl_vertices[i].y());
        if(minPos.z() > gl_vertices[i].z())
            minPos.setZ(gl_vertices[i].z());

        if(maxPos.x() < gl_vertices[i].x())
            maxPos.setX(gl_vertices[i].x());
        if(maxPos.y() < gl_vertices[i].y())
            maxPos.setY(gl_vertices[i].y());
        if(maxPos.z() < gl_vertices[i].z())
            maxPos.setZ(gl_vertices[i].z());
    }

    int vsize = 100 * 100 * 100;
    std::vector<int> *verlet = new std::vector<int>[vsize];

    for(int i = 0; i < gl_vertices.size() ; i++)
    {
        QVector3D pos = (gl_vertices[i] - minPos) / (maxPos-minPos).length() * 100;
        int ii = pos.x();
        int jj = pos.y();
        int kk = pos.z();
        verlet[( ii * 100 * 100 + jj * 100 + kk )].push_back(i);
    }

    int sum = 0;
    for(int i = 0; i < vsize ; i++)
    {
        sum += verlet[i].size();
    }

    for(int i = 0; i < vsize ; i++)
    {
        for(unsigned long k = 0 ; k < verlet[i].size() ; k++)
        {
            QVector3D smoothed;
            int ki = verlet[i][k];
            for(unsigned long l = 0 ; l < verlet[i].size() ; l++)
            {
                int li = verlet[i][l];
                if( (gl_vertices[ki] - gl_vertices[li]).length() / radius < 1.0E-5 )
                {
                    smoothed += gl_normals[li];
                }
            }
            smoothed.normalize();
            gl_smoothed_normals[ki] = smoothed;
        }
    }

    delete[] verlet;

    bLoaded = true;
    initializeMesh();
}

Mesh::~Mesh()
{
    qDebug() << Q_FUNC_INFO << " destroying mesh:" << mesh_path;
    gl_vertices  .clear();
    gl_normals   .clear();
    gl_texcoords .clear();
    gl_tangents  .clear();
    gl_bitangents.clear();
    gl_smoothed_normals.clear();

    if(vertexArray) delete vertexArray;
}

void Mesh::initializeMesh()
{
    //static bool bOneTime = true;
    //if(!bOneTime) return;
    //bOneTime = false;

    if(bLoaded == false)
    {
        qDebug() << Q_FUNC_INFO << " Cannot initialize mesh because is NULL.";
        return;
    }

    initializeOpenGLFunctions();

    vertexArray = new QOpenGLVertexArrayObject();
    vertexArray->create();
    vertexArray->bind();

    QOpenGLBuffer vertexBuffer(QOpenGLBuffer::VertexBuffer);
    vertexBuffer.create();
    vertexBuffer.bind();
    vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    vertexBuffer.allocate(gl_vertices.constData(), sizeof(QVector3D) * gl_vertices.size());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(0);

    QOpenGLBuffer textureCoordBuffer(QOpenGLBuffer::VertexBuffer);
    textureCoordBuffer.create();
    textureCoordBuffer.bind();
    textureCoordBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    textureCoordBuffer.allocate(gl_texcoords.constData(), sizeof(QVector3D) * gl_texcoords.size());
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(1);

    QOpenGLBuffer normalsBuffer(QOpenGLBuffer::VertexBuffer);
    normalsBuffer.create();
    normalsBuffer.bind();
    normalsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    normalsBuffer.allocate(gl_normals.constData(), sizeof(QVector3D) * gl_normals.size());
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(2);

    QOpenGLBuffer tangetsBuffer(QOpenGLBuffer::VertexBuffer);
    tangetsBuffer.create();
    tangetsBuffer.bind();
    tangetsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    tangetsBuffer.allocate(gl_tangents.constData(), sizeof(QVector3D) * gl_tangents.size());
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(3);

    QOpenGLBuffer bitangentsBuffer(QOpenGLBuffer::VertexBuffer);
    bitangentsBuffer.create();
    bitangentsBuffer.bind();
    bitangentsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    bitangentsBuffer.allocate(gl_bitangents.constData(), sizeof(QVector3D) * gl_bitangents.size());
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(4);

    QOpenGLBuffer smoothedNormalsBuffer(QOpenGLBuffer::VertexBuffer);
    smoothedNormalsBuffer.create();
    smoothedNormalsBuffer.bind();
    smoothedNormalsBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
    smoothedNormalsBuffer.allocate(gl_smoothed_normals.constData(), sizeof(QVector3D) * gl_smoothed_normals.size());
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, sizeof(QVector3D), (void*)0);
    glEnableVertexAttribArray(5);

    vertexArray->release();
}

void Mesh::drawMesh(bool bUseArrays)
{
    if(bLoaded == false) return;

    vertexArray->bind();

    if(bUseArrays)
    {
        GLCHK( glDrawArrays(GL_TRIANGLES, 0,  gl_vertices.size()) );
    }
    else
    {
#ifdef USE_OPENGL_330
        GLCHK( glDrawArrays(GL_TRIANGLES, 0,  gl_vertices.size()) );
#else
        // Tell OpenGL that every patch has 16 verts.
        GLCHK( glPatchParameteri(GL_PATCH_VERTICES, 3) );
        // Draw a bunch of patches.
        GLCHK( glDrawArrays(GL_PATCHES, 0, gl_vertices.size()) );
#endif
    }

    vertexArray->release();
}

bool Mesh::isLoaded()
{
    return bLoaded;
}

QString& Mesh::getMeshLog()
{
    return mesh_log;
}

// Calculation based on article:
// Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”.
// Terathon Software 3D Graphics Library, 2001. http://www.terathon.com/code/tangent.html
void Mesh::calculateTangents()
{
    int vertexCount = gl_vertices.size();

    QVector3D *tan1 = new QVector3D[vertexCount * 2];
    QVector3D *tan2 = tan1 + vertexCount;

    for(int i = 0; i < 2*vertexCount ; i++)
    {
        tan1[i] = QVector3D(0.0f,0.0f,0.0f);
    }
    //ZeroMemory(tan1, vertexCount * sizeof(Vector3D) * 2);

    for (int a = 0; a < vertexCount/3; a++)
    {
        long i1 = 3*a+0;
        long i2 = 3*a+1;
        long i3 = 3*a+2;

        const QVector3D& v1 = gl_vertices[i1];
        const QVector3D& v2 = gl_vertices[i2];
        const QVector3D& v3 = gl_vertices[i3];

        const QVector3D& w1 = gl_texcoords[i1];
        const QVector3D& w2 = gl_texcoords[i2];
        const QVector3D& w3 = gl_texcoords[i3];

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
        QVector3D& n = gl_normals[a];
        QVector3D& t = tan1[a];
        t.normalize();
        // Gram-Schmidt orthogonalize
        QVector3D tangent = t - n * QVector3D::dotProduct(n,t);
        tangent.normalize();
        // Calculate handedness
        float handedness =(QVector3D::dotProduct(QVector3D::crossProduct(n, t), tan2[a]) < 0.0F) ? -1.0F : 1.0F;
        gl_tangents.push_back(tangent);
        QVector3D bitangent = QVector3D::crossProduct(n, t) * handedness;
        bitangent.normalize();
        gl_bitangents.push_back(bitangent);
    }
    delete[] tan1;
}

/*
bool Mesh::hasCommonEdge(int i, int j)
{
    // Triangle A ID.
    int ta = i/3;
    // Triangle B ID.
    int tb = j/3;
    if(i > j) return false;
    if(ta > tb) return false;
    if(ta == tb) return false;

    // Local index in triangle A.
    int la = i - 3*ta;
    int lb = j - 3*tb;

    for(int li = 0; li < 3 ; li++ )
    {
        if( li != la)
        {
            for(int lj = 0; lj < 3 ; lj++)
            {
                if( lj != lb )
                {
                    // Check if two points are same.
                    if( (gl_vertices[3 * ta + li] - gl_vertices[3 * tb + lj]).length() / radius < 1.0E-5 )
                    {
                        // Both triangles have a common edge.
                        qDebug() << "i =" << i << "\tj =" << j;
                        qDebug() << "ta=" << ta << "\ttb=" << tb;
                        qDebug() << "la=" << la << "\tlb=" << lb ;
                        qDebug() << "ia=" << 3 * ta + li << "\tjb=" << 3 * tb + lj << endl;
                        if(no_shared_edges[j] > 1 || no_shared_edges[i] > 1 )
                        {
                            qDebug() << "i =" << i << "\tj =" << j;
                            qDebug() << "ia=" << 3 * ta + li << "\tjb=" << 3 * tb + lj << endl;
                        }
                        if(no_shared_edges[i] == 0)
                        {
                            no_shared_edges[i]++;
                            gl_shared_uvs[i].setX(gl_texcoords[j].x());
                            gl_shared_uvs[i].setY(gl_texcoords[j].y());
                        }
                        else if(no_shared_edges[i] == 1)
                        {
                            no_shared_edges[i]++;
                            gl_shared_uvs[i].setZ(gl_texcoords[j].x());
                            gl_shared_uvs[i].setW(gl_texcoords[j].y());
                        }
                        if(no_shared_edges[j] == 0)
                        {
                            no_shared_edges[j]++;
                            gl_shared_uvs[j].setX(gl_texcoords[i].x());
                            gl_shared_uvs[j].setY(gl_texcoords[i].y());
                        }
                        else if(no_shared_edges[j] == 1)
                        {
                            no_shared_edges[j]++;
                            gl_shared_uvs[j].setZ(gl_texcoords[i].x());
                            gl_shared_uvs[j].setW(gl_texcoords[i].y());
                        }

                        return true;
                    }
                }
            }
        } // End of if li.
    } // End of for li loop.
    //qDebug() << "" ;
    return false;
}
*/
