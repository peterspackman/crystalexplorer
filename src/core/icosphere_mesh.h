#pragma once
#include "mesh.h"
#include <occ/core/linear_algebra.h>

class IcosphereMesh {
public:
    // Generate a basic icosphere mesh with given subdivisions and radius
    static Mesh* create(int subdivisions = 2, double radius = 1.0, QObject *parent = nullptr);
    
    // Generate icosphere vertices (unit sphere)
    static Mesh::VertexList generateVertices(int subdivisions = 2);
    
    // Generate icosphere faces
    static Mesh::FaceList generateFaces(int subdivisions = 2);
    
private:
    // Helper to subdivide triangles
    static void subdivideTriangle(const occ::Vec3 &v1, const occ::Vec3 &v2, const occ::Vec3 &v3,
                                  int depth, std::vector<occ::Vec3> &vertices);
};