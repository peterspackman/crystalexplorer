#pragma once
#include "tiny_obj_loader.h"
#include "mesh.h"
#include <QString>
#include <vector>
#include <iostream>

class WavefrontObjectFile {
public:
    WavefrontObjectFile(const QString& filename) {
        load(filename.toStdString());
    }


    [[nodiscard]] inline const auto &shapes() const {
        return m_shapes;
    }

    [[nodiscard]] inline const auto &materials() const {
        return m_materials;
    }

    Mesh *getFirstMesh(QObject *parent = nullptr) const;

private:
    tinyobj::attrib_t m_attrib;
    std::vector<tinyobj::shape_t> m_shapes;
    std::vector<tinyobj::material_t> m_materials;
    void load(const std::string& filename);
};

