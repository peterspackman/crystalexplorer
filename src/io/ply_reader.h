#pragma once

#include "mesh.h"
#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <vector>
#include <fstream>
#include "tinyply.h"
#include <ankerl/unordered_dense.h>

class PlyReader {
public:
    explicit PlyReader(const QString &filepath, bool preloadIntoMemory = true);
    ~PlyReader();

    Mesh* read();
    
    // Static helper method
    static Mesh* loadFromFile(const QString &filepath, bool preloadIntoMemory = true);

private:
    bool parseFile();
    bool parseFileFromBuffer(const QByteArray& buffer);
    bool parseFileFromDisk();
    void requestProperties();
    void processVertexProperties();
    Mesh* constructMesh();
    void setMeshProperty(Mesh *mesh, const QString &displayName, 
                         const std::shared_ptr<tinyply::PlyData> &prop);

    QString m_filepath;
    bool m_preloadIntoMemory;
    std::unique_ptr<tinyply::PlyFile> m_plyFile;
    
    // PLY data components
    std::shared_ptr<tinyply::PlyData> m_vertices;
    std::shared_ptr<tinyply::PlyData> m_faces;
    std::shared_ptr<tinyply::PlyData> m_normals;
    ankerl::unordered_dense::map<std::string, std::shared_ptr<tinyply::PlyData>> m_properties;
};
