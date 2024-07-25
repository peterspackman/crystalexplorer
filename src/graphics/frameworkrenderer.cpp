#include "frameworkrenderer.h"
#include "graphics.h"
#include <QElapsedTimer>

namespace cx::graphics {

FrameworkRenderer::FrameworkRenderer(ChemicalStructure *structure,
                                     QObject *parent)
    : QObject(parent) {
  m_ellipsoidRenderer = new EllipsoidRenderer();
  m_cylinderRenderer = new CylinderRenderer();
  m_lineRenderer = new LineRenderer();

  update(structure);
}

void FrameworkRenderer::update(ChemicalStructure *structure) {
  m_structure = structure;
  if (m_structure)
    m_interactions = structure->pairInteractions();
  m_needsUpdate = true;
}

float FrameworkRenderer::thickness() const { return m_options.scale; }
void FrameworkRenderer::setThickness(float t) { m_options.scale = t; }

void FrameworkRenderer::setOptions(const FrameworkOptions &o) {
  m_needsUpdate = true;
  m_options = o;
}

void FrameworkRenderer::updateRendererUniforms(
    const RendererUniforms &uniforms) {
  m_uniforms = uniforms;
}

void FrameworkRenderer::updateInteractions() { m_needsUpdate = true; }

void FrameworkRenderer::beginUpdates() {
  m_lineRenderer->beginUpdates();
  m_ellipsoidRenderer->beginUpdates();
  m_cylinderRenderer->beginUpdates();
}

void FrameworkRenderer::endUpdates() {
  m_lineRenderer->endUpdates();
  m_ellipsoidRenderer->endUpdates();
  m_cylinderRenderer->endUpdates();
}

void FrameworkRenderer::setModel(const QString &model) {
  m_options.model = model;
}

void FrameworkRenderer::setComponent(const QString &comp) {
  m_options.component = comp;
}

void FrameworkRenderer::handleInteractionsUpdate() {
  if (!m_needsUpdate)
    return;
  if (!m_structure || !m_interactions)
    return;

  beginUpdates();
  m_ellipsoidRenderer->clear();
  m_lineRenderer->clear();
  m_cylinderRenderer->clear();

  if (m_options.display == FrameworkOptions::Display::None)
    return;

  QElapsedTimer timer;

  // Timing fragment pairs
  timer.start();
  qDebug() << "Fragment pairs";
  auto fragmentPairs = m_structure->findFragmentPairs();
  qint64 fragmentPairsTime = timer.elapsed();
  qDebug() << "Time taken for findFragmentPairs:" << fragmentPairsTime << "ms";

  // Timing fragment matching fragments
  timer.restart();
  qDebug() << "Fragment matching fragments";
  auto interactionMap = m_interactions->getInteractionsMatchingFragments(
      fragmentPairs.uniquePairs);
  qint64 interactionMapTime = timer.elapsed();
  qDebug() << "Time taken for getInteractionsMatchingFragments:"
           << interactionMapTime << "ms";

  // Compare times
  qDebug() << "Ratio (interactionMap / fragmentPairs):"
           << static_cast<double>(interactionMapTime) / fragmentPairsTime;

  auto uniqueInteractions = interactionMap.value(m_options.model, {});
  if (uniqueInteractions.size() < fragmentPairs.uniquePairs.size())
    return;

  for (const auto &molPairs : fragmentPairs.pairs) {
    for (const auto &[pair, uniqueIndex] : molPairs) {
      auto *interaction = uniqueInteractions[uniqueIndex];
      if (!interaction)
        continue;
      auto ca = pair.a.centroid();
      QVector3D va(ca.x(), ca.y(), ca.z());
      auto cb = pair.b.centroid();
      QVector3D vb(cb.x(), cb.y(), cb.z());
      double energy = interaction->getComponent(m_options.component);
      if (std::abs(energy) < m_options.cutoff)
        continue;
      double scale = std::abs(energy * thickness());
      if (scale < 1e-4)
        continue;

      QColor color(Qt::red);

      qDebug() << "Centroids: " << va << vb;
      cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, va, color,
                                                 scale);

      cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, vb, color,
                                                 scale);

      cx::graphics::addCylinderToCylinderRenderer(m_cylinderRenderer, va, vb,
                                                  color, color, scale);
    }
  }
  endUpdates();
  m_needsUpdate = false;
}

void FrameworkRenderer::draw(bool forPicking) {
  if (m_needsUpdate) {
    handleInteractionsUpdate();
  }

  const auto storedRenderMode = m_uniforms.u_renderMode;

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
}

} // namespace cx::graphics
