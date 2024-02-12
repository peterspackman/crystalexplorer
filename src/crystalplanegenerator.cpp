#include "crystalplanegenerator.h"
#include <numeric>

template <typename T>
T convertToCartesian(const UnitCell &uc, const T &vector) {
  Vector3q tmp = uc.aAxis() * vector.x() + uc.bAxis() * vector.y() +
                 uc.cAxis() * vector.z();
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
  return (m_uc.reciprocalMatrix() * Vector3q(m_hkl.h, m_hkl.k, m_hkl.l)).norm();
}

CrystalPlaneGenerator::CrystalPlaneGenerator(const UnitCell &uc,
                                             const MillerIndex &hkl)
    : m_uc(uc), m_hkl(hkl) {
  calculateVectors();
}

void CrystalPlaneGenerator::calculateVectors() {
  const double minimumLength = 1e-3;
  int commonDenominator =
      std::gcd(std::gcd(m_hkl.h, m_hkl.k), std::gcd(m_hkl.k, m_hkl.l));
  double depthMagnitude = commonDenominator / interplanarSpacing();
  Vector3q unitNormal = normalVector();
  std::vector<Vector3q> vectorCandidates;

  // H, K
  {
    int c = std::gcd(m_hkl.h, m_hkl.k);
    double cd = (c != 0) ? c : 1.0;
    Vector3q v = m_hkl.k / cd * m_uc.aAxis() - m_hkl.h / cd * m_uc.bAxis();
    if (v.squaredNorm() > minimumLength)
      vectorCandidates.push_back(v);
  }
  // H, L
  {
    int c = std::gcd(m_hkl.h, m_hkl.l);
    double cd = (c != 0) ? c : 1.0;
    Vector3q v = m_hkl.l / cd * m_uc.aAxis() - m_hkl.h / cd * m_uc.cAxis();
    if (v.squaredNorm() > minimumLength)
      vectorCandidates.push_back(v);
  }
  // K, L
  {
    int c = std::gcd(m_hkl.k, m_hkl.l);
    double cd = (c != 0) ? c : 1.0;
    Vector3q v = m_hkl.l / cd * m_uc.bAxis() - m_hkl.k / cd * m_uc.cAxis();
    if (v.squaredNorm() > minimumLength)
      vectorCandidates.push_back(v);
  }
  std::vector<Vector3q> temp;
  for (int i = 0; i < vectorCandidates.size(); i++) {
    const auto &vi = vectorCandidates[i];
    for (int j = i + 1; j < vectorCandidates.size(); j++) {
      const auto &vj = vectorCandidates[j];
      Vector3q v = vi + vj;
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
            [](const Vector3q &a, const Vector3q &b) {
              return a.squaredNorm() < b.squaredNorm();
            });

  bool found = false;
  m_aVector = vectorCandidates[0];
  for (int i = 1; i < vectorCandidates.size(); i++) {
    Vector3q tmp = m_aVector.cross(vectorCandidates[i]);
    if (tmp.squaredNorm() > minimumLength) {
      found = true;
      m_bVector = vectorCandidates[i];
      break;
    }
  }
  m_angle = std::acos(m_aVector.normalized().dot(m_bVector.normalized()));
  m_depthVector = depthMagnitude * unitNormal;
}

Vector3q CrystalPlaneGenerator::normalVector() const {
  std::vector<Vector3q> vecs;
  if (m_hkl.h == 0)
    vecs.push_back(m_uc.aAxis());
  if (m_hkl.k == 0)
    vecs.push_back(m_uc.bAxis());
  if (m_hkl.l == 0)
    vecs.push_back(m_uc.cAxis());

  if (vecs.size() < 2) {
    std::vector<Vector3q> points;
    if (m_hkl.h != 0)
      points.push_back(convertToCartesian(m_uc, Vector3q(1.0 / m_hkl.h, 0, 0)));
    if (m_hkl.k != 0)
      points.push_back(convertToCartesian(m_uc, Vector3q(0, 1.0 / m_hkl.k, 0)));
    if (m_hkl.l != 0)
      points.push_back(convertToCartesian(m_uc, Vector3q(0, 0, 1.0 / m_hkl.l)));
    if (points.size() == 2) {
      vecs.push_back(points[1] - points[0]);
    } else {
      vecs.push_back(points[1] - points[0]);
      vecs.push_back(points[2] - points[0]);
    }
  }
  Vector3q v = vecs[0].cross(vecs[1]);
  v.normalize();
  qDebug() << "Normal vector" << QVector3D(v.x(), v.y(), v.z());
  return v;
}

Vector3q CrystalPlaneGenerator::origin(double offset) const {
  return offset * normalVector();
}
