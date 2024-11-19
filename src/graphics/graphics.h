#pragma once
#include <QVector3D>

#include "billboardrenderer.h"
#include "circlerenderer.h"
#include "crystalplanerenderer.h"
#include "cylinderimpostorrenderer.h"
#include "cylinderrenderer.h"
#include "ellipsoidrenderer.h"
#include "linerenderer.h"
#include "sphereimpostorrenderer.h"

#include "pointcloudrenderer.h"
#include "colormap.h"
#include "mesh.h"

namespace cx::graphics {

struct ColorSettings {
    QString property{"None"};
    bool findRange{true};
    float minValue{0.0};
    float maxValue{0.0};
    ColorMapName colorMap{ColorMapName::Viridis};
};

void addCircleToCircleRenderer(CircleRenderer &c,
				    const QVector3D &position,
				    const QVector3D &right,
				    const QVector3D &up,
				    const QColor &color);
void
addPlaneToCrystalPlaneRenderer(CrystalPlaneRenderer &c,
			     const QVector3D &origin,
			     const QVector3D &a,
			     const QVector3D &b,
			     const QColor &color);

void addPartialDiskToCircleRenderer(CircleRenderer &c,
					 const QVector3D &v0,
					 const QVector3D &v1,
					 const QVector3D &origin,
					 const QColor &color);
LineRenderer *createLineRenderer(const QVector3D &pointA,
				      const QVector3D &pointB,
				      float lineWidth, const QColor &color);
void addLineToLineRenderer(LineRenderer &r, const QVector3D &pointA,
				const QVector3D &pointB, float lineWidth,
                const QColor &color, const QVector3D &idA = {}, bool selected=false);
void addDashedLineToLineRenderer(LineRenderer &r,
				      const QVector3D &pointA,
				      const QVector3D &pointB,
				      float lineWidth, const QColor &color,
				      float dashLength = 0.1,
				      float dashSpace = 0.1);
void addCurvedLineBetweenVectors(CircleRenderer &r, QVector3D v0,
				      QVector3D v1, QVector3D origin,
				      QColor color);
void addCurvedLineToLineRenderer(LineRenderer &r,
				      const QVector3D &pointA,
				      const QVector3D &pointB,
				      const QVector3D &origin,
				      float lineWidth, const QColor &color);
void addSphereToSphereRenderer(SphereImpostorRenderer *r,
				    const QVector3D &position,
				    const QColor &color, float radius,
				    const QVector3D &id = {},
				    bool selected = false);
void addCylinderToCylinderRenderer(
  CylinderImpostorRenderer *r, const QVector3D &pointA,
  const QVector3D &pointB, const QColor &colorA, const QColor &colorB,
  float radius, const QVector3D &id = {}, bool selectedA = false,
  bool selectedB = false);
void addCylinderToCylinderRenderer(
  CylinderRenderer *r, const QVector3D &pointA, const QVector3D &pointB,
  const QColor &colorA, const QColor &colorB, float radius,
  const QVector3D &id = {}, bool selectedA = false, bool selectedB = false);
void addEllipsoidToEllipsoidRenderer(EllipsoidRenderer *,
					  const QVector3D &position,
					  const QMatrix3x3 &transform,
					  const QColor &color,
					  const QVector3D &id = {},
					  bool selected = false);
void addSphereToEllipsoidRenderer(EllipsoidRenderer *r,
				       const QVector3D &position,
				       const QColor &color, float radius,
				       const QVector3D &id = {},
				       bool selected = false);
void addTextToBillboardRenderer(BillboardRenderer &b,
				     const QVector3D &position,
				     const QString &text);
void viewDownVector(const QVector3D &, QMatrix4x4 &);

std::vector<PointCloudVertex> makePointCloudVertices(const Mesh&, ColorSettings settings = {});

}
