#include "load_mesh.h"
#include "tinyply.h"
#include <vector>
#include <iostream>
#include <memory>
#include <vector>
#include <fstream>
#include <ankerl/unordered_dense.h>

inline std::vector<uint8_t> read_file_binary(const std::string & pathToFile)
{
    std::ifstream file(pathToFile, std::ios::binary);
    std::vector<uint8_t> fileBufferBytes;

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t sizeBytes = file.tellg();
        file.seekg(0, std::ios::beg);
        fileBufferBytes.resize(sizeBytes);
        if (file.read((char*)fileBufferBytes.data(), sizeBytes)) return fileBufferBytes;
    }
    else throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
    return fileBufferBytes;
}

struct memory_buffer : public std::streambuf
{
    char * p_start {nullptr};
    char * p_end {nullptr};
    size_t size;

    memory_buffer(char const * first_elem, size_t size)
        : p_start(const_cast<char*>(first_elem)), p_end(p_start + size), size(size)
    {
        setg(p_start, p_start, p_end);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode which) override
    {
        if (dir == std::ios_base::cur) gbump(static_cast<int>(off));
        else setg(p_start, (dir == std::ios_base::beg ? p_start : p_end) + off, p_end);
        return gptr() - p_start;
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override
    {
        return seekoff(pos, std::ios_base::beg, which);
    }
};

struct memory_stream : virtual memory_buffer, public std::istream
{
    memory_stream(char const * first_elem, size_t size)
        : memory_buffer(first_elem, size), std::istream(static_cast<std::streambuf*>(this)) {}
};

namespace io {

Mesh* read_ply_file(const QString& filepath, bool preload_into_memory = true) {
    qDebug() << "Loading mesh from " << filepath;

    std::string filepathStd = filepath.toStdString();
    std::unique_ptr<std::istream> file_stream;
    std::vector<uint8_t> byte_buffer;

    try {
	// For most files < 1gb, pre-loading the entire file upfront and wrapping it into a 
        // stream is a net win for parsing speed, about 40% faster. 
        if (preload_into_memory)
        {
            byte_buffer = read_file_binary(filepathStd);
            file_stream.reset(new memory_stream((char*)byte_buffer.data(), byte_buffer.size()));
        }
        else
        {
            file_stream.reset(new std::ifstream(filepathStd, std::ios::binary));
        }

        if (!file_stream || file_stream->fail()) throw std::runtime_error("file_stream failed to open " + filepathStd);

        file_stream->seekg(0, std::ios::end);
        const float size_mb = file_stream->tellg() * float(1e-6);
        file_stream->seekg(0, std::ios::beg);

        tinyply::PlyFile file;
        file.parse_header(*file_stream);

        // Assuming the existence of a PlyFile class or similar to handle the parsing
        auto vertices = file.request_properties_from_element("vertex", {"x", "y", "z"});
        auto faces = file.request_properties_from_element("face", {"vertex_indices"}, 3);

	auto normals = file.request_properties_from_element("vertex", {"nx", "ny", "nz"});

	std::vector<std::string> property_names = {"di", "di_norm", "de", "de_norm"};
	ankerl::unordered_dense::map<std::string, std::shared_ptr<tinyply::PlyData>> properties;
	for(const auto &prop_name: property_names) {
	    std::shared_ptr<tinyply::PlyData> prop;
	    try { prop = file.request_properties_from_element("vertex", {prop_name}); }
	    catch (const std::exception & e) { std::cerr << "tinyply exception: " << e.what() << std::endl; }
	    if (!prop) continue;
	    properties[prop_name] = prop;
	}


        file.read(*file_stream);


        if (vertices) {
            qDebug() << "Read" << vertices->count << "vertices";
        }

        if (faces) {
            qDebug() << "Read" << faces->count << "faces";
        }

	if (normals) {
            qDebug() << "Read" << normals->count << "normals";
	}

        // Assuming vertices and faces are now filled with data
        Mesh::VertexList vertexMatrix(3, vertices->count);
        Mesh::FaceList faceMatrix(3, faces->count);
	Mesh::VertexList normalMatrix(3, normals->count);

	vertexMatrix = Eigen::Map<const Eigen::Matrix3Xf>(reinterpret_cast<const float *>(vertices->buffer.get()), 3, vertices->count).cast<double>();

	faceMatrix = Eigen::Map<const Eigen::Matrix<uint32_t, 3, Eigen::Dynamic>>(reinterpret_cast<const uint32_t*>(faces->buffer.get()), 3, faces->count).cast<int>();


	normalMatrix = Eigen::Map<const Eigen::Matrix3Xf>(reinterpret_cast<const float *>(normals->buffer.get()), 3, normals->count).cast<double>();

        // Now, you can create your Mesh object
        Mesh* mesh = new Mesh(vertexMatrix, faceMatrix);
	mesh->setVertexNormals(normalMatrix);

	for(const auto &[prop_name, prop]: properties) {
	    qDebug() << QString::fromStdString(prop_name) << "read" << prop->count << "property values";
	    switch(prop->t) {
		case tinyply::Type::FLOAT32: {
		    qDebug() << "count =" << prop->count << prop->buffer.get();
		    const float * buf = reinterpret_cast<const float *>(prop->buffer.get());
		    if(!buf) {
			qDebug() << "Failed reading property values";
			break;
		    }
		    Eigen::VectorXf v = Eigen::Map<const Eigen::VectorXf>(buf, prop->count);
		    mesh->setVertexProperty(QString::fromStdString(prop_name), v);
		    break;
		}
		default:
		    break;
	    }
	}

        return mesh;

    } catch (const std::exception& e) {
        qDebug() << "Caught exception: " << e.what();
        return nullptr;
    }
}


Mesh * loadMesh(const QString &filename) {
    return read_ply_file(filename);
}


}
