#pragma once
#include "mesh.h"
#include <Eigen/Dense>
#include <vector>

class ChemicalStructure;

/**
 * Surface capping functionality for handling void surfaces and other isosurfaces
 * that extend beyond unit cell boundaries.
 * 
 * Provides multiple algorithms for closing surfaces:
 * 1. Plane-based clipping with unit cell boundaries
 * 2. Boundary triangulation methods
 * 3. Configurable capping options
 */
class SurfaceCapping {
public:
    enum class Method {
        None,                    // No capping
        PlaneCut,               // Clip against unit cell planes
        BoundaryFan,            // Triangle fan from boundary edges
        DelaunayTriangulation   // Constrained Delaunay (future)
    };

    enum class BoundaryMode {
        UnitCell,       // Cap at unit cell boundaries
        CustomBox,      // Cap at custom bounding box
        SphereClip      // Cap with spherical boundary
    };

    struct CappingOptions {
        Method method = Method::PlaneCut;
        BoundaryMode boundaryMode = BoundaryMode::UnitCell;
        
        // Unit cell capping options
        bool capXMin = true, capXMax = true;
        bool capYMin = true, capYMax = true; 
        bool capZMin = true, capZMax = true;
        
        // Custom box (fractional coordinates)
        Eigen::Vector3d boxMin{0.0, 0.0, 0.0};
        Eigen::Vector3d boxMax{1.0, 1.0, 1.0};
        
        // Sphere clipping
        Eigen::Vector3d sphereCenter{0.5, 0.5, 0.5};
        double sphereRadius = 0.5;
        
        // Quality settings
        double tolerance = 1e-6;
        bool removeDegenerate = true;
        bool smoothNormals = true;
    };

    /**
     * Apply surface capping to a mesh
     * @param mesh Input mesh to be capped
     * @param structure Crystal structure for unit cell information
     * @param options Capping configuration
     * @return New capped mesh, or nullptr on failure
     */
    static Mesh* applyCapping(const Mesh* mesh, 
                             const ChemicalStructure* structure,
                             const CappingOptions& options);

    /**
     * Check if a mesh likely needs capping (extends beyond boundaries)
     * @param mesh Mesh to analyze
     * @param structure Crystal structure for unit cell information
     * @return True if mesh extends beyond unit cell
     */
    static bool needsCapping(const Mesh* mesh, const ChemicalStructure* structure);

    /**
     * Get default capping options for void surfaces
     */
    static CappingOptions getVoidSurfaceDefaults();

private:
    /**
     * Plane-based clipping algorithm (Sutherland-Hodgman style)
     */
    static Mesh* applyPlaneClipping(const Mesh* mesh,
                                   const ChemicalStructure* structure,
                                   const CappingOptions& options);

    /**
     * Simple boundary fan triangulation
     */
    static Mesh* applyBoundaryFan(const Mesh* mesh,
                                 const ChemicalStructure* structure,
                                 const CappingOptions& options);

    /**
     * Utility: Get unit cell planes for clipping
     */
    static std::vector<Eigen::Vector4d> getUnitCellPlanes(const ChemicalStructure* structure,
                                                          const CappingOptions& options);

    /**
     * Utility: Clip triangle against plane
     */
    static std::vector<Eigen::Vector3d> clipTriangleAgainstPlane(
        const std::vector<Eigen::Vector3d>& triangle,
        const Eigen::Vector4d& plane,
        double tolerance = 1e-6);

    /**
     * Utility: Find boundary edges in mesh
     */
    static std::vector<std::pair<int, int>> findBoundaryEdges(const Mesh* mesh);

    /**
     * Utility: Generate cap triangles for boundary
     */
    static void generateCapTriangles(std::vector<Eigen::Vector3d>& vertices,
                                   std::vector<Eigen::Vector3i>& faces,
                                   const std::vector<std::pair<int, int>>& boundaryEdges,
                                   const Eigen::Vector3d& planeNormal);
};