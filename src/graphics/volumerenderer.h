#pragma once

#include "renderer.h"
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <vector>

class VolumeRenderer : public IndexedRenderer, protected QOpenGLFunctions {
public:
    VolumeRenderer();
    virtual ~VolumeRenderer();

    void setVolumeData(const std::vector<float>& data, int width, int height, int depth);
    void setGridVectors(const QVector3D& vec1, const QVector3D& vec2, const QVector3D& vec3);
    void setTransferFunction(const std::vector<QVector4D>& transferFunction);

    virtual void beginUpdates();
    virtual void endUpdates();
    virtual void draw();
    void clear();

private:
    void initializeGL();
    void updateBuffers();
    void createGeometry();

    QOpenGLBuffer m_vertex;
    QOpenGLTexture* m_volumeTexture;
    QOpenGLTexture* m_transferFunctionTexture;

    std::vector<QVector3D> m_vertices;
    std::vector<GLuint> m_indices;

    int m_volumeWidth;
    int m_volumeHeight;
    int m_volumeDepth;

    QVector3D m_gridVec1;
    QVector3D m_gridVec2;
    QVector3D m_gridVec3;

    bool m_updatesDisabled;
    bool m_initialized;

    // Uniform locations
    int m_volumeTextureLoc;
    int m_transferFunctionLoc;
    int m_volumeDimensionsLoc;
    int m_gridVec1Loc;
    int m_gridVec2Loc;
    int m_gridVec3Loc;
};
