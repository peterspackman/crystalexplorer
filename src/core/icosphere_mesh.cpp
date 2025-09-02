#include "icosphere_mesh.h"
#include <QDebug>
#include <cmath>
#include <unordered_map>

Mesh* IcosphereMesh::create(int subdivisions, double radius, QObject *parent) {
    // Validate inputs
    if (subdivisions < 0 || subdivisions > 7) {
        qDebug() << "Invalid subdivisions for icosphere:" << subdivisions;
        return nullptr;
    }
    if (radius <= 0.0) {
        qDebug() << "Invalid radius for icosphere:" << radius;
        return nullptr;
    }
    
    // Generate vertices and faces
    auto vertices = generateVertices(subdivisions);
    auto faces = generateFaces(subdivisions);
    
    // Scale vertices by radius
    vertices *= radius;
    
    // Create mesh
    Mesh *mesh = new Mesh(vertices, faces, parent);
    
    // Set object name and description
    mesh->setObjectName(QString("Icosphere (subdiv=%1, r=%2)").arg(subdivisions).arg(radius));
    mesh->setDescription(QString("Icosphere with %1 subdivisions and radius %2").arg(subdivisions).arg(radius));
    
    // Add default "None" property
    mesh->setVertexProperty("None", Eigen::VectorXf::Zero(vertices.cols()));
    
    // For an icosphere, the normals are just the normalized vertex positions
    // Since vertices are on a sphere, the normal at each vertex points radially outward
    Mesh::VertexList normals(3, vertices.cols());
    for (int i = 0; i < vertices.cols(); ++i) {
        normals.col(i) = vertices.col(i).normalized();
    }
    mesh->setVertexNormals(normals);
    
    qDebug() << "Created icosphere mesh with" << vertices.cols() << "vertices and" << faces.cols() << "faces";
    
    return mesh;
}

void IcosphereMesh::subdivideTriangle(const occ::Vec3 &v1, const occ::Vec3 &v2, const occ::Vec3 &v3,
                                      int depth, std::vector<occ::Vec3> &vertices) {
    if (depth == 0) {
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        return;
    }

    // Create midpoints and normalize to unit sphere
    occ::Vec3 v12 = (v1 + v2).normalized();
    occ::Vec3 v23 = (v2 + v3).normalized();
    occ::Vec3 v31 = (v3 + v1).normalized();

    // Recursively subdivide
    subdivideTriangle(v1, v12, v31, depth - 1, vertices);
    subdivideTriangle(v2, v23, v12, depth - 1, vertices);
    subdivideTriangle(v3, v31, v23, depth - 1, vertices);
    subdivideTriangle(v12, v23, v31, depth - 1, vertices);
}

Mesh::VertexList IcosphereMesh::generateVertices(int subdivisions) {
    std::vector<occ::Vec3> vertices;
    
    // Golden ratio
    const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
    
    // Initial icosahedron vertices (12 vertices)
    std::vector<occ::Vec3> ico_vertices = {
        occ::Vec3(-1, phi, 0).normalized(),
        occ::Vec3(1, phi, 0).normalized(),
        occ::Vec3(-1, -phi, 0).normalized(),
        occ::Vec3(1, -phi, 0).normalized(),
        
        occ::Vec3(0, -1, phi).normalized(),
        occ::Vec3(0, 1, phi).normalized(),
        occ::Vec3(0, -1, -phi).normalized(),
        occ::Vec3(0, 1, -phi).normalized(),
        
        occ::Vec3(phi, 0, -1).normalized(),
        occ::Vec3(phi, 0, 1).normalized(),
        occ::Vec3(-phi, 0, -1).normalized(),
        occ::Vec3(-phi, 0, 1).normalized()
    };
    
    // Initial icosahedron faces (20 faces)
    std::vector<std::array<int, 3>> ico_faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };
    
    // Subdivide each face
    for (const auto& face : ico_faces) {
        subdivideTriangle(ico_vertices[face[0]], ico_vertices[face[1]], ico_vertices[face[2]], 
                         subdivisions, vertices);
    }
    
    // Remove duplicates and create vertex matrix
    std::unordered_map<std::string, int> unique_vertices;
    std::vector<occ::Vec3> final_vertices;
    
    for (const auto& v : vertices) {
        std::string key = std::to_string(v(0)) + "," + std::to_string(v(1)) + "," + std::to_string(v(2));
        if (unique_vertices.find(key) == unique_vertices.end()) {
            unique_vertices[key] = final_vertices.size();
            final_vertices.push_back(v);
        }
    }
    
    // Convert to Eigen matrix
    Mesh::VertexList vertex_matrix(3, final_vertices.size());
    for (size_t i = 0; i < final_vertices.size(); ++i) {
        vertex_matrix.col(i) = final_vertices[i];
    }
    
    return vertex_matrix;
}

Mesh::FaceList IcosphereMesh::generateFaces(int subdivisions) {
    std::vector<occ::Vec3> vertices;
    
    // Golden ratio
    const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
    
    // Initial icosahedron vertices
    std::vector<occ::Vec3> ico_vertices = {
        occ::Vec3(-1, phi, 0).normalized(),
        occ::Vec3(1, phi, 0).normalized(),
        occ::Vec3(-1, -phi, 0).normalized(),
        occ::Vec3(1, -phi, 0).normalized(),
        
        occ::Vec3(0, -1, phi).normalized(),
        occ::Vec3(0, 1, phi).normalized(),
        occ::Vec3(0, -1, -phi).normalized(),
        occ::Vec3(0, 1, -phi).normalized(),
        
        occ::Vec3(phi, 0, -1).normalized(),
        occ::Vec3(phi, 0, 1).normalized(),
        occ::Vec3(-phi, 0, -1).normalized(),
        occ::Vec3(-phi, 0, 1).normalized()
    };
    
    // Initial icosahedron faces
    std::vector<std::array<int, 3>> ico_faces = {
        {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
        {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
        {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
        {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
    };
    
    // Subdivide to get all vertices
    for (const auto& face : ico_faces) {
        subdivideTriangle(ico_vertices[face[0]], ico_vertices[face[1]], ico_vertices[face[2]], 
                         subdivisions, vertices);
    }
    
    // Create vertex index map
    std::unordered_map<std::string, int> vertex_indices;
    std::vector<occ::Vec3> unique_vertices;
    
    for (const auto& v : vertices) {
        std::string key = std::to_string(v(0)) + "," + std::to_string(v(1)) + "," + std::to_string(v(2));
        if (vertex_indices.find(key) == vertex_indices.end()) {
            vertex_indices[key] = unique_vertices.size();
            unique_vertices.push_back(v);
        }
    }
    
    // Create faces (each triangle from subdivision)
    std::vector<std::array<int, 3>> faces;
    for (size_t i = 0; i < vertices.size(); i += 3) {
        std::string key1 = std::to_string(vertices[i](0)) + "," + std::to_string(vertices[i](1)) + "," + std::to_string(vertices[i](2));
        std::string key2 = std::to_string(vertices[i+1](0)) + "," + std::to_string(vertices[i+1](1)) + "," + std::to_string(vertices[i+1](2));
        std::string key3 = std::to_string(vertices[i+2](0)) + "," + std::to_string(vertices[i+2](1)) + "," + std::to_string(vertices[i+2](2));
        
        faces.push_back({vertex_indices[key1], vertex_indices[key2], vertex_indices[key3]});
    }
    
    // Convert to Eigen matrix
    Mesh::FaceList face_matrix(3, faces.size());
    for (size_t i = 0; i < faces.size(); ++i) {
        face_matrix.col(i) = Eigen::Vector3i(faces[i][0], faces[i][1], faces[i][2]);
    }
    
    return face_matrix;
}