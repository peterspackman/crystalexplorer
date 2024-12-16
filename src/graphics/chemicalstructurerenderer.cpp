#include "chemicalstructurerenderer.h"
#include "elementdata.h"
#include "graphics.h"
#include "mesh.h"
#include "settings.h"
#include <iostream>

namespace cx::graphics {
ChemicalStructureRenderer::ChemicalStructureRenderer(
    ChemicalStructure *structure, QObject *parent)
    : QObject(parent), m_structure(structure) {

  m_ellipsoidRenderer = new EllipsoidRenderer();
  m_cylinderRenderer = new CylinderRenderer();
  m_labelRenderer = new BillboardRenderer();
  m_bondLineRenderer = new LineRenderer();
  m_cellLinesRenderer = new LineRenderer();
  m_highlightRenderer = new LineRenderer();
  m_frameworkRenderer = new FrameworkRenderer(m_structure);

  connect(m_structure, &ChemicalStructure::childAdded, this,
          &ChemicalStructureRenderer::childAddedToStructure);
  connect(m_structure, &ChemicalStructure::childRemoved, this,
          &ChemicalStructureRenderer::childRemovedFromStructure);
  connect(m_structure, &ChemicalStructure::atomsChanged,
          [&]() { forceUpdates(); });
  initStructureChildren();
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

void ChemicalStructureRenderer::setShowHydrogenAtomEllipsoids(bool show) {
  if (show != m_showHydrogenAtomEllipsoids) {
    m_showHydrogenAtomEllipsoids = show;
    m_atomsNeedsUpdate = true;
  }
}

bool ChemicalStructureRenderer::showHydrogenAtomEllipsoids() const {
  return m_showHydrogenAtomEllipsoids;
}

void ChemicalStructureRenderer::toggleShowHydrogenAtomEllipsoids() {
  setShowHydrogenAtomEllipsoids(!m_showHydrogenAtomEllipsoids);
}

void ChemicalStructureRenderer::setShowCells(bool show) {
  if (show != m_showCells) {
    m_showCells = show;
    m_cellsNeedsUpdate = true;
  }
}

bool ChemicalStructureRenderer::showCells() const { return m_showCells; }

void ChemicalStructureRenderer::toggleShowCells() {
  setShowCells(!m_showCells);
}

void ChemicalStructureRenderer::setShowMultipleCells(bool show) {
  if (show != m_showMultipleCells) {
    m_showMultipleCells = show;
    m_cellsNeedsUpdate = true;
  }
}

bool ChemicalStructureRenderer::showMultipleCells() const {
  return m_showMultipleCells;
}

void ChemicalStructureRenderer::toggleShowMultipleCells() {
  setShowCells(!m_showMultipleCells);
}

void ChemicalStructureRenderer::setAtomLabelOptions(
    const AtomLabelOptions &options) {
  if (options != m_atomLabelOptions) {
    m_atomLabelOptions = options;
    m_labelsNeedsUpdate = true;
  }
}

const AtomLabelOptions &ChemicalStructureRenderer::atomLabelOptions() const {
  return m_atomLabelOptions;
}

void ChemicalStructureRenderer::toggleShowAtomLabels() {
  auto options = m_atomLabelOptions;
  options.showAtoms = !options.showAtoms;
  setAtomLabelOptions(options);
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

void ChemicalStructureRenderer::setDrawingStyle(DrawingStyle style) {
  m_drawingStyle = style;
  setAtomStyle(atomStyleForDrawingStyle(style));
  setBondStyle(bondStyleForDrawingStyle(style));
  m_atomsNeedsUpdate = true;
  m_bondsNeedsUpdate = true;
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
  float bondThicknessFactor =
      settings::readSetting(settings::keys::BOND_THICKNESS).toInt() / 100.0;
  return ElementData::elementFromAtomicNumber(1)->covRadius() *
         bondThicknessFactor;
}

void ChemicalStructureRenderer::forceUpdates() {
  m_labelsNeedsUpdate = true;
  m_atomsNeedsUpdate = true;
  m_bondsNeedsUpdate = true;
  m_meshesNeedsUpdate = true;
  m_frameworkRenderer->forceUpdates();
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

void ChemicalStructureRenderer::updateCells() {
  m_cellsNeedsUpdate = true;
  handleCellsUpdate();
}

QList<TextLabel> ChemicalStructureRenderer::getCurrentLabels() {
  QList<TextLabel> result;
  if (m_atomLabelOptions.showAtoms) {
    const auto &atomLabels = m_structure->labels();
    const auto &positions = m_structure->atomicPositions();
    for (int i = 0; i < m_structure->numberOfAtoms(); i++) {
      if (shouldSkipAtom(i))
        continue;
      auto idx = m_structure->indexToGenericIndex(i);
      if (m_structure->testAtomFlag(idx, AtomFlag::Contact))
        continue;
      QVector3D pos(positions(0, i), positions(1, i), positions(2, i));
      result.append(TextLabel{atomLabels[i], pos});
    }
  }
  if (m_atomLabelOptions.showFragment) {
    const auto &fragments = m_structure->getFragments();
    for (const auto &[fragmentIndex, fragment] : fragments) {
      auto centroid = fragment.centroid();
      QVector3D pos(centroid.x(), centroid.y(), centroid.z());
      result.append(TextLabel{
          m_structure->getFragmentLabel(fragment.asymmetricFragmentIndex),
          pos});
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

void ChemicalStructureRenderer::handleCellsUpdate() {
  if (!m_structure)
    return;
  if (!m_cellsNeedsUpdate)
    return;

  m_cellLinesRenderer->clear();

  if (!m_showCells) {
    m_cellsNeedsUpdate = false;
    return;
  }

  QVector3D a(1, 0, 0);
  QVector3D b(0, 1, 0);
  QVector3D c(0, 0, 1);

  const auto &unitCell = m_structure->cellVectors();

  const CellIndex origin = CellIndex{0, 0, 0};
  CellIndexSet cells{origin};
  if (m_showMultipleCells) {
    auto extraCells = m_structure->occupiedCells();
    cells.insert(extraCells.begin(), extraCells.end());
  }

  a = QVector3D(unitCell(0, 0), unitCell(1, 0), unitCell(2, 0));
  b = QVector3D(unitCell(0, 1), unitCell(1, 1), unitCell(2, 1));
  c = QVector3D(unitCell(0, 2), unitCell(1, 2), unitCell(2, 2));
  const QColor A_AXISCOLOR =
      settings::readSetting(settings::keys::CE_RED_COLOR).toString();
  const QColor B_AXISCOLOR =
      settings::readSetting(settings::keys::CE_GREEN_COLOR).toString();
  const QColor C_AXISCOLOR =
      settings::readSetting(settings::keys::CE_BLUE_COLOR).toString();
  const QColor UNITCELLCOLOR = QColor("#646464");

  CellIndexPairSet drawnLines;

  auto drawLine = [&](const CellIndex &start, const CellIndex &end,
                      const QColor &color) {
    auto line = std::minmax(start, end);
    if (drawnLines.insert(line).second) {
      QVector3D startPos = start.x * a + start.y * b + start.z * c;
      QVector3D endPos = end.x * a + end.y * b + end.z * c;
      cx::graphics::addLineToLineRenderer(
          *m_cellLinesRenderer, startPos, endPos,
          DrawingStyleConstants::unitCellLineWidth, color);
    }
  };

  for (const auto &cell : cells) {
    QColor aColor = (cell == CellIndex{0, 0, 0}) ? A_AXISCOLOR : UNITCELLCOLOR;
    QColor bColor = (cell == CellIndex{0, 0, 0}) ? B_AXISCOLOR : UNITCELLCOLOR;
    QColor cColor = (cell == CellIndex{0, 0, 0}) ? C_AXISCOLOR : UNITCELLCOLOR;

    // Draw 12 edges of the parallelepiped
    drawLine(cell, CellIndex{cell.x + 1, cell.y, cell.z}, aColor);
    drawLine(cell, CellIndex{cell.x, cell.y + 1, cell.z}, bColor);
    drawLine(cell, CellIndex{cell.x, cell.y, cell.z + 1}, cColor);
    drawLine(CellIndex{cell.x + 1, cell.y, cell.z},
             CellIndex{cell.x + 1, cell.y + 1, cell.z}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x + 1, cell.y, cell.z},
             CellIndex{cell.x + 1, cell.y, cell.z + 1}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x, cell.y + 1, cell.z},
             CellIndex{cell.x + 1, cell.y + 1, cell.z}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x, cell.y + 1, cell.z},
             CellIndex{cell.x, cell.y + 1, cell.z + 1}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x, cell.y, cell.z + 1},
             CellIndex{cell.x + 1, cell.y, cell.z + 1}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x, cell.y, cell.z + 1},
             CellIndex{cell.x, cell.y + 1, cell.z + 1}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x + 1, cell.y + 1, cell.z},
             CellIndex{cell.x + 1, cell.y + 1, cell.z + 1}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x + 1, cell.y, cell.z + 1},
             CellIndex{cell.x + 1, cell.y + 1, cell.z + 1}, UNITCELLCOLOR);
    drawLine(CellIndex{cell.x, cell.y + 1, cell.z + 1},
             CellIndex{cell.x + 1, cell.y + 1, cell.z + 1}, UNITCELLCOLOR);
  }

  m_cellsNeedsUpdate = false;
}

AggregateIndex
ChemicalStructureRenderer::getAggregateIndex(size_t index) const {
  if (index < m_aggregateIndices.size())
    return m_aggregateIndices[index];
  return AggregateIndex{};
}

void ChemicalStructureRenderer::addAggregateRepresentations() {
  if (!(m_drawingStyle == DrawingStyle::Centroid ||
        m_drawingStyle == DrawingStyle::CenterOfMass))
    return;
  m_ellipsoidRenderer->clear();

  if (m_selectionHandler) {
    m_selectionHandler->clear(SelectionType::Aggregate);
  }

  m_aggregateIndices.clear();
  const auto &fragments = m_structure->completedFragments();

  const auto &fragmentMap = m_structure->getFragments();
  int i = 0;
  for (const auto &frag : fragments) {
    auto fragment = fragmentMap.at(frag);
    QColor color = m_structure->getFragmentColor(frag);
    occ::Vec3 p = (m_drawingStyle == DrawingStyle::Centroid)
                      ? fragment.centroid()
                      : fragment.centerOfMass();
    QVector3D pos(p.x(), p.y(), p.z());
    bool selected =
        m_structure->atomsHaveFlags(fragment.atomIndices, AtomFlag::Selected);
    quint32 selectionId{0};
    QVector3D selectionIdColor;
    if (m_selectionHandler) {
      selectionId = m_selectionHandler->add(SelectionType::Aggregate, i);
      selectionIdColor = m_selectionHandler->getColorFromId(selectionId);
    }

    m_aggregateIndices.push_back(AggregateIndex{frag, pos});
    cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, pos, color,
                                               0.4, selectionIdColor, selected);
    i++;
  }
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

  if (atomStyle() == AtomDrawingStyle::None) {
    addAggregateRepresentations();
    m_atomsNeedsUpdate = false;
    return;
  }

  const auto &positions = m_structure->atomicPositions();
  const auto &nums = m_structure->atomicNumbers();

  const auto covRadii = m_structure->covalentRadii();
  const auto vdwRadii = m_structure->vdwRadii();

  auto drawAsEllipsoid = [&](int i) {
    if (atomStyle() != AtomDrawingStyle::Ellipsoid)
      return false;
    if ((nums(i) == 1) && !m_showHydrogenAtomEllipsoids)
      return false;
    return true;
  };

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
    bool selected = m_structure->atomFlagsSet(idx, AtomFlag::Selected);
    if (drawAsEllipsoid(i)) {
      auto adp = m_structure->atomicDisplacementParameters(idx);
      if (!adp.isZero()) {
        QMatrix3x3 scales = adp.thermalEllipsoidMatrixForProbability(
            m_thermalEllipsoidProbability);
        cx::graphics::addEllipsoidToEllipsoidRenderer(
            m_ellipsoidRenderer, position, scales, color, selectionIdColor,
            selected);
        continue;
      }
    }
    cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, position,
                                               color, radius, selectionIdColor,
                                               selected);
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

  m_bondLineRenderer->clear();
  m_cylinderRenderer->clear();

  if (bondStyle() == BondDrawingStyle::None) {
    m_bondsNeedsUpdate = false;
    return;
  }

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
          *m_bondLineRenderer, pointA, 0.5 * pointA + 0.5 * pointB,
          DrawingStyleConstants::bondLineWidth, colorA, id_color, selectedA);
      cx::graphics::addLineToLineRenderer(
          *m_bondLineRenderer, pointB, 0.5 * pointA + 0.5 * pointB,
          DrawingStyleConstants::bondLineWidth, colorB, id_color, selectedB);
    } else {
      cx::graphics::addCylinderToCylinderRenderer(
          m_cylinderRenderer, pointA, pointB, colorA, colorB, radius, id_color,
          selectedA, selectedB);
    }
  }
  m_bondsNeedsUpdate = false;
}

void ChemicalStructureRenderer::beginUpdates() {
  m_bondLineRenderer->beginUpdates();
  m_cylinderRenderer->beginUpdates();
  m_ellipsoidRenderer->beginUpdates();
  m_highlightRenderer->beginUpdates();
  m_cellLinesRenderer->beginUpdates();
}

void ChemicalStructureRenderer::endUpdates() {
  m_bondLineRenderer->endUpdates();
  m_cylinderRenderer->endUpdates();
  m_ellipsoidRenderer->endUpdates();
  m_highlightRenderer->endUpdates();
  m_cellLinesRenderer->endUpdates();
}

bool ChemicalStructureRenderer::needsUpdate() {
  // TODO check for efficiency in non-granular toggle like this
  return m_atomsNeedsUpdate || m_bondsNeedsUpdate || m_meshesNeedsUpdate ||
         m_labelsNeedsUpdate || m_cellsNeedsUpdate;
}

void ChemicalStructureRenderer::draw(bool forPicking) {
  if (needsUpdate()) {
    beginUpdates();
    handleLabelsUpdate();
    handleAtomsUpdate();
    handleBondsUpdate();
    handleMeshesUpdate();
    handleCellsUpdate();
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

  m_bondLineRenderer->bind();
  m_uniforms.apply(m_bondLineRenderer);
  m_bondLineRenderer->draw();
  m_bondLineRenderer->release();

  handleMeshesUpdate();

  for (auto *renderer : m_pointCloudRenderers) {
    renderer->bind();
    m_uniforms.apply(renderer);
    renderer->draw();
    renderer->release();
  }

  // Sort mesh renderers into opaque and transparent groups
  std::vector<MeshInstanceRenderer *> opaqueMeshes;
  std::vector<MeshInstanceRenderer *> transparentMeshes;

  for (auto *renderer : m_meshRenderers) {
    if (renderer->hasTransparentObjects()) {
      transparentMeshes.push_back(renderer);
    } else {
      opaqueMeshes.push_back(renderer);
    }
  }

  // Draw opaque meshes first
  for (auto *meshRenderer : opaqueMeshes) {
    meshRenderer->bind();
    m_uniforms.apply(meshRenderer);
    meshRenderer->draw();
    meshRenderer->release();
  }

  m_frameworkRenderer->draw();

  // Draw transparent meshes last
  for (auto *meshRenderer : transparentMeshes) {
    meshRenderer->bind();
    m_uniforms.apply(meshRenderer);
    meshRenderer->draw();
    meshRenderer->release();
  }

  if (!forPicking) {
    m_labelRenderer->bind();
    m_uniforms.apply(m_labelRenderer);
    m_labelRenderer->draw();
    m_labelRenderer->release();

    m_highlightRenderer->bind();
    m_uniforms.apply(m_highlightRenderer);
    m_highlightRenderer->draw();
    m_highlightRenderer->release();

    m_cellLinesRenderer->bind();
    m_uniforms.apply(m_cellLinesRenderer);
    m_cellLinesRenderer->draw();
    m_cellLinesRenderer->release();
  }

  if (forPicking) {
    m_uniforms.u_renderMode = storedRenderMode;
    m_uniforms.u_selectionMode = false;
  }
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

template <class Renderer>
void addInstanceToInstanceRenderer(MeshInstance *instance,
                                   Renderer *instanceRenderer,
                                   RenderSelection *selectionHandler,
                                   BiMap<MeshInstance *> &meshMap) {

  const auto &availableProperties = instanceRenderer->availableProperties();
  if (!instance || !instance->isVisible())
    return;

  int propertyIndex =
      availableProperties.indexOf(instance->getSelectedProperty());

  float alpha = instance->isTransparent() ? instance->getTransparency() : 1.0;

  QVector3D selectionColor;

  if (selectionHandler) {
    auto index = meshMap.add(instance);
    auto selectionId = selectionHandler->add(SelectionType::Surface, index);
    selectionColor = selectionHandler->getColorFromId(selectionId);
  }

  MeshInstanceVertex v(instance->translationVector(),
                       instance->rotationMatrix(), selectionColor,
                       propertyIndex, alpha);
  instanceRenderer->addInstance(v);
}

void ChemicalStructureRenderer::addFaceHighlightsForMeshInstance(
    Mesh *mesh, MeshInstance *meshInstance) {
  // face highlights
  QColor color = Qt::red;
  for (const int v : mesh->vertexHighlights()) {
    const auto vertex = meshInstance->vertexVector3D(v);
    const auto normal = meshInstance->vertexNormalVector3D(v);
    cx::graphics::addLineToLineRenderer(*m_highlightRenderer, vertex,
                                        vertex + normal, 1.0, color);
  }
}

void ChemicalStructureRenderer::handleMeshesUpdate() {
  if (!m_meshesNeedsUpdate)
    return;

  // TODO re-use mesh renderers
  m_meshRenderers.clear();
  m_pointCloudRenderers.clear();

  m_meshMap.clear();
  m_highlightRenderer->clear();

  if (m_selectionHandler) {
    m_selectionHandler->clear(SelectionType::Surface);
  }
  for (auto *child : m_structure->children()) {
    auto *mesh = qobject_cast<Mesh *>(child);
    if (!mesh)
      continue;

    if (mesh->numberOfFaces() == 0) {
      PointCloudInstanceRenderer *instanceRenderer =
          new PointCloudInstanceRenderer(mesh);
      instanceRenderer->beginUpdates();
      for (auto *meshChild : child->children()) {
        auto *meshInstance = qobject_cast<MeshInstance *>(meshChild);
        addInstanceToInstanceRenderer<PointCloudInstanceRenderer>(
            meshInstance, instanceRenderer, m_selectionHandler, m_meshMap);
        addFaceHighlightsForMeshInstance(mesh, meshInstance);
        mesh->setRendererIndex(m_pointCloudRenderers.size());
      }
      instanceRenderer->endUpdates();
      m_pointCloudRenderers.push_back(instanceRenderer);

    } else {
      MeshInstanceRenderer *instanceRenderer = new MeshInstanceRenderer(mesh);
      instanceRenderer->beginUpdates();
      for (auto *meshChild : child->children()) {
        auto *meshInstance = qobject_cast<MeshInstance *>(meshChild);
        addInstanceToInstanceRenderer<MeshInstanceRenderer>(
            meshInstance, instanceRenderer, m_selectionHandler, m_meshMap);

        addFaceHighlightsForMeshInstance(mesh, meshInstance);
        mesh->setRendererIndex(m_meshRenderers.size());
      }
      instanceRenderer->endUpdates();
      m_meshRenderers.push_back(instanceRenderer);
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

void ChemicalStructureRenderer::updateThermalEllipsoidProbability(double p) {
  if (m_thermalEllipsoidProbability == p)
    return;
  m_thermalEllipsoidProbability = p;
  m_atomsNeedsUpdate = true;
}

void ChemicalStructureRenderer::setFrameworkOptions(
    const FrameworkOptions &options) {
  m_frameworkOptions = options;
  m_frameworkRenderer->setOptions(m_frameworkOptions);
}

MeshInstance *ChemicalStructureRenderer::getMeshInstance(size_t index) const {
  return m_meshMap.get(index);
}

int ChemicalStructureRenderer::getMeshInstanceIndex(
    MeshInstance *meshInstance) const {
  auto result = m_meshMap.getIndex(meshInstance);
  if (!result)
    return -1;
  return static_cast<int>(*result);
}

} // namespace cx::graphics
