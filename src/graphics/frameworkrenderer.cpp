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
  m_labelRenderer = new BillboardRenderer();

  m_interactionComponentColors = {
      {"coulomb", QColor("#a40000")},    {"polarization", QColor("#b86092")},
      {"exchange", QColor("#721b3e")},   {"repulsion", QColor("#ffcd12")},
      {"dispersion", QColor("#007e2f")}, {"total", QColor("#16317d")}};
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
void FrameworkRenderer::forceUpdates() { m_needsUpdate = true; }

void FrameworkRenderer::beginUpdates() {
  m_lineRenderer->beginUpdates();
  m_ellipsoidRenderer->beginUpdates();
  m_cylinderRenderer->beginUpdates();
  m_labelRenderer->beginUpdates();
}

void FrameworkRenderer::endUpdates() {
  m_lineRenderer->endUpdates();
  m_ellipsoidRenderer->endUpdates();
  m_cylinderRenderer->endUpdates();
  m_labelRenderer->endUpdates();
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
  m_labelRenderer->clear();

  if (m_options.display == FrameworkOptions::Display::None)
    return;

  FragmentPairSettings pairSettings;
  pairSettings.allowInversion =
      m_options.allowInversion &
      m_interactions->hasInversionSymmetry(m_options.model);
  auto fragmentPairs = m_structure->findFragmentPairs(pairSettings);
  qDebug() << "Number of unique pairs:" << fragmentPairs.uniquePairs.size();
  auto interactionMap = m_interactions->getInteractionsMatchingFragments(
      fragmentPairs.uniquePairs);

  auto uniqueInteractions = interactionMap.value(m_options.model, {});
  if (uniqueInteractions.size() < fragmentPairs.uniquePairs.size())
    return;

  QColor color = m_options.customColor;
  if (m_options.coloring == FrameworkOptions::Coloring::Component) {
    color = m_interactionComponentColors.value(
        m_options.component, m_defaultInteractionComponentColor);
  }

  std::vector<std::pair<QColor, double>> energies;
  energies.reserve(uniqueInteractions.size());

  for (const auto &pair : fragmentPairs.uniquePairs) {
    qDebug() << pair.index;
  }

  double emin = std::numeric_limits<double>::max();
  double emax = std::numeric_limits<double>::min();

  for (const auto *interaction : uniqueInteractions) {
    QColor c = color;
    double energy = 0.0;
    if (interaction) {
      energy = interaction->getComponent(m_options.component);
      if (m_options.coloring == FrameworkOptions::Coloring::Interaction) {
        c = interaction->color();
      }
    }
    emin = qMin(energy, emin);
    emax = qMax(energy, emax);
    energies.push_back({c, energy});
  }

  if (m_options.coloring == FrameworkOptions::Coloring::Value) {
    ColorMapFunc cmap(ColorMapName::Turbo, emin, emax);
    for (auto &[color, energy] : energies) {
      color = cmap(energy);
    }
  }

  const bool inv = pairSettings.allowInversion;

  for (const auto &[fragIndex, molPairs] : fragmentPairs.pairs) {
    for (const auto &[pair, uniqueIndex] : molPairs) {
      auto [color, energy] = energies[uniqueIndex];
      if (std::abs(energy) <= m_options.cutoff)
        continue;
      double scale = std::abs(energy * thickness());
      if (scale < 1e-4)
        continue;

      auto ca = pair.a.centroid();
      QVector3D va(ca.x(), ca.y(), ca.z());
      auto cb = pair.b.centroid();
      QVector3D vb(cb.x(), cb.y(), cb.z());
      QVector3D m = va + (vb - va) * 0.5;

      if (inv) {
        if (m_options.display == FrameworkOptions::Display::Tubes) {
          cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, va,
                                                     color, scale);
          cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, vb,
                                                     color, scale);
          cx::graphics::addCylinderToCylinderRenderer(m_cylinderRenderer, va,
                                                      vb, color, color, scale);
        } else if (m_options.display == FrameworkOptions::Display::Lines) {
          cx::graphics::addLineToLineRenderer(*m_lineRenderer, va, vb, scale,
                                              color);
          cx::graphics::addTextToBillboardRenderer(*m_labelRenderer, m,
                                                   QString::number(energy, 'd', 1));
        }
      } else {
        if (m_options.display == FrameworkOptions::Display::Tubes) {
          cx::graphics::addSphereToEllipsoidRenderer(m_ellipsoidRenderer, va,
                                                     color, scale);
          cx::graphics::addCylinderToCylinderRenderer(m_cylinderRenderer, va, m,
                                                      color, color, scale);
        } else if (m_options.display == FrameworkOptions::Display::Lines) {
          cx::graphics::addLineToLineRenderer(*m_lineRenderer, va, m, scale,
                                              color);
        }
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

  m_labelRenderer->bind();
  m_uniforms.apply(m_labelRenderer);
  m_labelRenderer->draw();
  m_labelRenderer->release();
}

} // namespace cx::graphics
