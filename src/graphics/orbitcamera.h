#pragma once
#include "cameraoptions.h"
#include <QMatrix4x4>
#include <QPoint>
#include <QQuaternion>
#include <QVector3D>

class OrbitCamera {

public:
  OrbitCamera(const QVector3D &location = {0, 0, 1},
              const QVector3D &up = {0, 1, 0},
              const QVector3D &origin = {0, 0, 0},
              const CameraOptions &options = {});
  void setTheta(float theta);
  void setPhi(float phi);
  void setFov(float fov);
  void setAspect(float aspect);
  void onResize(float width, float height);
  void setProjectionType(CameraProjection type);
  void onMouseDrag(QPointF delta);
  void onMouseScroll(float delta);
  void lookAt(const QVector3D &origin);
  inline float theta() { return m_options.theta; }
  inline float distance() { return (m_location - m_origin).length(); }
  inline float phi() { return m_options.phi; }
  inline float zoom() { return m_options.fov; }
  inline float aspect() { return m_options.aspect; }
  inline const QMatrix4x4 &model() { return m_model; }
  inline const QMatrix4x4 &view() { return m_view; }
  inline const QMatrix4x4 &projection() { return m_projection; }
  inline const QVector3D &location() { return m_location; }
  inline const QVector3D &origin() { return m_origin; }
  inline const QMatrix4x4 modelView() const { return m_view * m_model; }
  inline const QMatrix4x4 modelViewInverse() const {
    return (m_view * m_model).inverted();
  }
  inline const QMatrix4x4 viewInverse() const { return m_view.inverted(); }
  inline const QMatrix4x4 modelViewProjection() const {
    return m_projection * m_view * m_model;
  }
  inline const QMatrix3x3 normal() const { return modelView().normalMatrix(); }
  inline CameraProjection projectionType() const {
    return m_options.projection;
  }

  inline const QVector3D up() const {
    return m_view.row(1).toVector3D().normalized();
  }
  inline const QVector3D right() const {
    return m_view.row(0).toVector3D().normalized();
  }
  inline const QVector3D forward() const {
    return (m_origin - m_location).normalized();
  }

  inline void setModel(const QMatrix4x4 &m) { m_model = m; }
  inline void setView(const QMatrix4x4 &m) {
    m_view = m;
    m_location = m_view.inverted().column(3).toVector3D();
  }
  inline void setProjection(const QMatrix4x4 &m) { m_projection = m; }

private:
  void updateProjection();
  void updateView();
  float m_width = 0.0;
  float m_height = 0.0;
  float m_zoom = 1.0;
  float m_window_aspect = 1.0;
  CameraOptions m_options = {};
  QMatrix4x4 m_projection;
  QMatrix4x4 m_view;
  QMatrix4x4 m_model;
  QVector3D m_location;
  QVector3D m_upVector;
  QVector3D m_origin;
};
