#pragma once

enum class CameraProjection { Perspective, Orthographic };

struct CameraOptions {

  static constexpr float DefaultFar = 1000.0f;
  static constexpr float DefaultNear = 0.1f;
  static constexpr float DefaultPhi = 0.0f;
  static constexpr float DefaultTheta = 0.0f;
  static constexpr float DefaultFov = 35.0f;
  static constexpr float DefaultAspectRatio = 1.0f;
  static constexpr CameraProjection DefaultProjection =
      CameraProjection::Perspective;
  float zfar = DefaultFar;
  float znear = DefaultNear;
  float phi = DefaultPhi;
  float theta = DefaultTheta;
  float fov = DefaultFov;
  float aspect = DefaultAspectRatio;
  CameraProjection projection = DefaultProjection;
  inline bool isPerspective() {
    return projection == CameraProjection::Perspective;
  }
  inline bool isOrthographic() {
    return projection == CameraProjection::Orthographic;
  }
};
