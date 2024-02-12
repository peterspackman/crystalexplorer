#pragma once
#include <QKeySequence>

enum class DrawingStyle { Tube, BallAndStick, SpaceFill, WireFrame, Ortep };

enum class AtomDrawingStyle {
  None,
  CovalentRadiusSphere,
  VanDerWaalsSphere,
  RoundCapped,
  Ellipsoid
};

enum class BondDrawingStyle { None, Line, Stick };

inline constexpr AtomDrawingStyle
atomStyleForDrawingStyle(const DrawingStyle &drawingStyle) {
  switch (drawingStyle) {
  case DrawingStyle::Tube:
    return AtomDrawingStyle::RoundCapped;
  case DrawingStyle::SpaceFill:
    return AtomDrawingStyle::VanDerWaalsSphere;
  case DrawingStyle::WireFrame:
    return AtomDrawingStyle::None;
  case DrawingStyle::Ortep:
    return AtomDrawingStyle::Ellipsoid;
  default:
    return AtomDrawingStyle::CovalentRadiusSphere; // Ball and Stick
  }
}

inline constexpr BondDrawingStyle
bondStyleForDrawingStyle(const DrawingStyle &drawingStyle) {
  switch (drawingStyle) {
  case DrawingStyle::SpaceFill:
    return BondDrawingStyle::None;
  case DrawingStyle::WireFrame:
    return BondDrawingStyle::Line;
  default:
    return BondDrawingStyle::Stick;
  }
}

inline constexpr const char *
drawingStyleLabel(const DrawingStyle &drawingStyle) {
  switch (drawingStyle) {
  case DrawingStyle::Tube:
    return "Tube";
  case DrawingStyle::SpaceFill:
    return "Space Filling";
  case DrawingStyle::WireFrame:
    return "Wireframe";
  case DrawingStyle::Ortep:
    return "Thermal Ellipsoids";
  default:
    return "Ball and Stick"; // Ball and Stick
  }
}

inline const QKeySequence
drawingStyleKeySequence(const DrawingStyle &drawingStyle) {
  switch (drawingStyle) {
  case DrawingStyle::Tube:
    return QKeySequence(Qt::SHIFT | Qt::Key_1);
  case DrawingStyle::SpaceFill:
    return QKeySequence(Qt::SHIFT | Qt::Key_3);
  case DrawingStyle::WireFrame:
    return QKeySequence(Qt::SHIFT | Qt::Key_4);
  case DrawingStyle::Ortep:
    return QKeySequence(Qt::SHIFT | Qt::Key_5);
  default:
    return QKeySequence(Qt::SHIFT | Qt::Key_2); // Ball and Stick
  }
}

inline const DrawingStyle GLOBAL_DRAWING_STYLE = DrawingStyle::BallAndStick;

namespace DrawingStyleConstants {

inline constexpr float bondLineWidth = 0.2f;
inline constexpr float unitCellLineWidth = 0.25f;

inline constexpr float defaultAlpha = 0.0f;
inline constexpr float contactAtomAlpha = 0.5f;
inline constexpr float suppressedAtomAlpha = 0.5f;

} // namespace DrawingStyleConstants
