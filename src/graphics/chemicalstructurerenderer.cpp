#include "chemicalstructurerenderer.h"
#include "elementdata.h"
#include "graphics.h"
#include "mesh.h"

namespace cx::graphics {
ChemicalStructureRenderer::ChemicalStructureRenderer(
    ChemicalStructure *structure, QObject *parent)
    : QObject(parent), m_structure(structure) {

  m_ellipsoidRenderer = new EllipsoidRenderer();
  m_cylinderRenderer = new CylinderRenderer();
  m_labelRenderer = new BillboardRenderer();
  m_lineRenderer = new LineRenderer();
  m_highlightRenderer = new LineRenderer();
  m_pointCloudRenderer = new PointCloudRenderer();
  m_frameworkRenderer = new FrameworkRenderer(m_structure);

  connect(m_structure, &ChemicalStructure::childAdded, this,
          &ChemicalStructureRenderer::childAddedToStructure);
  connect(m_structure, &ChemicalStructure::childRemoved, this,
          &ChemicalStructureRenderer::childRemovedFromStructure);
  connect(m_structure, &ChemicalStructure::atomsChanged, [&](){
    m_labelsNeedsUpdate = true;
    m_atomsNeedsUpdate = true;
    m_bondsNeedsUpdate = true;
  });
  initStructureChildren();

  m_propertyColorMaps = {
      {"None", ColorMapName::CE_None},
      {"dnorm", ColorMapName::CE_bwr},
      {"di", ColorMapName::CE_rgb},
      {"de", ColorMapName::CE_rgb},
      {"di_norm", ColorMapName::CE_bwr},
      {"shape_index", ColorMapName::CE_rgb},
      {"curvedness", ColorMapName::CE_rgb},
  };
}

void ChemicalStructureRenderer::initStructureChildren() {
  m_meshesNeedsUpdate = true;
}

void ChemicalStructureRenderer::setSelectionHandler(RenderSelection *ptr) {
  m_selectionHandler = ptr;
}

void ChemicalStructureRenderer::setShowHydrogenAtoms(bool show) {
  if (show != m_showHydrogens) {
    m_showHydrogens = show;
    m_atomsNeedsUpdate = true;
    m_bondsNeedsUpdate = true;
  }
}

bool ChemicalStructureRenderer::showHydrogenAtoms() const {
  return m_showHydrogens;
}

void ChemicalStructureRenderer::toggleShowHydrogenAtoms() {
  setShowHydrogenAtoms(!m_showHydrogens);
}

void ChemicalStructureRenderer::setShowAtomLabels(bool show) {
  if (show != m_showAtomLabels) {
    m_showAtomLabels = show;
    m_labelsNeedsUpdate = true;
  }
}

bool ChemicalStructureRenderer::showAtomLabels() const {
  return m_showAtomLabels;
}

void ChemicalStructureRenderer::toggleShowAtomLabels() {
  setShowAtomLabels(!m_showAtomLabels);
}

void ChemicalStructureRenderer::setShowSuppressedAtoms(bool show) {
  if (show != m_showSuppressedAtoms) {
    m_showSuppressedAtoms = show;
    m_atomsNeedsUpdate = true;
    m_bondsNeedsUpdate = true;
  }
}

bool ChemicalStructureRenderer::showSuppressedAtoms() const {
  return m_showSuppressedAtoms;
}

void ChemicalStructureRenderer::toggleShowSuppressedAtoms() {
  setShowSuppressedAtoms(!m_showSuppressedAtoms);
}

bool ChemicalStructureRenderer::shouldSkipAtom(int index) const {
  const auto &numbers = m_structure->atomicNumbers();
  auto atomIndex = m_structure->indexToGenericIndex(index);

  if (!showHydrogenAtoms() && (numbers(index) == 1)) {
    return true;
  } else if (!showSuppressedAtoms() &&
             (m_structure->testAtomFlag(atomIndex, AtomFlag::Suppressed))) {
    return true;
  }
  return false;
}

void ChemicalStructureRenderer::setAtomStyle(AtomDrawingStyle style) {
  if (m_atomStyle == style)
    return;
  m_atomStyle = style;
  m_atomsNeedsUpdate = true;
}

AtomDrawingStyle ChemicalStructureRenderer::atomStyle() const {
  return m_atomStyle;
}

void ChemicalStructureRenderer::setBondStyle(BondDrawingStyle style) {
  if (m_bondStyle == style)
    return;
  m_bondStyle = style;
  m_bondsNeedsUpdate = true;
}

BondDrawingStyle ChemicalStructureRenderer::bondStyle() const {
  return m_bondStyle;
}

[[nodiscard]] float ChemicalStructureRenderer::bondThickness() const {
  return ElementData::elementFromAtomicNumber(1)->covRadius() *
         m_bondThicknessFactor;
}

void ChemicalStructureRenderer::updateLabels() {
  m_labelsNeedsUpdate = true;
  handleLabelsUpdate();
}

void ChemicalStructureRenderer::updateAtoms() {
  m_atomsNeedsUpdate = true;
  handleAtomsUpdate();
}

void ChemicalStructureRenderer::updateBonds() {
  m_bondsNeedsUpdate = true;
  handleBondsUpdate();
}

void ChemicalStructureRenderer::updateMeshes() {
  m_meshesNeedsUpdate = true;
  emit meshesChanged();
}

QList<TextLabel> ChemicalStructureRenderer::getCurrentLabels() {
  QList<TextLabel> result;
  if (m_showAtomLabels) {
    const auto &atomLabels = m_structure->labels();
    const auto &positions = m_structure->atomicPositions();
    for (int i = 0; i < m_structure->numberOfAtoms(); i++) {
      auto idx = m_structure->indexToGenericIndex(i);
      if (m_structure->testAtomFlag(idx, AtomFlag::Contact))
        continue;
      QVector3D pos(positions(0, i), positions(1, i), positions(2, i));
      result.append(TextLabel{atomLabels[i], pos});
    }
  }
  return result;
}

void ChemicalStructureRenderer::handleLabelsUpdate() {
  if (!m_structure)
    return;
  if (!m_labelsNeedsUpdate)
    return;

  m_labelRenderer->clear();
  auto labels = getCurrentLabels();
  if (labels.empty())
    return;

  m_labelRenderer->beginUpdates();
  for (const auto &label : labels) {
    cx::graphics::addTextToBillboardRenderer(*m_labelRenderer, label.position,
                                             label.text);
  }
  m_labelRenderer->endUpdates();
  m_labelsNeedsUpdate = false;
}

void ChemicalStructureRenderer::handleAtomsUpdate() {
  if (!m_structure)
    return;
  if (!m_atomsNeedsUpdate)
    return;

  if (m_selectionHandler) {
    m_selectionHandler->clear(SelectionType::Atom);
  }

  m_ellipsoidRenderer->clear();

  const auto &positions = m_structure->atomicPositions();

  const auto covRadii = m_structure->covalentRadii();
  const auto vdwRadii = m_structure->vdwRadii();

  for (int i = 0; i < m_structure->numberOfAtoms(); i++) {

    if (shouldSkipAtom(i))
      continue;
    auto idx = m_structure->indexToGenericIndex(i);
    auto color = m_structure->atomColor(idx);
    float radius = covRadii(i) * 0.5;

    if (atomStyle() == AtomDrawingStyle::RoundCapped) {
      radius = bondThickness();
    } else if (atomStyle() == AtomDrawingStyle::VanDerWaalsSphere) {
      radius = vdwRadii(i);
    }
    if (m_structure->testAtomFlag(idx, AtomFlag::Contact))
      color = color.lighter();

    quint32 selectionId{0};
    QVector3D selectionIdColor;
    if (m_selectionHandler) {
      selectionId = m_selectionHandler->add(SelectionType::Atom, i);
      selectionIdColor = m_selectionHandler->getColorFromId(selectionId);
    }
    QVector3D position(positions(0, i), positions(1, i), positions(2, i));
    cx::graphics::addSphereToEllipsoidRenderer(
        m_ellipsoidRenderer, position, color, radius, selectionIdColor,
        m_structure->atomFlagsSet(idx, AtomFlag::Selected));
  }
  m_atomsNeedsUpdate = false;
}

void ChemicalStructureRenderer::handleBondsUpdate() {
  if (!m_structure)
    return;
  if (!m_bondsNeedsUpdate)
    return;
  if (m_selectionHandler) {
    m_selectionHandler->clear(SelectionType::Bond);
  }

  m_lineRenderer->clear();
  m_cylinderRenderer->clear();

  float radius = bondThickness();
  const auto &atomPositions = m_structure->atomicPositions();
  const auto &covalentBonds = m_structure->covalentBonds();

  for (int bondIndex = 0; bondIndex < covalentBonds.size(); bondIndex++) {
    const auto [i, j] = covalentBonds[bondIndex];
    if (shouldSkipAtom(i) || shouldSkipAtom(j))
      continue;

    auto idxA = m_structure->indexToGenericIndex(i);
    auto idxB = m_structure->indexToGenericIndex(j);
    QVector3D pointA(atomPositions(0, i), atomPositions(1, i),
                     atomPositions(2, i));
    QVector3D pointB(atomPositions(0, j), atomPositions(1, j),
                     atomPositions(2, j));

    auto colorA = m_structure->atomColor(idxA);
    auto colorB = m_structure->atomColor(idxB);

    bool selectedA = m_structure->atomFlagsSet(idxA, AtomFlag::Selected);
    bool selectedB = m_structure->atomFlagsSet(idxB, AtomFlag::Selected);

    quint32 bond_id{0};
    QVector3D id_color;

    if (m_selectionHandler) {
      bond_id = m_selectionHandler->add(SelectionType::Bond, bondIndex);
      id_color = m_selectionHandler->getColorFromId(bond_id);
    }

    if (bondStyle() == BondDrawingStyle::Line) {
      cx::graphics::addLineToLineRenderer(
          *m_lineRenderer, pointA, 0.5 * pointA + 0.5 * pointB,
          DrawingStyleConstants::bondLineWidth, colorA);
      cx::graphics::addLineToLineRenderer(
          *m_lineRenderer, pointB, 0.5 * pointA + 0.5 * pointB,
          DrawingStyleConstants::bondLineWidth, colorB);
    } else {
      cx::graphics::addCylinderToCylinderRenderer(
          m_cylinderRenderer, pointA, pointB, colorA, colorB, radius, id_color,
          selectedA, selectedB);
    }
  }
  m_bondsNeedsUpdate = false;
}

void ChemicalStructureRenderer::beginUpdates() {
  m_lineRenderer->beginUpdates();
  m_cylinderRenderer->beginUpdates();
  m_ellipsoidRenderer->beginUpdates();
  m_highlightRenderer->beginUpdates();
}

void ChemicalStructureRenderer::endUpdates() {
  m_lineRenderer->endUpdates();
  m_cylinderRenderer->endUpdates();
  m_ellipsoidRenderer->endUpdates();
  m_highlightRenderer->endUpdates();
}

bool ChemicalStructureRenderer::needsUpdate() {
  // TODO check for efficiency in non-granular toggle like this
  return m_atomsNeedsUpdate || m_bondsNeedsUpdate || m_meshesNeedsUpdate ||
         m_labelsNeedsUpdate;
}

void ChemicalStructureRenderer::draw(bool forPicking) {
  if (needsUpdate()) {
    beginUpdates();
    if (m_labelsNeedsUpdate)
      updateLabels();
    if (m_atomsNeedsUpdate)
      updateAtoms();
    if (m_bondsNeedsUpdate)
      updateBonds();
    if (m_meshesNeedsUpdate)
      updateMeshes();
    endUpdates();
  }

  const auto storedRenderMode = m_uniforms.u_renderMode;

  if (forPicking) {
    m_uniforms.u_renderMode = 0;
    m_uniforms.u_selectionMode = true;
  }

  m_ellipsoidRenderer->bind();
  m_uniforms.apply(m_ellipsoidRenderer);
  m_ellipsoidRenderer->draw();
  m_ellipsoidRenderer->release();

  m_cylinderRenderer->bind();
  m_uniforms.apply(m_cylinderRenderer);
  m_cylinderRenderer->draw();
  m_cylinderRenderer->release();

  m_lineRenderer->bind();
  m_uniforms.apply(m_lineRenderer);
  m_lineRenderer->draw();
  m_lineRenderer->release();

  m_labelRenderer->bind();
  m_uniforms.apply(m_labelRenderer);
  m_labelRenderer->draw();
  m_labelRenderer->release();

  handleMeshesUpdate();

  m_highlightRenderer->bind();
  m_uniforms.apply(m_highlightRenderer);
  m_highlightRenderer->draw();
  m_highlightRenderer->release();

  for (auto *meshRenderer : m_meshRenderers) {
    meshRenderer->bind();
    m_uniforms.apply(meshRenderer);
    meshRenderer->draw();
    meshRenderer->release();
  }

  m_pointCloudRenderer->bind();
  m_uniforms.apply(m_pointCloudRenderer);
  m_pointCloudRenderer->draw();
  m_pointCloudRenderer->release();

  if (forPicking) {
    m_uniforms.u_renderMode = storedRenderMode;
    m_uniforms.u_selectionMode = false;
  }

  m_frameworkRenderer->draw();
}

void ChemicalStructureRenderer::updateRendererUniforms(
    const RendererUniforms &uniforms) {
  m_uniforms = uniforms;
  m_frameworkRenderer->updateRendererUniforms(uniforms);
}

void ChemicalStructureRenderer::clearMeshRenderers() {
  for (auto *r : m_meshRenderers) {
    delete r;
  }
  m_meshRenderers.clear();
}

void ChemicalStructureRenderer::handleMeshesUpdate() {
  if (!m_meshesNeedsUpdate)
    return;

  // TODO re-use mesh renderers
  m_meshRenderers.clear();
  m_meshIndexToMesh.clear();
  m_pointCloudRenderer->clear();
  m_highlightRenderer->clear();

  if (m_selectionHandler) {
    m_selectionHandler->clear(SelectionType::Surface);
  }
  for (auto *child : m_structure->children()) {
    auto *mesh = qobject_cast<Mesh *>(child);
    if (!mesh)
      continue;

    if (mesh->numberOfFaces() == 0) {
      m_pointCloudRenderer->addPoints(
          cx::graphics::makePointCloudVertices(*mesh));
    } else {
      MeshInstanceRenderer *instanceRenderer = new MeshInstanceRenderer(mesh);
      instanceRenderer->beginUpdates();
      const auto &availableProperties = instanceRenderer->availableProperties();
      for (auto *meshChild : child->children()) {
        auto *meshInstance = qobject_cast<MeshInstance *>(meshChild);
        if (!meshInstance || !meshInstance->isVisible())
          continue;

        int propertyIndex =
            availableProperties.indexOf(meshInstance->getSelectedProperty());
        // TODO transparency
        float alpha = meshInstance->isTransparent() ? 0.8 : 1.0;

        QVector3D selectionColor;

        if (m_selectionHandler) {
          auto selectionId = m_selectionHandler->add(SelectionType::Surface,
                                                     m_meshIndexToMesh.size());
          m_meshIndexToMesh.push_back(meshInstance);
          selectionColor = m_selectionHandler->getColorFromId(selectionId);
        }

        MeshInstanceVertex v(meshInstance->translationVector(),
                             meshInstance->rotationMatrix(), selectionColor,
                             propertyIndex, alpha);
        instanceRenderer->addInstance(v);

        {
          // face highlights
          QColor color = Qt::red;
          for (const int v : mesh->vertexHighlights()) {
            const auto vertex = meshInstance->vertexVector3D(v);
            const auto normal = meshInstance->vertexNormalVector3D(v);
            cx::graphics::addLineToLineRenderer(*m_highlightRenderer, vertex,
                                                vertex + normal, 1.0, color);
          }
        }
        instanceRenderer->endUpdates();
        mesh->setRendererIndex(m_meshRenderers.size());
        m_meshRenderers.push_back(instanceRenderer);
      }
    }
  }
  m_meshesNeedsUpdate = false;
}

void ChemicalStructureRenderer::childVisibilityChanged() {
  // TODO more granularity
  updateMeshes();
}

void ChemicalStructureRenderer::childPropertyChanged() {
  // TODO more granularity
  updateMeshes();
}

void ChemicalStructureRenderer::childAddedToStructure(QObject *child) {
  qDebug() << "Child added to structure called" << child;
  qDebug() << "Class Name:" << child->metaObject()->className();
  qDebug() << "Is MeshInstance:"
           << (dynamic_cast<MeshInstance *>(child) != nullptr);
  auto *mesh = qobject_cast<Mesh *>(child);
  auto *meshInstance = qobject_cast<MeshInstance *>(child);
  if (mesh) {
    connect(mesh, &Mesh::visibilityChanged, this,
            &ChemicalStructureRenderer::childVisibilityChanged);
    connect(mesh, &Mesh::selectedPropertyChanged, this,
            &ChemicalStructureRenderer::childPropertyChanged);
    connect(mesh, &Mesh::transparencyChanged, this,
            &ChemicalStructureRenderer::childPropertyChanged);
  } else if (meshInstance) {
    qDebug() << "Mesh instance added";
  }
  updateMeshes();
}

void ChemicalStructureRenderer::childRemovedFromStructure(QObject *child) {
  qDebug() << "Child removed @" << child << "TODO, for now bulk reset";
  auto *mesh = qobject_cast<Mesh *>(child);
  if (mesh) {
    qDebug() << "Child removed (mesh) from structure, disconnected";
    disconnect(mesh, &Mesh::visibilityChanged, this,
               &ChemicalStructureRenderer::childVisibilityChanged);
    m_meshesNeedsUpdate = true;
  }
}

void ChemicalStructureRenderer::setFrameworkOptions(
    const FrameworkOptions &options) {
  m_frameworkOptions = options;
  m_frameworkRenderer->setOptions(m_frameworkOptions);
}

MeshInstance *ChemicalStructureRenderer::getMeshInstance(size_t index) const {
  if (index >= m_meshIndexToMesh.size())
    return nullptr;
  return m_meshIndexToMesh.at(index);
}

} // namespace cx::graphics
