#include "crystalplanegenerator.h"
#include <numeric>

template <typename T>
T convertToCartesian(const occ::crystal::UnitCell &uc, const T &vector) {
  occ::Vec3 tmp = uc.direct() * occ::Vec3(vector.x(), vector.y(), vector.z());
  return T(tmp.x(), tmp.y(), tmp.z());
}

template <typename T, int Rows = -1>
std::vector<size_t> argsort(const Eigen::Matrix<T, Rows, 1> &vec) {
  std::vector<size_t> idx(vec.size());
  std::iota(idx.begin(), idx.end(), 0);

  std::stable_sort(idx.begin(), idx.end(), [&vec](size_t i1, size_t i2) {
    return std::abs(vec(i1)) < std::abs(vec(i2));
  });

  return idx;
}

double CrystalPlaneGenerator::interplanarSpacing() const {
  return 1.0 / (m_uc.reciprocal() * occ::Vec3(m_hkl.h, m_hkl.k, m_hkl.l)).norm();
}

CrystalPlaneGenerator::CrystalPlaneGenerator(ChemicalStructure * structure,
                                             const MillerIndex &hkl)
    : m_hkl(hkl) {
  m_uc = occ::crystal::UnitCell(structure->cellVectors());
  qDebug() << hkl.h << hkl.k << hkl.l;
  calculateVectors();
}

void CrystalPlaneGenerator::calculateVectors() {
  const double minimumLength = 1e-3;
  int commonDenominator =
      std::gcd(std::gcd(m_hkl.h, m_hkl.k), std::gcd(m_hkl.k, m_hkl.l));
  double depthMagnitude = commonDenominator / interplanarSpacing();
  occ::Vec3 unitNormal = normalVector();
  std::vector<occ::Vec3> vectorCandidates;

  // H, K
  {
    int c = std::gcd(m_hkl.h, m_hkl.k);
    double cd = (c != 0) ? c : 1.0;
    occ::Vec3 v = m_hkl.k / cd * m_uc.a_vector() - m_hkl.h / cd * m_uc.b_vector();
    if (v.squaredNorm() > minimumLength)
      vectorCandidates.push_back(v);
  }
  // H, L
  {
    int c = std::gcd(m_hkl.h, m_hkl.l);
    double cd = (c != 0) ? c : 1.0;
    occ::Vec3 v = m_hkl.l / cd * m_uc.a_vector() - m_hkl.h / cd * m_uc.c_vector();
    if (v.squaredNorm() > minimumLength)
      vectorCandidates.push_back(v);
  }
  // K, L
  {
    int c = std::gcd(m_hkl.k, m_hkl.l);
    double cd = (c != 0) ? c : 1.0;
    occ::Vec3 v = m_hkl.l / cd * m_uc.b_vector() - m_hkl.k / cd * m_uc.c_vector();
    if (v.squaredNorm() > minimumLength)
      vectorCandidates.push_back(v);
  }
  std::vector<occ::Vec3> temp;
  for (int i = 0; i < vectorCandidates.size(); i++) {
    const auto &vi = vectorCandidates[i];
    for (int j = i + 1; j < vectorCandidates.size(); j++) {
      const auto &vj = vectorCandidates[j];
      occ::Vec3 v = vi + vj;
      if (v.squaredNorm() > minimumLength) {
        temp.push_back(v);
      }
      v = vi - vj;
      if (v.squaredNorm() > minimumLength) {
        temp.push_back(v);
      }
    }
  }
  vectorCandidates.insert(vectorCandidates.end(), temp.begin(), temp.end());

  std::sort(vectorCandidates.begin(), vectorCandidates.end(),
            [](const occ::Vec3 &a, const occ::Vec3 &b) {
              return a.squaredNorm() < b.squaredNorm();
            });

  bool found = false;
  m_aVector = vectorCandidates[0];
  for (int i = 1; i < vectorCandidates.size(); i++) {
    occ::Vec3 tmp = m_aVector.cross(vectorCandidates[i]);
    if (tmp.squaredNorm() > minimumLength) {
      found = true;
      m_bVector = vectorCandidates[i];
      break;
    }
  }
  if(!found) qDebug() << "Candidate vector not found!";
  m_angle = std::acos(m_aVector.normalized().dot(m_bVector.normalized()));
  m_depthVector = depthMagnitude * unitNormal;
}

occ::Vec3 CrystalPlaneGenerator::normalVector() const {
  std::vector<occ::Vec3> vecs;
  if (m_hkl.h == 0)
    vecs.push_back(m_uc.a_vector());
  if (m_hkl.k == 0)
    vecs.push_back(m_uc.b_vector());
  if (m_hkl.l == 0)
    vecs.push_back(m_uc.c_vector());

  if (vecs.size() < 2) {
    std::vector<occ::Vec3> points;
    if (m_hkl.h != 0)
      points.push_back(convertToCartesian(m_uc, occ::Vec3(1.0 / m_hkl.h, 0, 0)));
    if (m_hkl.k != 0)
      points.push_back(convertToCartesian(m_uc, occ::Vec3(0, 1.0 / m_hkl.k, 0)));
    if (m_hkl.l != 0)
      points.push_back(convertToCartesian(m_uc, occ::Vec3(0, 0, 1.0 / m_hkl.l)));
    if (points.size() == 2) {
      vecs.push_back(points[1] - points[0]);
    } else {
      vecs.push_back(points[1] - points[0]);
      vecs.push_back(points[2] - points[0]);
    }
  }
  occ::Vec3 v = vecs[0].cross(vecs[1]);
  v.normalize();
  qDebug() << "Normal vector" << QVector3D(v.x(), v.y(), v.z());
  return v;
}

occ::Vec3 CrystalPlaneGenerator::origin(double offset) const {
  return offset * normalVector();
}
