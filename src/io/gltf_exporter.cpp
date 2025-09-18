#include "gltf_exporter.h"
#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>
#include <filesystem>

#include "chemicalstructure.h"
#include "drawingstyle.h"
#include "elementdata.h"
#include "fragment.h"
#include "mesh.h"
#include "meshinstance.h"
#include "scene.h"
#include "settings.h"

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QTextStream>
#include <algorithm>
#include <ankerl/unordered_dense.h>
#include <cmath>
#include <unordered_set>

namespace cx::core {

GLTFExporter::GLTFExporter() {
  // Read material properties from settings
  m_materialRoughness =
      settings::readSetting(settings::keys::MATERIAL_ROUGHNESS).toFloat();
  m_materialMetallic =
      settings::readSetting(settings::keys::MATERIAL_METALLIC).toFloat();
}

void GLTFExporter::loadIcosphereMesh() {
  QFile obj(":/mesh/icosphere.obj");
  if (!obj.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open icosphere.obj";
    return;
  }

  QTextStream stream(&obj);
  QString line;
  while (stream.readLineInto(&line)) {
    if (line.startsWith('v')) {
      QStringList tokens = line.split(' ');
      if (tokens.size() >= 4) {
        m_sphereVertices.push_back(tokens[1].toFloat());
        m_sphereVertices.push_back(tokens[2].toFloat());
        m_sphereVertices.push_back(tokens[3].toFloat());
      }
    } else if (line.startsWith('f')) {
      QStringList tokens = line.split(' ');
      if (tokens.size() >= 4) {
        m_sphereIndices.push_back(tokens[1].toUInt() - 1);
        m_sphereIndices.push_back(tokens[2].toUInt() - 1);
        m_sphereIndices.push_back(tokens[3].toUInt() - 1);
      }
    }
  }
  qDebug() << "Loaded icosphere:" << m_sphereVertices.size() / 3 << "vertices,"
           << m_sphereIndices.size() / 3 << "triangles";
}

void GLTFExporter::loadCylinderMesh() {
  QFile obj(":/mesh/cylinder.obj");
  if (!obj.open(QIODevice::ReadOnly)) {
    qWarning() << "Failed to open cylinder.obj";
    return;
  }

  QTextStream stream(&obj);
  QString line;
  while (stream.readLineInto(&line)) {
    if (line.startsWith('v')) {
      QStringList tokens = line.split(' ');
      if (tokens.size() >= 4) {
        m_cylinderVertices.push_back(tokens[1].toFloat());
        m_cylinderVertices.push_back(tokens[2].toFloat());
        m_cylinderVertices.push_back(tokens[3].toFloat());
      }
    } else if (line.startsWith('f')) {
      QStringList tokens = line.split(' ');
      if (tokens.size() >= 4) {
        m_cylinderIndices.push_back(tokens[1].toUInt() - 1);
        m_cylinderIndices.push_back(tokens[2].toUInt() - 1);
        m_cylinderIndices.push_back(tokens[3].toUInt() - 1);
      }
    }
  }
  qDebug() << "Loaded cylinder:" << m_cylinderVertices.size() / 3 << "vertices,"
           << m_cylinderIndices.size() / 3 << "triangles";
}

void GLTFExporter::addAtomsToAsset(
    fastgltf::Asset &asset,
    const ankerl::unordered_dense::map<
        int, std::vector<std::pair<QVector3D, float>>> &atomsByElement,
    const ExportOptions &options) {
  // Convert vertex data to bytes (copying from wad2gltf pattern)
  std::vector<uint8_t> vertexBytes;
  vertexBytes.resize(m_sphereVertices.size() * sizeof(float));
  std::memcpy(vertexBytes.data(), m_sphereVertices.data(), vertexBytes.size());

  std::vector<uint8_t> indexBytes;
  indexBytes.resize(m_sphereIndices.size() * sizeof(uint32_t));
  std::memcpy(indexBytes.data(), m_sphereIndices.data(), indexBytes.size());

  // Create buffers exactly like wad2gltf
  auto &vertexBuffer = asset.buffers.emplace_back();
  vertexBuffer.name = "sphere_vertices";
  vertexBuffer.byteLength = vertexBytes.size();
  // Convert to std::byte as required by fastgltf v0.9
  std::vector<std::byte> vertexByteData(
      reinterpret_cast<std::byte *>(vertexBytes.data()),
      reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
  vertexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

  auto &indexBuffer = asset.buffers.emplace_back();
  indexBuffer.name = "sphere_indices";
  indexBuffer.byteLength = indexBytes.size();
  std::vector<std::byte> indexByteData(
      reinterpret_cast<std::byte *>(indexBytes.data()),
      reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
  indexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

  // Create buffer views
  auto &vertexBufferView = asset.bufferViews.emplace_back();
  vertexBufferView.bufferIndex = asset.buffers.size() - 2; // vertex buffer
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteStride = sizeof(float) * 3; // 3 floats per vertex
  vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

  auto &indexBufferView = asset.bufferViews.emplace_back();
  indexBufferView.bufferIndex = asset.buffers.size() - 1; // index buffer
  indexBufferView.byteOffset = 0;
  indexBufferView.byteLength = indexBuffer.byteLength;
  indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

  // Create accessors
  auto &positionAccessor = asset.accessors.emplace_back();
  positionAccessor.bufferViewIndex =
      asset.bufferViews.size() - 2; // vertex buffer view
  positionAccessor.componentType = fastgltf::ComponentType::Float;
  positionAccessor.count = m_sphereVertices.size() / 3;
  positionAccessor.type = fastgltf::AccessorType::Vec3;

  auto &indexAccessor = asset.accessors.emplace_back();
  indexAccessor.bufferViewIndex =
      asset.bufferViews.size() - 1; // index buffer view
  indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
  indexAccessor.count = m_sphereIndices.size();
  indexAccessor.type = fastgltf::AccessorType::Scalar;

  // Create materials and meshes for each element type
  for (const auto &[atomicNumber, atoms] : atomsByElement) {
    auto *element = ElementData::elementFromAtomicNumber(atomicNumber);
    if (!element)
      continue;

    // Create material
    auto &material = asset.materials.emplace_back();
    material.name = element->symbol().toStdString();
    QColor color = element->color();
    material.pbrData.baseColorFactor = {
        static_cast<float>(color.redF()), static_cast<float>(color.greenF()),
        static_cast<float>(color.blueF()), 1.0f};
    material.pbrData.metallicFactor = m_materialMetallic;
    material.pbrData.roughnessFactor = m_materialRoughness;

    // Create mesh
    auto &mesh = asset.meshes.emplace_back();
    mesh.name = element->symbol().toStdString() + "_spheres";

    auto &primitive = mesh.primitives.emplace_back();
    primitive.type = fastgltf::PrimitiveType::Triangles;
    primitive.attributes.emplace_back("POSITION", asset.accessors.size() -
                                                      2);   // position accessor
    primitive.indicesAccessor = asset.accessors.size() - 1; // index accessor
    primitive.materialIndex = asset.materials.size() - 1;

    // Create nodes for each atom instance
    for (const auto &[position, radius] : atoms) {
      auto &node = asset.nodes.emplace_back();
      node.name = element->symbol().toStdString() + "_atom";
      fastgltf::TRS trs;
      trs.translation = {position.x(), position.y(), position.z()};
      trs.rotation =
          fastgltf::math::fquat(0.0f, 0.0f, 0.0f, 1.0f); // identity quaternion
      trs.scale = {radius, radius, radius};
      node.transform = trs;
      node.meshIndex = asset.meshes.size() - 1;

      // Add to scene
      if (!asset.scenes.empty()) {
        asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
      }
    }
  }

  qDebug() << "Added" << atomsByElement.size() << "element types to GLTF asset";
}

bool GLTFExporter::exportScene(const Scene *scene, const QString &filePath,
                               const ExportOptions &options) {
  if (!scene) {
    qWarning() << "GLTFExporter: Invalid scene";
    return false;
  }

  // Get export data from scene
  auto exportData = scene->getExportData();

  // Load primitive meshes
  loadIcosphereMesh();
  loadCylinderMesh();

  // Create GLTF asset
  fastgltf::Asset asset;
  asset.assetInfo =
      fastgltf::AssetInfo{.gltfVersion = "2.0", .generator = "CrystalExplorer"};
  asset.defaultScene = 0;

  // Create root scene and node
  auto &gltfScene = asset.scenes.emplace_back();
  gltfScene.name = "Scene";

  auto &rootNode = asset.nodes.emplace_back();
  rootNode.name = "Root";
  gltfScene.nodeIndices.push_back(0);

  // Export atoms, bonds, meshes, and camera using SceneExportData
  qDebug() << "SceneExportData contains:" << exportData.spheres().size()
           << "spheres," << exportData.cylinders().size() << "cylinders,"
           << exportData.meshes().size() << "meshes";

  // TODO: Camera export is currently broken, needs investigation
  // addCameraToAsset(asset, exportData.camera());

  if (options.exportAtoms && !exportData.spheres().empty()) {
    addSpheresToAsset(asset, exportData.spheres(), options);
  }

  if (options.exportBonds && !exportData.cylinders().empty()) {
    addCylindersToAsset(asset, exportData.cylinders(), options);
  }

  if (options.exportMeshes && !exportData.meshes().empty()) {
    addMeshesToAsset(asset, exportData.meshes(), options);
  }

  // Write file
  fastgltf::FileExporter exporter;
  fastgltf::ExportOptions exportOptions = fastgltf::ExportOptions::None;
  fastgltf::Error result =
      exporter.writeGltfBinary(asset, filePath.toStdString(), exportOptions);

  if (result != fastgltf::Error::None) {
    qWarning() << "GLTFExporter: Failed to write file:" << filePath;
    return false;
  }

  qDebug() << "GLTFExporter: Successfully exported scene to" << filePath;
  return true;
}

bool GLTFExporter::exportScene(const Scene *scene, const QString &filePath) {
  return exportScene(scene, filePath, ExportOptions{});
}

bool GLTFExporter::exportStructure(const ChemicalStructure *structure,
                                   const QString &filePath,
                                   const ExportOptions &options) {
  return exportStructure(structure, filePath, options, nullptr);
}

bool GLTFExporter::exportStructure(const ChemicalStructure *structure,
                                   const QString &filePath,
                                   const ExportOptions &options,
                                   const Scene *scene) {
  if (!structure) {
    qWarning() << "GLTFExporter: Invalid structure";
    return false;
  }

  // Load meshes if needed
  if (m_sphereVertices.empty()) {
    loadIcosphereMesh();
    loadCylinderMesh();
  }

  if (m_sphereVertices.empty()) {
    qWarning() << "GLTFExporter: Failed to load sphere mesh";
    return false;
  }

  // Create fastgltf asset
  fastgltf::Asset asset;
  asset.assetInfo =
      fastgltf::AssetInfo{.gltfVersion = "2.0", .generator = "CrystalExplorer"};
  asset.defaultScene = 0;

  // Create default scene
  auto &gltfScene = asset.scenes.emplace_back();
  gltfScene.name = "CrystalExplorer Scene";

  // Always use fragment-based approach for proper hierarchy
  addStructureByFragments(asset, structure, options, scene);

  // Write file
  fastgltf::FileExporter exporter;
  auto exportOptions = options.prettyPrint
                           ? fastgltf::ExportOptions::PrettyPrintJson
                           : fastgltf::ExportOptions::None;

  fastgltf::Error result;
  if (options.binaryFormat) {
    result =
        exporter.writeGltfBinary(asset, filePath.toStdString(), exportOptions);
  } else {
    // For JSON format, data is automatically base64-encoded and embedded
    result =
        exporter.writeGltfJson(asset, filePath.toStdString(), exportOptions);
  }

  if (result != fastgltf::Error::None) {
    qWarning() << "GLTFExporter: Failed to write file:" << filePath;
    return false;
  }

  return true;
}

bool GLTFExporter::exportStructure(const ChemicalStructure *structure,
                                   const QString &filePath) {
  return exportStructure(structure, filePath, ExportOptions{});
}

float GLTFExporter::getAtomDisplayRadius(const ChemicalStructure *structure,
                                         int atomIndex, const Scene *scene,
                                         const ExportOptions &options) {
  auto *element = ElementData::elementFromAtomicNumber(
      structure->atomicNumbers()(atomIndex));
  if (!element)
    return 1.0f * options.atomRadiusScale;

  float radius = element->covRadius() * 0.5f; // Default: covalent radius sphere

  if (scene) {
    auto drawingStyle = scene->drawingStyle();
    auto atomStyle = atomStyleForDrawingStyle(drawingStyle);

    switch (atomStyle) {
    case AtomDrawingStyle::VanDerWaalsSphere:
      radius = element->vdwRadius();
      break;
    case AtomDrawingStyle::RoundCapped:
      // Use bond thickness - approximate with covalent radius of hydrogen
      radius = ElementData::elementFromAtomicNumber(1)->covRadius() *
               0.2f; // Bond thickness factor
      break;
    case AtomDrawingStyle::CovalentRadiusSphere:
    default:
      radius = element->covRadius() * 0.5f;
      break;
    }
  }

  return radius * options.atomRadiusScale;
}

void GLTFExporter::addBondsToAsset(fastgltf::Asset &asset,
                                   const ChemicalStructure *structure,
                                   const ExportOptions &options) {
  const auto &bonds = structure->covalentBonds();
  if (bonds.empty())
    return;

  // Convert cylinder vertex data to bytes
  std::vector<uint8_t> vertexBytes;
  vertexBytes.resize(m_cylinderVertices.size() * sizeof(float));
  std::memcpy(vertexBytes.data(), m_cylinderVertices.data(),
              vertexBytes.size());

  std::vector<uint8_t> indexBytes;
  indexBytes.resize(m_cylinderIndices.size() * sizeof(uint32_t));
  std::memcpy(indexBytes.data(), m_cylinderIndices.data(), indexBytes.size());

  // Create buffers
  auto &vertexBuffer = asset.buffers.emplace_back();
  vertexBuffer.name = "bond_vertices";
  vertexBuffer.byteLength = vertexBytes.size();
  std::vector<std::byte> vertexByteData(
      reinterpret_cast<std::byte *>(vertexBytes.data()),
      reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
  vertexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

  auto &indexBuffer = asset.buffers.emplace_back();
  indexBuffer.name = "bond_indices";
  indexBuffer.byteLength = indexBytes.size();
  std::vector<std::byte> indexByteData(
      reinterpret_cast<std::byte *>(indexBytes.data()),
      reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
  indexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

  // Create buffer views
  auto &vertexBufferView = asset.bufferViews.emplace_back();
  vertexBufferView.bufferIndex = asset.buffers.size() - 2; // vertex buffer
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteStride = sizeof(float) * 3; // 3 floats per vertex
  vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

  auto &indexBufferView = asset.bufferViews.emplace_back();
  indexBufferView.bufferIndex = asset.buffers.size() - 1; // index buffer
  indexBufferView.byteOffset = 0;
  indexBufferView.byteLength = indexBuffer.byteLength;
  indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

  // Create accessors
  auto &positionAccessor = asset.accessors.emplace_back();
  positionAccessor.bufferViewIndex =
      asset.bufferViews.size() - 2; // vertex buffer view
  positionAccessor.componentType = fastgltf::ComponentType::Float;
  positionAccessor.count = m_cylinderVertices.size() / 3;
  positionAccessor.type = fastgltf::AccessorType::Vec3;

  auto &indexAccessor = asset.accessors.emplace_back();
  indexAccessor.bufferViewIndex =
      asset.bufferViews.size() - 1; // index buffer view
  indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
  indexAccessor.count = m_cylinderIndices.size();
  indexAccessor.type = fastgltf::AccessorType::Scalar;

  // Create bond mesh (shared by all half-bonds)
  auto &bondMesh = asset.meshes.emplace_back();
  bondMesh.name = "bond_cylinders";

  auto &primitive = bondMesh.primitives.emplace_back();
  primitive.type = fastgltf::PrimitiveType::Triangles;
  primitive.attributes.emplace_back("POSITION", asset.accessors.size() -
                                                    2);   // position accessor
  primitive.indicesAccessor = asset.accessors.size() - 1; // index accessor
  // Material will be set per instance

  // Collect unique element colors and create materials for them
  ankerl::unordered_dense::map<QRgb, size_t> colorToMaterialIndex;
  const auto &positions = structure->atomicPositions();
  const auto &atomicNumbers = structure->atomicNumbers();

  // Create bond instances as half-bonds with atom colors
  for (const auto &[atomA, atomB] : bonds) {
    QVector3D posA(positions(0, atomA), positions(1, atomA),
                   positions(2, atomA));
    QVector3D posB(positions(0, atomB), positions(1, atomB),
                   positions(2, atomB));
    QVector3D bondMidpoint = (posA + posB) * 0.5f;

    // Get atom colors
    auto *elementA = ElementData::elementFromAtomicNumber(atomicNumbers(atomA));
    auto *elementB = ElementData::elementFromAtomicNumber(atomicNumbers(atomB));
    QColor colorA = elementA ? elementA->color() : Qt::gray;
    QColor colorB = elementB ? elementB->color() : Qt::gray;

    float bondRadius = options.bondRadiusScale;

    // Create two half-bonds with different materials
    for (int half = 0; half < 2; ++half) {
      QVector3D startPos = (half == 0) ? posA : posB;
      QVector3D endPos = bondMidpoint;
      QColor bondColor = (half == 0) ? colorA : colorB;

      QVector3D bondVector = endPos - startPos;
      float halfBondLength = bondVector.length();
      bondVector.normalize();

      QVector3D halfBondCenter = (startPos + endPos) * 0.5f;

      // Calculate rotation to align cylinder with bond direction
      // Cylinder default orientation is along Z-axis (0, 0, 1)
      QVector3D defaultDirection(0.0f, 0.0f, 1.0f);
      QVector3D axis = QVector3D::crossProduct(defaultDirection, bondVector);
      float angle =
          std::acos(QVector3D::dotProduct(defaultDirection, bondVector));

      // Get or create material for this color
      QRgb colorKey = bondColor.rgb();
      size_t materialIndex;
      auto it = colorToMaterialIndex.find(colorKey);
      if (it != colorToMaterialIndex.end()) {
        materialIndex = it->second;
      } else {
        // Create new material
        auto &material = asset.materials.emplace_back();
        material.name =
            QString("Bond Material %1").arg(bondColor.name()).toStdString();
        material.pbrData.baseColorFactor = {
            static_cast<float>(bondColor.redF()),
            static_cast<float>(bondColor.greenF()),
            static_cast<float>(bondColor.blueF()), 1.0f};
        material.pbrData.metallicFactor = m_materialMetallic;
        material.pbrData.roughnessFactor = m_materialRoughness;
        materialIndex = asset.materials.size() - 1;
        colorToMaterialIndex[colorKey] = materialIndex;
      }

      // Create mesh instance with this material
      auto &halfMesh = asset.meshes.emplace_back();
      halfMesh.name = "Half Bond Cylinder";

      auto &halfPrimitive = halfMesh.primitives.emplace_back();
      halfPrimitive.type = fastgltf::PrimitiveType::Triangles;
      halfPrimitive.attributes.emplace_back(
          "POSITION", asset.accessors.size() - 2); // position accessor
      halfPrimitive.indicesAccessor =
          asset.accessors.size() - 1; // index accessor
      halfPrimitive.materialIndex = materialIndex;

      auto &node = asset.nodes.emplace_back();
      node.name = "Half Bond";
      fastgltf::TRS trs;
      trs.translation = {halfBondCenter.x(), halfBondCenter.y(),
                         halfBondCenter.z()};

      // Handle rotation - if axis is too small, bond is already aligned with Z
      if (axis.length() > 0.001f) {
        axis.normalize();
        float s = std::sin(angle * 0.5f);
        float c = std::cos(angle * 0.5f);
        trs.rotation =
            fastgltf::math::fquat(axis.x() * s, axis.y() * s, axis.z() * s, c);
      } else {
        // Bond is aligned with Z-axis, check if it's in the same or opposite
        // direction
        if (QVector3D::dotProduct(defaultDirection, bondVector) < 0) {
          // Bond points in negative Z, rotate 180 degrees around X-axis
          trs.rotation = fastgltf::math::fquat(1.0f, 0.0f, 0.0f, 0.0f);
        } else {
          // Bond already aligned correctly
          trs.rotation = fastgltf::math::fquat(0.0f, 0.0f, 0.0f, 1.0f);
        }
      }

      trs.scale = {bondRadius, bondRadius, halfBondLength};
      node.transform = trs;
      node.meshIndex = asset.meshes.size() - 1;

      // Add to scene
      if (!asset.scenes.empty()) {
        asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
      }
    }
  }

  qDebug() << "Added" << bonds.size() << "bonds to GLTF asset";
}

void GLTFExporter::addMeshesToAsset(fastgltf::Asset &asset,
                                    const ChemicalStructure *structure,
                                    const ExportOptions &options) {
  int meshCount = 0;
  int instanceCount = 0;

  // Iterate through structure children to find meshes
  for (auto *child : structure->children()) {
    auto *mesh = qobject_cast<Mesh *>(child);
    if (!mesh)
      continue;

    // Skip empty meshes
    if (mesh->numberOfVertices() == 0)
      continue;

    meshCount++;

    // Get mesh data
    const auto &vertices = mesh->vertices();
    const auto &faces = mesh->faces();

    // Convert vertices to flat float array
    std::vector<float> vertexData;
    vertexData.reserve(vertices.cols() * 3);
    for (int i = 0; i < vertices.cols(); ++i) {
      vertexData.push_back(static_cast<float>(vertices(0, i)));
      vertexData.push_back(static_cast<float>(vertices(1, i)));
      vertexData.push_back(static_cast<float>(vertices(2, i)));
    }

    // Convert faces to flat uint32_t array
    std::vector<uint32_t> indexData;
    indexData.reserve(faces.cols() * 3);
    for (int i = 0; i < faces.cols(); ++i) {
      indexData.push_back(static_cast<uint32_t>(faces(0, i)));
      indexData.push_back(static_cast<uint32_t>(faces(1, i)));
      indexData.push_back(static_cast<uint32_t>(faces(2, i)));
    }

    // Convert vertex data to bytes
    std::vector<uint8_t> vertexBytes;
    vertexBytes.resize(vertexData.size() * sizeof(float));
    std::memcpy(vertexBytes.data(), vertexData.data(), vertexBytes.size());

    std::vector<uint8_t> indexBytes;
    indexBytes.resize(indexData.size() * sizeof(uint32_t));
    std::memcpy(indexBytes.data(), indexData.data(), indexBytes.size());

    // Create buffers
    auto &vertexBuffer = asset.buffers.emplace_back();
    vertexBuffer.name = (mesh->objectName() + " Vertices").toStdString();
    vertexBuffer.byteLength = vertexBytes.size();
    std::vector<std::byte> vertexByteData(
        reinterpret_cast<std::byte *>(vertexBytes.data()),
        reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
    vertexBuffer.data = fastgltf::sources::Array{
        .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

    auto &indexBuffer = asset.buffers.emplace_back();
    indexBuffer.name = (mesh->objectName() + " Indices").toStdString();
    indexBuffer.byteLength = indexBytes.size();
    std::vector<std::byte> indexByteData(
        reinterpret_cast<std::byte *>(indexBytes.data()),
        reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
    indexBuffer.data = fastgltf::sources::Array{
        .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

    // Create buffer views
    auto &vertexBufferView = asset.bufferViews.emplace_back();
    vertexBufferView.bufferIndex = asset.buffers.size() - 2; // vertex buffer
    vertexBufferView.byteOffset = 0;
    vertexBufferView.byteLength = vertexBuffer.byteLength;
    vertexBufferView.byteStride = sizeof(float) * 3; // 3 floats per vertex
    vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

    auto &indexBufferView = asset.bufferViews.emplace_back();
    indexBufferView.bufferIndex = asset.buffers.size() - 1; // index buffer
    indexBufferView.byteOffset = 0;
    indexBufferView.byteLength = indexBuffer.byteLength;
    indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

    // Create accessors
    auto &positionAccessor = asset.accessors.emplace_back();
    positionAccessor.bufferViewIndex =
        asset.bufferViews.size() - 2; // vertex buffer view
    positionAccessor.componentType = fastgltf::ComponentType::Float;
    positionAccessor.count = vertices.cols();
    positionAccessor.type = fastgltf::AccessorType::Vec3;

    auto &indexAccessor = asset.accessors.emplace_back();
    indexAccessor.bufferViewIndex =
        asset.bufferViews.size() - 1; // index buffer view
    indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
    indexAccessor.count = indexData.size();
    indexAccessor.type = fastgltf::AccessorType::Scalar;

    // Create material - use mesh color if available, otherwise default
    auto &material = asset.materials.emplace_back();
    material.name = (mesh->objectName() + " Material").toStdString();

    // Get mesh color (this might need adjustment based on how mesh colors are
    // stored)
    QColor meshColor = Qt::gray; // Default color
    if (mesh->isVisible()) {
      // Try to get actual mesh color - this may need adjustment
      meshColor = QColor(128, 128, 255); // Light blue for surfaces
    }

    material.pbrData.baseColorFactor = {
        static_cast<float>(meshColor.redF()),
        static_cast<float>(meshColor.greenF()),
        static_cast<float>(meshColor.blueF()),
        mesh->isTransparent() ? (1.0f - mesh->getTransparency()) : 1.0f};
    material.pbrData.metallicFactor = m_materialMetallic;
    material.pbrData.roughnessFactor = m_materialRoughness;

    // Create mesh
    auto &gltfMesh = asset.meshes.emplace_back();
    gltfMesh.name = mesh->objectName().toStdString();

    auto &primitive = gltfMesh.primitives.emplace_back();
    primitive.type = fastgltf::PrimitiveType::Triangles;
    primitive.attributes.emplace_back("POSITION", asset.accessors.size() -
                                                      2);   // position accessor
    primitive.indicesAccessor = asset.accessors.size() - 1; // index accessor
    primitive.materialIndex = asset.materials.size() - 1;

    // Create instances for each MeshInstance child
    for (auto *meshChild : mesh->children()) {
      auto *meshInstance = qobject_cast<MeshInstance *>(meshChild);
      if (!meshInstance)
        continue;
      if (!meshInstance->isVisible())
        continue;

      instanceCount++;

      // Get transform from MeshInstance
      const auto &transform = meshInstance->transform();
      Eigen::Matrix4d transformMatrix = transform.matrix();

      auto &node = asset.nodes.emplace_back();
      node.name = meshInstance->objectName().toStdString();

      // Convert Eigen transform to fastgltf TRS
      fastgltf::TRS trs;

      // Extract translation
      trs.translation = {static_cast<float>(transformMatrix(0, 3)),
                         static_cast<float>(transformMatrix(1, 3)),
                         static_cast<float>(transformMatrix(2, 3))};

      // Extract rotation (convert rotation matrix to quaternion)
      Eigen::Matrix3d rotationMatrix = transform.rotation();
      Eigen::Quaterniond quat(rotationMatrix);
      trs.rotation = fastgltf::math::fquat(
          static_cast<float>(quat.x()), static_cast<float>(quat.y()),
          static_cast<float>(quat.z()), static_cast<float>(quat.w()));

      // Extract scale (assuming uniform scaling for now)
      Eigen::Vector3d scale = transform.linear().diagonal();
      trs.scale = {static_cast<float>(scale.x()), static_cast<float>(scale.y()),
                   static_cast<float>(scale.z())};

      node.transform = trs;
      node.meshIndex = asset.meshes.size() - 1;

      // Add to scene
      if (!asset.scenes.empty()) {
        asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
      }
    }
  }

  qDebug() << "Added" << meshCount << "meshes with" << instanceCount
           << "instances to GLTF asset";
}

void GLTFExporter::addFrameworkToAsset(fastgltf::Asset &asset,
                                       const ChemicalStructure *structure,
                                       const ExportOptions &options) {
  // Check if structure has interactions (framework data)
  auto *interactions =
      const_cast<ChemicalStructure *>(structure)->pairInteractions();
  if (!interactions) {
    qDebug() << "No pair interactions available for framework export";
    return;
  }

  // Get fragment pairs - use default settings for now
  FragmentPairSettings pairSettings;
  auto fragmentPairs = structure->findFragmentPairs(pairSettings);
  auto uniquePairs = fragmentPairs.uniquePairs;

  if (uniquePairs.empty()) {
    qDebug() << "No fragment pairs found for framework export";
    return;
  }

  // Get interactions for the pairs
  auto interactionMap =
      interactions->getInteractionsMatchingFragments(uniquePairs);

  // Use default model for now - could be made configurable
  QString model = "CE-1P";
  auto uniqueInteractions = interactionMap.value(model, {});

  if (uniqueInteractions.empty()) {
    qDebug() << "No interactions found for framework export with model"
             << model;
    return;
  }

  // Load cylinder mesh for framework tubes
  if (m_cylinderVertices.empty()) {
    loadCylinderMesh();
  }

  if (m_cylinderVertices.empty()) {
    qDebug() << "Failed to load cylinder mesh for framework export";
    return;
  }

  // Create cylinder buffers for framework (reuse the same approach as bonds)
  std::vector<uint8_t> vertexBytes;
  vertexBytes.resize(m_cylinderVertices.size() * sizeof(float));
  std::memcpy(vertexBytes.data(), m_cylinderVertices.data(),
              vertexBytes.size());

  std::vector<uint8_t> indexBytes;
  indexBytes.resize(m_cylinderIndices.size() * sizeof(uint32_t));
  std::memcpy(indexBytes.data(), m_cylinderIndices.data(), indexBytes.size());

  // Create buffers
  auto &vertexBuffer = asset.buffers.emplace_back();
  vertexBuffer.name = "framework_vertices";
  vertexBuffer.byteLength = vertexBytes.size();
  std::vector<std::byte> vertexByteData(
      reinterpret_cast<std::byte *>(vertexBytes.data()),
      reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
  vertexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

  auto &indexBuffer = asset.buffers.emplace_back();
  indexBuffer.name = "framework_indices";
  indexBuffer.byteLength = indexBytes.size();
  std::vector<std::byte> indexByteData(
      reinterpret_cast<std::byte *>(indexBytes.data()),
      reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
  indexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

  // Create buffer views
  auto &vertexBufferView = asset.bufferViews.emplace_back();
  vertexBufferView.bufferIndex = asset.buffers.size() - 2;
  vertexBufferView.byteOffset = 0;
  vertexBufferView.byteLength = vertexBuffer.byteLength;
  vertexBufferView.byteStride = sizeof(float) * 3;
  vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

  auto &indexBufferView = asset.bufferViews.emplace_back();
  indexBufferView.bufferIndex = asset.buffers.size() - 1;
  indexBufferView.byteOffset = 0;
  indexBufferView.byteLength = indexBuffer.byteLength;
  indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

  // Create accessors
  auto &positionAccessor = asset.accessors.emplace_back();
  positionAccessor.bufferViewIndex = asset.bufferViews.size() - 2;
  positionAccessor.componentType = fastgltf::ComponentType::Float;
  positionAccessor.count = m_cylinderVertices.size() / 3;
  positionAccessor.type = fastgltf::AccessorType::Vec3;

  auto &indexAccessor = asset.accessors.emplace_back();
  indexAccessor.bufferViewIndex = asset.bufferViews.size() - 1;
  indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
  indexAccessor.count = m_cylinderIndices.size();
  indexAccessor.type = fastgltf::AccessorType::Scalar;

  // Create framework material
  auto &frameworkMaterial = asset.materials.emplace_back();
  frameworkMaterial.name = "Framework Material";
  frameworkMaterial.pbrData.baseColorFactor = {0.0f, 0.5f, 1.0f,
                                               1.0f}; // Blue framework
  frameworkMaterial.pbrData.metallicFactor = m_materialMetallic;
  frameworkMaterial.pbrData.roughnessFactor = m_materialRoughness;

  // Create framework mesh
  auto &frameworkMesh = asset.meshes.emplace_back();
  frameworkMesh.name = "Framework Tubes";

  auto &primitive = frameworkMesh.primitives.emplace_back();
  primitive.type = fastgltf::PrimitiveType::Triangles;
  primitive.attributes.emplace_back("POSITION", asset.accessors.size() - 2);
  primitive.indicesAccessor = asset.accessors.size() - 1;
  primitive.materialIndex = asset.materials.size() - 1;

  int tubeCount = 0;

  // Export framework tubes
  for (size_t uniqueIndex = 0; uniqueIndex < uniqueInteractions.size();
       ++uniqueIndex) {
    const auto *interaction = uniqueInteractions[uniqueIndex];
    if (!interaction)
      continue;

    const auto &pair = uniquePairs[uniqueIndex];

    // Get energy and apply cutoff
    double energy = interaction->getComponent("total");
    if (std::abs(energy) <= 0.0)
      continue;

    // Get positions for the fragment pair
    auto fragA = structure->getFragment(pair.index.a);
    auto fragB = structure->getFragment(pair.index.b);
    if (!fragA || !fragB)
      continue;

    // Use fragment centroids as connection points
    auto centroidA = fragA->get().centroid();
    auto centroidB = fragB->get().centroid();
    QVector3D posA(centroidA.x(), centroidA.y(), centroidA.z());
    QVector3D posB(centroidB.x(), centroidB.y(), centroidB.z());

    // Calculate framework tube parameters
    double scale =
        -energy * 0.001; // Scale factor similar to framework renderer
    if (std::abs(scale) < 1e-4)
      continue;

    float tubeRadius = std::abs(scale);
    if (tubeRadius < 0.005f)
      tubeRadius = 0.005f; // Minimum visible thickness

    QVector3D tubeVector = posB - posA;
    float tubeLength = tubeVector.length();
    tubeVector.normalize();

    QVector3D tubeCenter = (posA + posB) * 0.5f;

    // Calculate rotation to align cylinder with tube direction
    QVector3D defaultDirection(0.0f, 0.0f, 1.0f);
    QVector3D axis = QVector3D::crossProduct(defaultDirection, tubeVector);
    float angle =
        std::acos(QVector3D::dotProduct(defaultDirection, tubeVector));

    auto &node = asset.nodes.emplace_back();
    node.name = "Framework Tube";
    fastgltf::TRS trs;
    trs.translation = {tubeCenter.x(), tubeCenter.y(), tubeCenter.z()};

    // Handle rotation
    if (axis.length() > 0.001f) {
      axis.normalize();
      float s = std::sin(angle * 0.5f);
      float c = std::cos(angle * 0.5f);
      trs.rotation =
          fastgltf::math::fquat(axis.x() * s, axis.y() * s, axis.z() * s, c);
    } else {
      if (QVector3D::dotProduct(defaultDirection, tubeVector) < 0) {
        trs.rotation = fastgltf::math::fquat(1.0f, 0.0f, 0.0f, 0.0f);
      } else {
        trs.rotation = fastgltf::math::fquat(0.0f, 0.0f, 0.0f, 1.0f);
      }
    }

    trs.scale = {tubeRadius, tubeRadius, tubeLength};
    node.transform = trs;
    node.meshIndex = asset.meshes.size() - 1;

    // Add to scene
    if (!asset.scenes.empty()) {
      asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
    }

    tubeCount++;
  }

  qDebug() << "Added" << tubeCount << "framework tubes to GLTF asset";
}

void GLTFExporter::addCameraToAsset(fastgltf::Asset &asset,
                                    const cx::graphics::ExportCamera &camera) {
  // TODO: Camera export is currently broken, needs investigation
  // Implementation commented out until issues are resolved
  /*
  auto& gltfCamera = asset.cameras.emplace_back();
  gltfCamera.name = camera.name.toStdString();

  if (camera.type == cx::graphics::ExportCamera::Orthographic) {
    fastgltf::Camera::Orthographic ortho;
    ortho.xmag = camera.orthographic.xmag;
    ortho.ymag = camera.orthographic.ymag;
    ortho.znear = camera.orthographic.znear;
    ortho.zfar = camera.orthographic.zfar;
    gltfCamera.camera = ortho;
  } else {
    fastgltf::Camera::Perspective persp;
    persp.yfov = camera.perspective.yfov;
    persp.znear = camera.perspective.znear;
    if (camera.perspective.aspectRatio > 0) {
      persp.aspectRatio = camera.perspective.aspectRatio;
    }
    if (camera.perspective.zfar > camera.perspective.znear) {
      persp.zfar = camera.perspective.zfar;
    }
    gltfCamera.camera = persp;
  }

  qDebug() << "Exported camera:" << camera.name
           << (camera.type == cx::graphics::ExportCamera::Orthographic ?
  "Orthographic" : "Perspective");
  */
  qDebug() << "Camera export not implemented - TODO: fix broken camera export";
}

void GLTFExporter::addStructureByFragments(fastgltf::Asset &asset,
                                           const ChemicalStructure *structure,
                                           const ExportOptions &options,
                                           const Scene *scene) {
  qDebug() << "Using working element-based export (asset merging too complex)";

  // Just use the existing working methods
  if (options.exportAtoms) {
    auto atomIndices = structure->atomIndices();
    auto positions = structure->atomicPositionsForIndices(atomIndices);
    auto atomicNumbers = structure->atomicNumbersForIndices(atomIndices);

    if (!atomIndices.empty()) {
      // Group atoms by element for efficiency
      ankerl::unordered_dense::map<int,
                                   std::vector<std::pair<QVector3D, float>>>
          atomsByElement;

      for (int i = 0; i < atomIndices.size(); ++i) {
        auto *element = ElementData::elementFromAtomicNumber(atomicNumbers(i));
        if (!element)
          continue;

        QVector3D position(positions(0, i), positions(1, i), positions(2, i));
        float radius = getAtomDisplayRadius(structure, i, scene, options);

        atomsByElement[atomicNumbers(i)].emplace_back(position, radius);
      }

      // Add atoms to GLTF asset using the working approach
      addAtomsToAsset(asset, atomsByElement, options);
    }
  }

  // Bonds are exported via SceneExportData

  // Meshes are exported via SceneExportData

  // Framework is exported via SceneExportData cylinders when using exportScene
  // method
}

void GLTFExporter::addSpheresToAsset(
    fastgltf::Asset &asset,
    const std::vector<cx::graphics::ExportSphere> &spheres,
    const ExportOptions &options) {
  if (spheres.empty())
    return;

  // Convert vertex data to bytes
  std::vector<uint8_t> vertexBytes;
  vertexBytes.resize(m_sphereVertices.size() * sizeof(float));
  std::memcpy(vertexBytes.data(), m_sphereVertices.data(), vertexBytes.size());

  std::vector<uint8_t> indexBytes;
  indexBytes.resize(m_sphereIndices.size() * sizeof(uint32_t));
  std::memcpy(indexBytes.data(), m_sphereIndices.data(), indexBytes.size());

  // Create buffers
  auto &vertexBuffer = asset.buffers.emplace_back();
  vertexBuffer.name = "sphere_vertices";
  vertexBuffer.byteLength = vertexBytes.size();
  std::vector<std::byte> vertexByteData(
      reinterpret_cast<std::byte *>(vertexBytes.data()),
      reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
  vertexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

  auto &indexBuffer = asset.buffers.emplace_back();
  indexBuffer.name = "sphere_indices";
  indexBuffer.byteLength = indexBytes.size();
  std::vector<std::byte> indexByteData(
      reinterpret_cast<std::byte *>(indexBytes.data()),
      reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
  indexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

  // Create buffer views
  auto &vertexBufferView = asset.bufferViews.emplace_back();
  vertexBufferView.bufferIndex = asset.buffers.size() - 2;
  vertexBufferView.byteLength = vertexBytes.size();
  vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

  auto &indexBufferView = asset.bufferViews.emplace_back();
  indexBufferView.bufferIndex = asset.buffers.size() - 1;
  indexBufferView.byteLength = indexBytes.size();
  indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

  // Create accessors
  auto &positionAccessor = asset.accessors.emplace_back();
  positionAccessor.bufferViewIndex = asset.bufferViews.size() - 2;
  positionAccessor.componentType = fastgltf::ComponentType::Float;
  positionAccessor.count = m_sphereVertices.size() / 3;
  positionAccessor.type = fastgltf::AccessorType::Vec3;

  auto &indexAccessor = asset.accessors.emplace_back();
  indexAccessor.bufferViewIndex = asset.bufferViews.size() - 1;
  indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
  indexAccessor.count = m_sphereIndices.size();
  indexAccessor.type = fastgltf::AccessorType::Scalar;

  // Group spheres by element for materials
  ankerl::unordered_dense::map<QString,
                               std::vector<const cx::graphics::ExportSphere *>>
      spheresByGroup;
  for (const auto &sphere : spheres) {
    spheresByGroup[sphere.group].push_back(&sphere);
  }

  // Create separate mesh for each material group
  for (const auto &[group, groupSpheres] : spheresByGroup) {
    // Create material for this group
    auto &material = asset.materials.emplace_back();
    material.name = group.toStdString() + " Material";

    if (!groupSpheres.empty()) {
      const auto &firstSphere = *groupSpheres[0];
      material.pbrData.baseColorFactor = {
          static_cast<float>(firstSphere.color.redF()),
          static_cast<float>(firstSphere.color.greenF()),
          static_cast<float>(firstSphere.color.blueF()),
          static_cast<float>(firstSphere.color.alphaF())};
    }
    material.pbrData.metallicFactor = m_materialMetallic;
    material.pbrData.roughnessFactor = m_materialRoughness;

    // Create mesh for this group
    auto &gltfMesh = asset.meshes.emplace_back();
    gltfMesh.name = "Sphere_" + group.toStdString();

    auto &primitive = gltfMesh.primitives.emplace_back();
    primitive.type = fastgltf::PrimitiveType::Triangles;
    primitive.attributes.emplace_back("POSITION", asset.accessors.size() - 2);
    primitive.indicesAccessor = asset.accessors.size() - 1;
    primitive.materialIndex = asset.materials.size() - 1;

    // Create instances for each sphere in this group
    for (const auto *sphere : groupSpheres) {
      auto &node = asset.nodes.emplace_back();
      node.name = sphere->name.toStdString();
      node.meshIndex = asset.meshes.size() - 1;

      // Set transform (position and scale)
      fastgltf::TRS trs;
      trs.translation = {sphere->position.x(), sphere->position.y(),
                         sphere->position.z()};
      trs.rotation =
          fastgltf::math::fquat(0.0f, 0.0f, 0.0f, 1.0f); // identity quaternion
      float radius = sphere->radius * options.atomRadiusScale;
      trs.scale = {radius, radius, radius};
      node.transform = trs;

      // Add to root scene
      if (!asset.scenes.empty()) {
        asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
      }
    }
  }
}

void GLTFExporter::addCylindersToAsset(
    fastgltf::Asset &asset,
    const std::vector<cx::graphics::ExportCylinder> &cylinders,
    const ExportOptions &options) {
  if (cylinders.empty())
    return;

  // Convert cylinder vertex data to bytes (reuse existing cylinder mesh)
  std::vector<uint8_t> vertexBytes;
  vertexBytes.resize(m_cylinderVertices.size() * sizeof(float));
  std::memcpy(vertexBytes.data(), m_cylinderVertices.data(),
              vertexBytes.size());

  std::vector<uint8_t> indexBytes;
  indexBytes.resize(m_cylinderIndices.size() * sizeof(uint32_t));
  std::memcpy(indexBytes.data(), m_cylinderIndices.data(), indexBytes.size());

  // Create buffers
  auto &vertexBuffer = asset.buffers.emplace_back();
  vertexBuffer.name = "cylinder_vertices";
  vertexBuffer.byteLength = vertexBytes.size();
  std::vector<std::byte> vertexByteData(
      reinterpret_cast<std::byte *>(vertexBytes.data()),
      reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
  vertexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

  auto &indexBuffer = asset.buffers.emplace_back();
  indexBuffer.name = "cylinder_indices";
  indexBuffer.byteLength = indexBytes.size();
  std::vector<std::byte> indexByteData(
      reinterpret_cast<std::byte *>(indexBytes.data()),
      reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
  indexBuffer.data = fastgltf::sources::Array{
      .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

  // Create buffer views
  auto &vertexBufferView = asset.bufferViews.emplace_back();
  vertexBufferView.bufferIndex = asset.buffers.size() - 2;
  vertexBufferView.byteLength = vertexBytes.size();
  vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

  auto &indexBufferView = asset.bufferViews.emplace_back();
  indexBufferView.bufferIndex = asset.buffers.size() - 1;
  indexBufferView.byteLength = indexBytes.size();
  indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

  // Create accessors
  auto &positionAccessor = asset.accessors.emplace_back();
  positionAccessor.bufferViewIndex = asset.bufferViews.size() - 2;
  positionAccessor.componentType = fastgltf::ComponentType::Float;
  positionAccessor.count = m_cylinderVertices.size() / 3;
  positionAccessor.type = fastgltf::AccessorType::Vec3;

  auto &indexAccessor = asset.accessors.emplace_back();
  indexAccessor.bufferViewIndex = asset.bufferViews.size() - 1;
  indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
  indexAccessor.count = m_cylinderIndices.size();
  indexAccessor.type = fastgltf::AccessorType::Scalar;

  // Group cylinders by color for materials (each unique color gets its own
  // material)
  ankerl::unordered_dense::map<
      QRgb, std::vector<const cx::graphics::ExportCylinder *>>
      cylindersByColor;
  for (const auto &cylinder : cylinders) {
    cylindersByColor[cylinder.color.rgb()].push_back(&cylinder);
  }

  // Create separate material and mesh for each unique color
  for (const auto &[colorKey, colorCylinders] : cylindersByColor) {
    // Create material for this color
    auto &material = asset.materials.emplace_back();
    const auto &firstCylinder = *colorCylinders[0];
    QColor color = firstCylinder.color;
    material.name =
        QString("Cylinder_Material_%1").arg(color.name()).toStdString();
    material.pbrData.baseColorFactor = {
        static_cast<float>(color.redF()), static_cast<float>(color.greenF()),
        static_cast<float>(color.blueF()), static_cast<float>(color.alphaF())};
    material.pbrData.metallicFactor = m_materialMetallic;
    material.pbrData.roughnessFactor = m_materialRoughness;

    // Create mesh for this color
    auto &gltfMesh = asset.meshes.emplace_back();
    gltfMesh.name = QString("Cylinders_%1").arg(color.name()).toStdString();

    auto &primitive = gltfMesh.primitives.emplace_back();
    primitive.type = fastgltf::PrimitiveType::Triangles;
    primitive.attributes.emplace_back("POSITION", asset.accessors.size() - 2);
    primitive.indicesAccessor = asset.accessors.size() - 1;
    primitive.materialIndex = asset.materials.size() - 1;

    // Create instances for each cylinder with this color
    for (const auto *cylinder : colorCylinders) {
      auto &node = asset.nodes.emplace_back();
      node.name = cylinder->name.toStdString();
      node.meshIndex = asset.meshes.size() - 1;

      // Calculate cylinder transform (position, rotation, scale)
      QVector3D direction = cylinder->endPosition - cylinder->startPosition;
      QVector3D center =
          (cylinder->startPosition + cylinder->endPosition) / 2.0f;
      float length = direction.length();

      if (length > 0.0f) {
        direction.normalize();

        fastgltf::TRS trs;
        trs.translation = {center.x(), center.y(), center.z()};

        // Calculate rotation to align with direction (cylinder default is along
        // Z-axis)
        QVector3D defaultDirection(0, 0, 1);
        QVector3D axis = QVector3D::crossProduct(defaultDirection, direction);
        float dot = QVector3D::dotProduct(defaultDirection, direction);

        if (axis.length() > 0.001f) {
          axis.normalize();
          float angle = std::acos(std::clamp(dot, -1.0f, 1.0f));
          float sinHalfAngle = std::sin(angle / 2.0f);
          float cosHalfAngle = std::cos(angle / 2.0f);

          trs.rotation = fastgltf::math::fquat(
              axis.x() * sinHalfAngle, axis.y() * sinHalfAngle,
              axis.z() * sinHalfAngle, cosHalfAngle);
        } else {
          // Parallel or anti-parallel
          if (dot < 0) {
            // Anti-parallel: rotate 180 degrees around X-axis
            trs.rotation = fastgltf::math::fquat(1.0f, 0.0f, 0.0f, 0.0f);
          } else {
            // Parallel: identity rotation
            trs.rotation = fastgltf::math::fquat(0.0f, 0.0f, 0.0f, 1.0f);
          }
        }

        float radius = cylinder->radius * options.bondRadiusScale;
        trs.scale = {radius, radius, length};

        node.transform = trs;
      }

      // Add to root scene
      if (!asset.scenes.empty()) {
        asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
      }
    }
  }
}

void GLTFExporter::addMeshesToAsset(
    fastgltf::Asset &asset, const std::vector<cx::graphics::ExportMesh> &meshes,
    const ExportOptions &options) {
  if (meshes.empty())
    return;

  for (const auto &exportMesh : meshes) {
    if (exportMesh.vertices.empty() || exportMesh.indices.empty())
      continue;

    // Convert vertex and index data to bytes
    std::vector<uint8_t> vertexBytes;
    vertexBytes.resize(exportMesh.vertices.size() * sizeof(float));
    std::memcpy(vertexBytes.data(), exportMesh.vertices.data(),
                vertexBytes.size());

    std::vector<uint8_t> normalBytes;
    if (!exportMesh.normals.empty()) {
      normalBytes.resize(exportMesh.normals.size() * sizeof(float));
      std::memcpy(normalBytes.data(), exportMesh.normals.data(),
                  normalBytes.size());
    }

    std::vector<uint8_t> colorBytes;
    if (!exportMesh.colors.empty()) {
      colorBytes.resize(exportMesh.colors.size() * sizeof(float));
      std::memcpy(colorBytes.data(), exportMesh.colors.data(),
                  colorBytes.size());
    }

    std::vector<uint8_t> indexBytes;
    indexBytes.resize(exportMesh.indices.size() * sizeof(uint32_t));
    std::memcpy(indexBytes.data(), exportMesh.indices.data(),
                indexBytes.size());

    // Create buffers
    auto &vertexBuffer = asset.buffers.emplace_back();
    vertexBuffer.name = exportMesh.name.toStdString() + "_vertices";
    vertexBuffer.byteLength = vertexBytes.size();
    std::vector<std::byte> vertexByteData(
        reinterpret_cast<std::byte *>(vertexBytes.data()),
        reinterpret_cast<std::byte *>(vertexBytes.data()) + vertexBytes.size());
    vertexBuffer.data = fastgltf::sources::Array{
        .bytes = fastgltf::StaticVector<std::byte>::fromVector(vertexByteData)};

    // Create normal buffer if normals exist
    if (!normalBytes.empty()) {
      auto &normalBuffer = asset.buffers.emplace_back();
      normalBuffer.name = exportMesh.name.toStdString() + "_normals";
      normalBuffer.byteLength = normalBytes.size();
      std::vector<std::byte> normalByteData(
          reinterpret_cast<std::byte *>(normalBytes.data()),
          reinterpret_cast<std::byte *>(normalBytes.data()) +
              normalBytes.size());
      normalBuffer.data = fastgltf::sources::Array{
          .bytes =
              fastgltf::StaticVector<std::byte>::fromVector(normalByteData)};
    }

    // Create color buffer if vertex colors exist
    if (!colorBytes.empty()) {
      auto &colorBuffer = asset.buffers.emplace_back();
      colorBuffer.name = exportMesh.name.toStdString() + "_colors";
      colorBuffer.byteLength = colorBytes.size();
      std::vector<std::byte> colorByteData(
          reinterpret_cast<std::byte *>(colorBytes.data()),
          reinterpret_cast<std::byte *>(colorBytes.data()) + colorBytes.size());
      colorBuffer.data = fastgltf::sources::Array{
          .bytes =
              fastgltf::StaticVector<std::byte>::fromVector(colorByteData)};
    }

    auto &indexBuffer = asset.buffers.emplace_back();
    indexBuffer.name = exportMesh.name.toStdString() + "_indices";
    indexBuffer.byteLength = indexBytes.size();
    std::vector<std::byte> indexByteData(
        reinterpret_cast<std::byte *>(indexBytes.data()),
        reinterpret_cast<std::byte *>(indexBytes.data()) + indexBytes.size());
    indexBuffer.data = fastgltf::sources::Array{
        .bytes = fastgltf::StaticVector<std::byte>::fromVector(indexByteData)};

    // Create buffer views
    int bufferCount = 2; // Always have vertex and index buffers
    if (!normalBytes.empty())
      bufferCount++;
    if (!colorBytes.empty())
      bufferCount++;

    size_t vertexBufferIndex = asset.buffers.size() - bufferCount;
    size_t normalBufferIndex = vertexBufferIndex + 1;
    size_t colorBufferIndex =
        normalBufferIndex + (!normalBytes.empty() ? 1 : 0);
    size_t indexBufferIndex = asset.buffers.size() - 1;

    auto &vertexBufferView = asset.bufferViews.emplace_back();
    vertexBufferView.bufferIndex = vertexBufferIndex;
    vertexBufferView.byteLength = vertexBytes.size();
    vertexBufferView.target = fastgltf::BufferTarget::ArrayBuffer;

    size_t normalBufferViewIndex = 0;
    if (!normalBytes.empty()) {
      auto &normalBufferView = asset.bufferViews.emplace_back();
      normalBufferView.bufferIndex = normalBufferIndex;
      normalBufferView.byteLength = normalBytes.size();
      normalBufferView.target = fastgltf::BufferTarget::ArrayBuffer;
      normalBufferViewIndex = asset.bufferViews.size() - 1;
    }

    size_t colorBufferViewIndex = 0;
    if (!colorBytes.empty()) {
      auto &colorBufferView = asset.bufferViews.emplace_back();
      colorBufferView.bufferIndex = colorBufferIndex;
      colorBufferView.byteLength = colorBytes.size();
      colorBufferView.target = fastgltf::BufferTarget::ArrayBuffer;
      colorBufferViewIndex = asset.bufferViews.size() - 1;
    }

    auto &indexBufferView = asset.bufferViews.emplace_back();
    indexBufferView.bufferIndex = indexBufferIndex;
    indexBufferView.byteLength = indexBytes.size();
    indexBufferView.target = fastgltf::BufferTarget::ElementArrayBuffer;

    // Create accessors - track indices properly
    size_t vertexBufferViewIndex =
        asset.bufferViews.size() -
        ((!colorBytes.empty() ? 1 : 0) + (!normalBytes.empty() ? 1 : 0) + 2);
    size_t indexBufferViewIndex = asset.bufferViews.size() - 1;

    // Position accessor
    auto &positionAccessor = asset.accessors.emplace_back();
    positionAccessor.bufferViewIndex = vertexBufferViewIndex;
    positionAccessor.componentType = fastgltf::ComponentType::Float;
    positionAccessor.count = exportMesh.vertices.size() / 3;
    positionAccessor.type = fastgltf::AccessorType::Vec3;
    size_t positionAccessorIndex = asset.accessors.size() - 1;

    size_t normalAccessorIndex = 0;
    if (!normalBytes.empty()) {
      auto &normalAccessor = asset.accessors.emplace_back();
      normalAccessor.bufferViewIndex = normalBufferViewIndex;
      normalAccessor.componentType = fastgltf::ComponentType::Float;
      normalAccessor.count = exportMesh.normals.size() / 3;
      normalAccessor.type = fastgltf::AccessorType::Vec3;
      normalAccessorIndex = asset.accessors.size() - 1;
    }

    size_t colorAccessorIndex = 0;
    if (!colorBytes.empty()) {
      auto &colorAccessor = asset.accessors.emplace_back();
      colorAccessor.bufferViewIndex = colorBufferViewIndex;
      colorAccessor.componentType = fastgltf::ComponentType::Float;
      colorAccessor.count = exportMesh.colors.size() / 3;
      colorAccessor.type = fastgltf::AccessorType::Vec3;
      colorAccessorIndex = asset.accessors.size() - 1;
    }

    auto &indexAccessor = asset.accessors.emplace_back();
    indexAccessor.bufferViewIndex = indexBufferViewIndex;
    indexAccessor.componentType = fastgltf::ComponentType::UnsignedInt;
    indexAccessor.count = exportMesh.indices.size();
    indexAccessor.type = fastgltf::AccessorType::Scalar;
    size_t indexAccessorIndex = asset.accessors.size() - 1;

    // Create material
    auto &material = asset.materials.emplace_back();
    material.name = exportMesh.name.toStdString() + "_material";

    // Use fallback color or white if vertex colors are present
    if (!exportMesh.colors.empty()) {
      // White base color when using vertex colors
      material.pbrData.baseColorFactor = {1.0f, 1.0f, 1.0f, exportMesh.opacity};
    } else {
      // Use fallback color
      material.pbrData.baseColorFactor = {
          static_cast<float>(exportMesh.fallbackColor.redF()),
          static_cast<float>(exportMesh.fallbackColor.greenF()),
          static_cast<float>(exportMesh.fallbackColor.blueF()),
          exportMesh.opacity};
    }

    material.pbrData.metallicFactor = m_materialMetallic;
    material.pbrData.roughnessFactor = m_materialRoughness;

    // Create mesh
    auto &gltfMesh = asset.meshes.emplace_back();
    gltfMesh.name = exportMesh.name.toStdString();

    auto &primitive = gltfMesh.primitives.emplace_back();
    primitive.type = fastgltf::PrimitiveType::Triangles;

    // Add position attribute using tracked index
    primitive.attributes.emplace_back("POSITION", positionAccessorIndex);

    // Add normal attribute if available
    if (!normalBytes.empty()) {
      primitive.attributes.emplace_back("NORMAL", normalAccessorIndex);
    }

    // Add vertex color attribute if available
    if (!colorBytes.empty()) {
      primitive.attributes.emplace_back("COLOR_0", colorAccessorIndex);
    }

    primitive.indicesAccessor = indexAccessorIndex;
    primitive.materialIndex = asset.materials.size() - 1;

    // Create node
    auto &node = asset.nodes.emplace_back();
    node.name = exportMesh.name.toStdString();
    node.meshIndex = asset.meshes.size() - 1;

    // Add to root scene
    if (!asset.scenes.empty()) {
      asset.scenes[0].nodeIndices.push_back(asset.nodes.size() - 1);
    }
  }
}

} // namespace cx::core
