#include "wavefrontobjfile.h"
#include <QDebug>
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


Mesh *WavefrontObjectFile::getFirstMesh(QObject * parent) const {
    if(m_shapes.empty()) return nullptr;

    const auto& attrib = m_attrib; // Assuming this is where you store the parsed attributes
    const auto& mesh = m_shapes.front().mesh; // Working with the first shape

    std::vector<double> vertices;
    std::vector<int> faces;
    std::vector<double> normals;

    for(int i = 0; i < attrib.vertices.size(); i++) {
	vertices.push_back(attrib.vertices[i]);
    }
    for(int i = 0; i < attrib.normals.size(); i++) {
	normals.push_back(attrib.normals[i]);
    }
    // Loop over faces
    size_t index_offset = 0;
    for (size_t f = 0; f < mesh.num_face_vertices.size(); ++f) {
        int fv = mesh.num_face_vertices[f];
	if(fv > 3) qDebug() << "Face has more than 3 vertices... this will be a problem";

        for (size_t v = 0; v < fv; ++v) {
            tinyobj::index_t idx = mesh.indices[index_offset + v];
            faces.push_back(idx.vertex_index);
        }
        index_offset += fv;
    }

    // Convert vectors to Eigen matrices for vertices and normals
    Eigen::Map<const Mesh::VertexList> vertexMap(
	    reinterpret_cast<const double*>(vertices.data()), 3, vertices.size() / 3);
    Eigen::Map<const Mesh::FaceList> faceMap(faces.data(), 3, faces.size() / 3);

    // Create the Mesh object
    Mesh* result = new Mesh(vertexMap, faceMap, parent);
    // Assuming you have a method to set normals in your Mesh class
    if(normals.size() == vertices.size()) {
	qDebug() << "Vertex normals: " << normals.size() / 3;
	Eigen::Map<const Mesh::VertexList> normalMap(reinterpret_cast<const double*>(normals.data()), 3, normals.size() / 3);
	result->setVertexNormals(normalMap);
    }
    else {
	result->setVertexNormals(result->computeVertexNormals());
    }
    return result;
}

void WavefrontObjectFile::load(const std::string& filename) {
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./"; // Path to material files

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(filename, reader_config)) {
      if (!reader.Error().empty()) {
	  qWarning() << "TinyObjReader: " << reader.Error();
      }
      return;
    }

    if (!reader.Warning().empty()) {
      qWarning() << "TinyObjReader: " << reader.Warning();
    }

    if(!reader.Valid()) return;

    m_attrib = reader.GetAttrib();
    m_shapes = reader.GetShapes();
    m_materials = reader.GetMaterials();
}
