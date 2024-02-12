#include "orbitcamera.h"
#include <cmath>

OrbitCamera::OrbitCamera(const QVector3D &location, const QVector3D &up,
                         const QVector3D &origin, const CameraOptions &options)
    : m_zoom(1.0), m_options(options), m_location(location), m_upVector(up),
      m_origin(origin) {
  m_projection.setToIdentity();
  updateProjection();
  updateView();
  m_model.setToIdentity();
}

void OrbitCamera::updateView() {
  m_view.setToIdentity();
  m_view.lookAt(m_location, m_origin, m_upVector);
}

void OrbitCamera::setProjectionType(CameraProjection type) {
  if (type != m_options.projection) {
    m_options.projection = type;
    updateProjection();
  }
}

void OrbitCamera::setTheta(float theta) {
  m_options.theta = theta;
  updateView();
}

void OrbitCamera::lookAt(const QVector3D &origin) {
  m_origin = origin;
  updateView();
}

void OrbitCamera::setPhi(float phi) {
  m_options.phi = phi;
  updateView();
}

void OrbitCamera::updateProjection() {
  m_projection.setToIdentity();
  if (m_options.isPerspective()) {
    m_projection.perspective(m_options.fov, m_window_aspect * m_options.aspect,
                             m_options.znear, m_options.zfar);
    qDebug() << "Camera type: perspective";
  } else {
    float right = 10.0f;
    float left = -10.0f;
    float top = 10.0f;
    float bottom = -10.0f;

    m_projection.ortho(left, right, top, bottom, -10.0f, 100.0f);
    qDebug() << "Camera type: orthographic";
  }
}

// Set zoom level (aperture angle in degrees)
void OrbitCamera::setFov(float zoom) {
  m_options.fov = qMin(qMax(zoom, 1.0f), 179.0f);
  updateProjection();
}

// Set proj aspect
void OrbitCamera::setAspect(float aspect) {
  m_options.aspect = aspect;
  updateProjection();
}

void OrbitCamera::onResize(float width, float height) {
  m_width = width;
  m_height = height;
  m_window_aspect = m_width / m_height;
  updateProjection();
}

void OrbitCamera::onMouseDrag(QPointF delta) {
  double x_rot = fmod(360.0 + delta.y(), 360.0);
  double y_rot = fmod(360.0 + delta.x(), 360.0);
  QVector3D displacementFromOrigin = (m_location - m_origin);
  QVector3D rightVector =
      QVector3D::crossProduct(m_upVector, displacementFromOrigin);
  QQuaternion x_rotation =
      QQuaternion::fromAxisAndAngle(rightVector, static_cast<float>(x_rot));
  QQuaternion y_rotation =
      QQuaternion::fromAxisAndAngle(m_upVector, static_cast<float>(y_rot));
  displacementFromOrigin =
      (x_rotation * y_rotation).rotatedVector(displacementFromOrigin);
  m_location = m_origin + displacementFromOrigin;
  m_upVector = x_rotation.rotatedVector(m_upVector);
  updateView();
}

void OrbitCamera::onMouseScroll(float delta) {
  if (m_options.isPerspective()) {
    QVector3D displacementFromOrigin = m_location - m_origin;
    displacementFromOrigin *= (1.0f - delta / 500.0f);
    m_location = m_origin + displacementFromOrigin;
    updateView();
  } else {
    m_zoom = qMax(0.001f, qMin(1000.0f, m_zoom * (1.0f + delta / 500.0f)));
    updateProjection();
  }
}
