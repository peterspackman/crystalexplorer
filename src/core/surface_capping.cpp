#include "surface_capping.h"
#include "chemicalstructure.h"
#include <QDebug>
#include <algorithm>
#include <ankerl/unordered_dense.h>

Mesh* SurfaceCapping::applyCapping(const Mesh* mesh, 
                                  const ChemicalStructure* structure,
                                  const CappingOptions& options) {
    if (!mesh) {
        qDebug() << "SurfaceCapping: Invalid mesh parameter";
        return nullptr;
    }

    if (options.method == Method::None) {
        // Return copy of original mesh
        auto* result = new Mesh(mesh->vertices(), mesh->faces(), mesh->parent());
        result->setObjectName(mesh->objectName());
        result->setDescription(mesh->description() + " (No Capping)");
        return result;
    }

    switch (options.method) {
        case Method::PlaneCut:
            return applyPlaneClipping(mesh, structure, options);
        case Method::BoundaryFan:
            return applyBoundaryFan(mesh, structure, options);
        case Method::DelaunayTriangulation:
            qWarning() << "Delaunay triangulation not yet implemented, falling back to plane clipping";
            return applyPlaneClipping(mesh, structure, options);
        default:
            return nullptr;
    }
}

bool SurfaceCapping::needsCapping(const Mesh* mesh, const ChemicalStructure* structure) {
    if (!mesh || mesh->numberOfVertices() == 0) {
        return false;
    }

    // Get unit cell bounds in fractional coordinates
    const auto& vertices = mesh->vertices();
    
    // Convert vertices to fractional coordinates
    // Note: This assumes the mesh is in Cartesian coordinates
    // and we need to convert to fractional to check unit cell bounds
    
    for (int i = 0; i < vertices.cols(); ++i) {
        Eigen::Vector3d vertex = vertices.col(i);
        // Convert to fractional coordinates (simplified - would need actual unit cell conversion)
        // For now, check if any vertex has coordinates outside [0,1] range
        // This is a simplified check - real implementation would use structure->toFractionalCoordinates
        
        if (vertex.x() < -0.1 || vertex.x() > 1.1 ||
            vertex.y() < -0.1 || vertex.y() > 1.1 ||
            vertex.z() < -0.1 || vertex.z() > 1.1) {
            return true; // Mesh extends beyond reasonable unit cell bounds
        }
    }
    
    return false;
}

SurfaceCapping::CappingOptions SurfaceCapping::getVoidSurfaceDefaults() {
    CappingOptions options;
    options.method = Method::PlaneCut;
    options.boundaryMode = BoundaryMode::UnitCell;
    options.capXMin = options.capXMax = true;
    options.capYMin = options.capYMax = true;
    options.capZMin = options.capZMax = true;
    options.tolerance = 1e-6;
    options.removeDegenerate = true;
    options.smoothNormals = true;
    return options;
}

Mesh* SurfaceCapping::applyPlaneClipping(const Mesh* mesh,
                                       const ChemicalStructure* structure,
                                       const CappingOptions& options) {
    qDebug() << "Applying plane clipping to mesh with" << mesh->numberOfVertices() << "vertices";

    // Get clipping planes
    auto planes = getUnitCellPlanes(structure, options);
    if (planes.empty()) {
        qWarning() << "No clipping planes defined";
        return nullptr;
    }

    // Start with original mesh data
    std::vector<Eigen::Vector3d> vertices;
    std::vector<Eigen::Vector3i> faces;

    // Convert mesh to working format
    const auto& origVertices = mesh->vertices();
    const auto& origFaces = mesh->faces();

    vertices.reserve(origVertices.cols() * 2); // Reserve extra space for new vertices
    for (int i = 0; i < origVertices.cols(); ++i) {
        vertices.push_back(origVertices.col(i));
    }

    faces.reserve(origFaces.cols() * 2);
    for (int i = 0; i < origFaces.cols(); ++i) {
        faces.push_back(origFaces.col(i));
    }

    // Apply Sutherland-Hodgman clipping for each plane
    for (const auto& plane : planes) {
        std::vector<Eigen::Vector3d> newVertices;
        std::vector<Eigen::Vector3i> newFaces;

        // Clip each triangle against this plane
        for (const auto& face : faces) {
            std::vector<Eigen::Vector3d> triangle = {
                vertices[face.x()],
                vertices[face.y()], 
                vertices[face.z()]
            };

            auto clippedVertices = clipTriangleAgainstPlane(triangle, plane, options.tolerance);
            
            if (clippedVertices.size() >= 3) {
                // Add clipped vertices and create new faces
                int startIdx = newVertices.size();
                for (const auto& vertex : clippedVertices) {
                    newVertices.push_back(vertex);
                }

                // Triangulate the clipped polygon (simple fan triangulation)
                for (size_t i = 1; i < clippedVertices.size() - 1; ++i) {
                    newFaces.push_back(Eigen::Vector3i(startIdx, startIdx + i, startIdx + i + 1));
                }
            }
        }

        vertices = std::move(newVertices);
        faces = std::move(newFaces);

        if (vertices.empty()) {
            qWarning() << "Mesh completely clipped away";
            return nullptr;
        }
    }

    // Convert back to Mesh format
    Mesh::VertexList finalVertices(3, vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        finalVertices.col(i) = vertices[i];
    }

    Mesh::FaceList finalFaces(3, faces.size());
    for (size_t i = 0; i < faces.size(); ++i) {
        finalFaces.col(i) = faces[i];
    }

    // Create result mesh
    auto* result = new Mesh(finalVertices, finalFaces, mesh->parent());
    result->setObjectName(mesh->objectName());
    result->setDescription(mesh->description() + " (Plane Clipped)");
    
    // Copy mesh attributes (isovalue, kind, etc.)
    result->setAttributes(mesh->attributes());
    
    // Compute and set vertex normals for the capped mesh
    auto normals = result->computeVertexNormals(Mesh::NormalSetting::Average);
    result->setVertexNormals(normals);

    // Copy properties (simplified)
    const auto& origProperties = mesh->vertexProperties();
    for (const auto& [name, values] : origProperties) {
        if (values.size() == mesh->numberOfVertices()) {
            // For now, skip property interpolation - would need to track vertex correspondences
            qDebug() << "Skipping property" << name << "- interpolation not yet implemented";
        }
    }

    qDebug() << "Clipping complete:" << result->numberOfVertices() << "vertices," 
             << result->numberOfFaces() << "faces";

    return result;
}

Mesh* SurfaceCapping::applyBoundaryFan(const Mesh* mesh,
                                     const ChemicalStructure* structure,
                                     const CappingOptions& options) {
    qWarning() << "Boundary fan capping not yet fully implemented";
    return applyPlaneClipping(mesh, structure, options); // Fallback for now
}

std::vector<Eigen::Vector4d> SurfaceCapping::getUnitCellPlanes(const ChemicalStructure* structure,
                                                             const CappingOptions& options) {
    std::vector<Eigen::Vector4d> planes;

    if (options.boundaryMode != BoundaryMode::UnitCell) {
        qWarning() << "Only unit cell boundary mode currently supported";
        return planes;
    }

    // Get unit cell vectors - columns are a, b, c vectors
    occ::Mat3 cellVec = occ::Mat3::Identity();
    if (structure) {
        cellVec = structure->cellVectors();
    }
    
    // For axis-aligned unit cells, we can use simple planes
    // Plane equation: nx*x + ny*y + nz*z + d = 0
    // A point is inside if nx*x + ny*y + nz*z + d >= 0
    
    // Get the unit cell dimensions (assuming orthogonal for now)
    double xMax = cellVec.col(0).norm();
    double yMax = cellVec.col(1).norm();
    double zMax = cellVec.col(2).norm();

    if (options.capXMin) {
        // Plane at x=0 with normal pointing inward (+x direction)
        planes.push_back(Eigen::Vector4d(1, 0, 0, 0));  // x >= 0
    }
    if (options.capXMax) {
        // Plane at x=xMax with normal pointing inward (-x direction)
        planes.push_back(Eigen::Vector4d(-1, 0, 0, xMax));  // x <= xMax
    }
    if (options.capYMin) {
        // Plane at y=0 with normal pointing inward (+y direction)
        planes.push_back(Eigen::Vector4d(0, 1, 0, 0));  // y >= 0
    }
    if (options.capYMax) {
        // Plane at y=yMax with normal pointing inward (-y direction)
        planes.push_back(Eigen::Vector4d(0, -1, 0, yMax));  // y <= yMax
    }
    if (options.capZMin) {
        // Plane at z=0 with normal pointing inward (+z direction)
        planes.push_back(Eigen::Vector4d(0, 0, 1, 0));  // z >= 0
    }
    if (options.capZMax) {
        // Plane at z=zMax with normal pointing inward (-z direction)
        planes.push_back(Eigen::Vector4d(0, 0, -1, zMax));  // z <= zMax
    }

    qDebug() << "Generated" << planes.size() << "clipping planes for unit cell:" 
             << xMax << "x" << yMax << "x" << zMax;
    return planes;
}

std::vector<Eigen::Vector3d> SurfaceCapping::clipTriangleAgainstPlane(
    const std::vector<Eigen::Vector3d>& triangle,
    const Eigen::Vector4d& plane,
    double tolerance) {
    
    if (triangle.size() != 3) {
        return {};
    }

    std::vector<Eigen::Vector3d> result;
    
    // Sutherland-Hodgman clipping algorithm
    std::vector<Eigen::Vector3d> input = triangle;
    std::vector<Eigen::Vector3d> output;

    for (size_t i = 0; i < input.size(); ++i) {
        const auto& current = input[i];
        const auto& previous = input[(i + input.size() - 1) % input.size()];

        // Calculate signed distances to plane
        double currentDist = plane.head<3>().dot(current) + plane.w();
        double previousDist = plane.head<3>().dot(previous) + plane.w();

        if (currentDist >= -tolerance) {  // Current point is inside
            if (previousDist < -tolerance) {  // Previous was outside
                // Find intersection point
                double t = previousDist / (previousDist - currentDist);
                Eigen::Vector3d intersection = previous + t * (current - previous);
                output.push_back(intersection);
            }
            output.push_back(current);
        } else if (previousDist >= -tolerance) {  // Current outside, previous inside
            // Find intersection point
            double t = previousDist / (previousDist - currentDist);
            Eigen::Vector3d intersection = previous + t * (current - previous);
            output.push_back(intersection);
        }
        // If both outside, add nothing
    }

    return output;
}

// Custom hash for edge pairs
struct EdgeHash {
    std::size_t operator()(const std::pair<int, int>& edge) const {
        // Simple hash combination for integer pairs
        return std::hash<int>{}(edge.first) ^ (std::hash<int>{}(edge.second) << 1);
    }
};

std::vector<std::pair<int, int>> SurfaceCapping::findBoundaryEdges(const Mesh* mesh) {
    std::vector<std::pair<int, int>> boundaryEdges;
    ankerl::unordered_dense::map<std::pair<int, int>, int, EdgeHash> edgeCount;

    // Count edge occurrences
    const auto& faces = mesh->faces();
    for (int i = 0; i < faces.cols(); ++i) {
        const auto& face = faces.col(i);
        
        // Add edges with consistent orientation
        for (int j = 0; j < 3; ++j) {
            int v1 = face[j];
            int v2 = face[(j + 1) % 3];
            
            std::pair<int, int> edge = (v1 < v2) ? std::make_pair(v1, v2) : std::make_pair(v2, v1);
            edgeCount[edge]++;
        }
    }

    // Find boundary edges (appear only once)
    for (const auto& [edge, count] : edgeCount) {
        if (count == 1) {
            boundaryEdges.push_back(edge);
        }
    }

    return boundaryEdges;
}