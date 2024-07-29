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

  m_interactionComponentColors = {
    {"coulomb", QColor("#a40000")},
    {"polarization", QColor("#b86092")},
    {"exchange", QColor("#721b3e")},
    {"repulsion", QColor("#ffcd12")},
    {"dispersion", QColor("#007e2f")},
    {"total", QColor("#16317d")}
  };
  m_defaultInteractionComponentColor = QColor("#00b7a7");

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

  auto fragmentPairs = m_structure->findFragmentPairs();
  auto interactionMap = m_interactions->getInteractionsMatchingFragments(
      fragmentPairs.uniquePairs);


  auto uniqueInteractions = interactionMap.value(m_options.model, {});
  if (uniqueInteractions.size() < fragmentPairs.uniquePairs.size())
    return;

  auto color = m_interactionComponentColors.value(m_options.component, m_defaultInteractionComponentColor);
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

      if (m_options.display == FrameworkOptions::Display::Tubes) {
        cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, va,
                                                   color, scale);

        cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, vb,
                                                   color, scale);

        cx::graphics::addCylinderToCylinderRenderer(m_cylinderRenderer, va, vb,
                                                    color, color, scale);
      } else if (m_options.display == FrameworkOptions::Display::Lines) {
        cx::graphics::addLineToLineRenderer(*m_lineRenderer, va, vb, scale,
                                            color);
      }
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
