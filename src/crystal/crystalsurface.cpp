#include "crystalsurface.h"

using occ::Vec3;

double CrystalSurface::interplanarSpacing() const {
  return m_settings.hkl.d(m_crystal.unit_cell().reciprocal());
}

Vec3 CrystalSurface::normalVector() const {
  std::vector<Vec3> vecs;
  const auto &hkl = m_settings.hkl;
  const auto &uc = m_crystal.unit_cell();
  if (hkl.h == 0)
    vecs.push_back(uc.a_vector());
  if (hkl.k == 0)
    vecs.push_back(uc.b_vector());
  if (hkl.l == 0)
    vecs.push_back(uc.c_vector());

  if (vecs.size() < 2) {
    std::vector<Vec3> points;
    if (hkl.h != 0)
      points.push_back(m_crystal.to_cartesian(Vec3(1.0 / hkl.h, 0, 0)));
    if (hkl.k != 0)
      points.push_back(m_crystal.to_cartesian(Vec3(0, 1.0 / hkl.k, 0)));
    if (hkl.l != 0)
      points.push_back(m_crystal.to_cartesian(Vec3(0, 0, 1.0 / hkl.l)));
    if (points.size() == 2) {
      vecs.push_back(points[1] - points[0]);
    } else {
      vecs.push_back(points[1] - points[0]);
      vecs.push_back(points[2] - points[0]);
    }
  }
  Vec3 v = vecs[0].cross(vecs[1]);
  v.normalize();
  return v;
}

void CrystalSurface::determineVectors() {
  const auto &hkl = m_settings.hkl;
  const auto &uc = m_crystal.unit_cell();

  int cd = std::gcd(std::gcd(hkl.h, hkl.k), std::gcd(hkl.k, hkl.l));
  m_depth = cd / interplanarSpacing();
  Vec3 unitNormal = normalVector();

  std::vector<Vec3> vector_candidates;
  {
    int c = std::gcd(hkl.h, hkl.k);
    double cd = (c != 0) ? c : 1.0;
    Vec3 a = hkl.k / cd * uc.a_vector() - hkl.h / cd * uc.b_vector();
    if (a.squaredNorm() > 1e-3)
      vector_candidates.push_back(a);
  }
  {
    int c = std::gcd(hkl.h, hkl.l);
    double cd = (c != 0) ? c : 1.0;
    Vec3 a = hkl.l / cd * uc.a_vector() - hkl.h / cd * uc.c_vector();
    if (a.squaredNorm() > 1e-3)
      vector_candidates.push_back(a);
  }
  {
    int c = std::gcd(hkl.k, hkl.l);
    double cd = (c != 0) ? c : 1.0;
    Vec3 a = hkl.l / cd * uc.b_vector() - hkl.k / cd * uc.c_vector();
    if (a.squaredNorm() > 1e-3)
      vector_candidates.push_back(a);
  }
  std::vector<Vec3> temp_vectors;
  // add linear combinations
  for (int i = 0; i < vector_candidates.size() - 1; i++) {
    const auto &v_a = vector_candidates[i];
    for (int j = i + 1; j < vector_candidates.size(); j++) {
      const auto &v_b = vector_candidates[j];
      Vec3 a = v_a + v_b;
      if (a.squaredNorm() > 1e-3) {
        temp_vectors.push_back(a);
      }
      a = v_a - v_b;
      if (a.squaredNorm() > 1e-3) {
        temp_vectors.push_back(a);
      }
    }
  }
  vector_candidates.insert(vector_candidates.end(), temp_vectors.begin(),
                           temp_vectors.end());

  qDebug() << "Candidate surface vectors:";
  for (const auto &x : vector_candidates) {
    qDebug() << x(0) << x(1) << x(2);
  }

  std::sort(vector_candidates.begin(), vector_candidates.end(),
            [](const Vec3 &a, const Vec3 &b) {
              return a.squaredNorm() < b.squaredNorm();
            });
  m_aVector = vector_candidates[0];
  bool found = false;
  for (int i = 1; i < vector_candidates.size(); i++) {
    Vec3 a = m_aVector.cross(vector_candidates[i]);
    if (a.squaredNorm() > 1e-3) {
      found = true;
      m_bVector = vector_candidates[i];
      break;
    }
  }
  if (!found)
    qWarning() << "No valid second vector for surface was found!";

  qDebug() << "Found vectors:";
  qDebug() << "A = " << m_aVector(0) << m_aVector(1) << m_aVector(2);
  qDebug() << "B = " << m_bVector(0) << m_bVector(1) << m_bVector(2);

  m_angle = std::acos(m_aVector.normalized().dot(m_bVector.normalized()));
  m_depthVector = m_depth * unitNormal;
}

void CrystalSurface::setOccCrystalCut(const OccCrystal &crystal,
                                      const CrystalSurfaceSettings &settings) {
  m_settings = settings;
  m_crystal = crystal;
  determineVectors();
}
