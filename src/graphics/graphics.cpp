#include <QMatrix4x4>
#include <QPainter>
#include <QQuaternion>
#include <QTextCursor>
#include <QTextDocument>
#include <QVector2D>
#include <QtDebug>
#include <cmath>

#include "colormap.h"
#include "graphics.h"
#include "mathconstants.h"
#include "settings.h"
#include "signed_distance_field.h"

namespace cx::graphics {

void addCircleToCircleRenderer(CircleRenderer &c, const QVector3D &position,
                               const QVector3D &right, const QVector3D &up,
                               const QColor &color) {
  QVector4D col(color.redF(), color.greenF(), color.blueF(), color.alphaF());
  c.addVertices({
      CircleVertex(position, right, up, col, {1, 1}),
      CircleVertex(position, right, up, col, {1, -1}),
      CircleVertex(position, right, up, col, {-1, -1}),
      CircleVertex(position, right, up, col, {-1, 1}),
  });
}

void addPlaneToCrystalPlaneRenderer(CrystalPlaneRenderer &c,
                                    const QVector3D &origin, const QVector3D &a,
                                    const QVector3D &b, const QColor &color) {
  QVector4D col(color.redF(), color.greenF(), color.blueF(), 0.5);
  c.addVertices({
      CrystalPlaneVertex(origin, a, b, col, {0, 0}),
      CrystalPlaneVertex(origin, a, b, col, {0, 1}),
      CrystalPlaneVertex(origin, a, b, col, {1, 1}),
      CrystalPlaneVertex(origin, a, b, col, {1, 0}),
  });
}

void addPartialDiskToCircleRenderer(CircleRenderer &c, const QVector3D &v0,
                                    const QVector3D &v1,
                                    const QVector3D &origin,
                                    const QColor &color) {
  QVector4D col(color.redF(), color.greenF(), color.blueF(), color.alphaF());

  float l0 = v0.length();
  float l1 = v1.length();
  QVector3D u_v0 = v0 / l0;
  QVector3D u_v1 = v1 / l1;
  QVector3D right = u_v0;
  QVector3D up = (u_v1 - QVector3D::dotProduct(u_v0, u_v1) * u_v0).normalized();
  float scale = std::min(l0, l1);
  right *= scale;
  up *= scale;

  // cos(2 theta) = 1 - 2 * sin(theta) * sin(theta)
  // i.e. sin(theta) * sin(theta) = - dp / 2
  float cosTheta = QVector3D::dotProduct(u_v0, u_v1);

  c.addVertices({CircleVertex(origin, right, up, col, {1, 1}, 0.0, cosTheta),
                 CircleVertex(origin, right, up, col, {1, -1}, 0.0, cosTheta),
                 CircleVertex(origin, right, up, col, {-1, -1}, 0.0, cosTheta),
                 CircleVertex(origin, right, up, col, {-1, 1}, 0.0, cosTheta)});
}

void addCurvedLineBetweenVectors(CircleRenderer &c, QVector3D v0, QVector3D v1,
                                 QVector3D origin, QColor color) {
  QVector4D col(color.redF(), color.greenF(), color.blueF(), color.alphaF());

  float l0 = v0.length();
  float l1 = v1.length();
  QVector3D u_v0 = v0 / l0;
  QVector3D u_v1 = v1 / l1;
  QVector3D right = u_v0;
  QVector3D up = (u_v1 - QVector3D::dotProduct(u_v0, u_v1) * u_v0).normalized();
  float scale = std::min(l0, l1);
  right *= scale;
  up *= scale;

  // cos(2 theta) = 1 - 2 * sin(theta) * sin(theta)
  // i.e. sin(theta) * sin(theta) = - dp / 2
  float cosTheta = QVector3D::dotProduct(u_v0, u_v1);

  c.addVertices(
      {CircleVertex(origin, right, up, col, {1, 1}, 0.9f, cosTheta),
       CircleVertex(origin, right, up, col, {1, -1}, 0.9f, cosTheta),
       CircleVertex(origin, right, up, col, {-1, -1}, 0.9f, cosTheta),
       CircleVertex(origin, right, up, col, {-1, 1}, 0.9f, cosTheta)});
}

void addCurvedLineToLineRenderer(LineRenderer &r, const QVector3D &pointA,
                                 const QVector3D &pointB,
                                 const QVector3D &origin, float lineWidth,
                                 const QColor &color) {
  QVector3D col(color.redF(), color.greenF(), color.blueF());

  float l0 = pointA.length();
  float l1 = pointB.length();
  QVector3D u_v0 = pointA / l0;
  QVector3D u_v1 = pointB / l1;
  float cosTheta = QVector3D::dotProduct(u_v0, u_v1);
  float theta = acosf(cosTheta) * 180.0 / PI;
  QVector3D up = QVector3D::crossProduct(u_v0, u_v1);
  QVector3D sweepLine = pointA;
  QVector3D p1 = sweepLine + origin;
  std::vector<LineVertex> vertices;
  const float spacing = 2.0;
  int num_pts = static_cast<int>(theta / spacing);
  for (int t = 0.0; t < num_pts; t++) {
    QVector3D p2 = origin + QQuaternion::fromAxisAndAngle(up, (t + 1) * spacing)
                                .rotatedVector(sweepLine);
    vertices.emplace_back(
        LineVertex(p1, p2, col, col, QVector2D(-1, 1), lineWidth));
    vertices.emplace_back(
        LineVertex(p1, p2, col, col, QVector2D(-1, -1), lineWidth));
    vertices.emplace_back(
        LineVertex(p1, p2, col, col, QVector2D(1, 1), lineWidth));
    vertices.emplace_back(
        LineVertex(p1, p2, col, col, QVector2D(1, -1), lineWidth));
    p1 = p2;
  }
  r.addLines(vertices);
}

LineRenderer *createLineRenderer(const QVector3D &pointA,
                                 const QVector3D &pointB, float lineWidth,
                                 const QColor &color) {
  QVector3D col(color.redF(), color.greenF(), color.blueF());
  return new LineRenderer({
      LineVertex(pointA, pointB, col, col, QVector2D(-1, 1), lineWidth),
      LineVertex(pointA, pointB, col, col, QVector2D(-1, -1), lineWidth),
      LineVertex(pointA, pointB, col, col, QVector2D(1, 1), lineWidth),
      LineVertex(pointA, pointB, col, col, QVector2D(1, -1), lineWidth),
  });
}

void addLineToLineRenderer(LineRenderer &r, const QVector3D &pointA,
                           const QVector3D &pointB, float lineWidth,
                           const QColor &color, const QVector3D &id, bool selected) {
  QVector3D col(color.redF(), color.greenF(), color.blueF());
  r.addLines({
      LineVertex(pointA, pointB, col, col, QVector2D(-1, 1), lineWidth, id),
      LineVertex(pointA, pointB, col, col, QVector2D(-1, -1), lineWidth, id),
      LineVertex(pointA, pointB, col, col, QVector2D(1, 1), lineWidth, id),
      LineVertex(pointA, pointB, col, col, QVector2D(1, -1), lineWidth, id),
  });
  if(selected) {
      QColor selectionColor =
          settings::readSetting(settings::keys::SELECTION_COLOR).toString();
      QVector3D y(selectionColor.redF(), selectionColor.greenF(), selectionColor.blueF());
      r.addLines({
          LineVertex(pointA, pointB, y, y, QVector2D(-1, 1.01), lineWidth*2),
          LineVertex(pointA, pointB, y, y, QVector2D(-1, -1.01), lineWidth*2),
          LineVertex(pointA, pointB, y, y, QVector2D(1, 1.01), lineWidth*2),
          LineVertex(pointA, pointB, y, y, QVector2D(1, -1.01), lineWidth*2),
      });
  }
}

void addDashedLineToLineRenderer(LineRenderer &r, const QVector3D &pointA,
                                 const QVector3D &pointB, float lineWidth,
                                 const QColor &color, float dashLength,
                                 float dashSpacing) {
  // TODO sort this out in the shader, should be much more efficient
  size_t num_dashes = (pointB - pointA).length() / (dashLength + dashSpacing);
  QVector3D col(color.redF(), color.greenF(), color.blueF());
  QVector3D directionAB = (pointB - pointA).normalized();
  QVector3D start = pointA;
  QVector3D end = pointA + (directionAB * dashLength);
  for (size_t i = 0; i < num_dashes; i++) {
    r.addLines({
        LineVertex(start, end, col, col, QVector2D(-1, 1), lineWidth),
        LineVertex(start, end, col, col, QVector2D(-1, -1), lineWidth),
        LineVertex(start, end, col, col, QVector2D(1, 1), lineWidth),
        LineVertex(start, end, col, col, QVector2D(1, -1), lineWidth),
    });
    start = end + (directionAB * dashSpacing);
    end += directionAB * (dashSpacing + dashLength);
  }
  // add a little line at the end if necessary
  if ((pointB - start).length() > 0.001) {
    r.addLines({
        LineVertex(start, pointB, col, col, QVector2D(-1, 1), lineWidth),
        LineVertex(start, pointB, col, col, QVector2D(-1, -1), lineWidth),
        LineVertex(start, pointB, col, col, QVector2D(1, 1), lineWidth),
        LineVertex(start, pointB, col, col, QVector2D(1, -1), lineWidth),
    });
  }
}

void addSphereToSphereRenderer(SphereImpostorRenderer *r,
                               const QVector3D &position, const QColor &color,
                               float radius, const QVector3D &id,
                               bool selected) {

  QVector4D col(selected ? -color.redF() - 0.0001f : color.redF(),
                color.greenF(), color.blueF(), color.alphaF());
  r->addVertices({SphereImpostorVertex(position, col, radius, {-1, 1}, id),
                  SphereImpostorVertex(position, col, radius, {-1, -1}, id),
                  SphereImpostorVertex(position, col, radius, {1, 1}, id),
                  SphereImpostorVertex(position, col, radius, {1, -1}, id)});
}

void addEllipsoidToEllipsoidRenderer(EllipsoidRenderer *r,
                                     const QVector3D &position,
                                     const QMatrix3x3 &transform,
                                     const QColor &color, const QVector3D &id,
                                     bool selected) {
  QVector3D col(selected ? -color.redF() - 0.0001f : color.redF(),
                -color.greenF() - 0.0001f, color.blueF());
  r->addInstance(EllipsoidInstance(
      position, QVector3D(transform(0, 0), transform(1, 0), transform(2, 0)),
      QVector3D(transform(0, 1), transform(1, 1), transform(2, 1)),
      QVector3D(transform(0, 2), transform(1, 2), transform(2, 2)), col, id));
}

void addSphereToEllipsoidRenderer(EllipsoidRenderer *r,
                                  const QVector3D &position,
                                  const QColor &color, float radius,
                                  const QVector3D &id, bool selected) {
  QVector3D col(selected ? -color.redF() - 0.0001f : color.redF(),
                color.greenF(), color.blueF());
  r->addInstance(EllipsoidInstance(position, QVector3D(radius, 0.0f, 0.0f),
                                   QVector3D(0.0f, radius, 0.0f),
                                   QVector3D(0.0f, 0.0f, radius), col, id));
}

void addTextToBillboardRenderer(BillboardRenderer &b, const QVector3D &position,
                                const QString &text) {
  if (!b.hasTextureForText(text)) {
    QString fontName =
        settings::readSetting(settings::keys::TEXT_FONT_FAMILY).toString();
    int fontSize =
        settings::readSetting(settings::keys::TEXT_FONT_SIZE).toInt();
    QFont font(fontName, fontSize);

    // Create a QTextDocument for rich text rendering
    QTextDocument doc;
    doc.setDefaultFont(font);
    doc.setHtml(text);

    // Set text color to black
    QTextCursor cursor(&doc);
    cursor.select(QTextCursor::Document);
    QTextCharFormat format;
    format.setForeground(Qt::black);
    cursor.mergeCharFormat(format);

    // Calculate the size of the text
    doc.setTextWidth(-1); // Set to -1 to get the size without wrapping
    QSizeF docSize = doc.size();

    // Calculate padding based on font size
    int padding = qRound(fontSize * 0.25); // 25% of font size for padding

    // Create an image with appropriate size
    int pixelsWide = qCeil(docSize.width()) + 2 * padding;
    int pixelsHigh = qCeil(docSize.height()) + 2 * padding;
    QImage img(QSize(pixelsWide, pixelsHigh), QImage::Format_Grayscale8);
    img.fill(Qt::white);

    // Set up the painter
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Draw the text
    painter.translate(padding, padding);
    doc.drawContents(&painter);
    painter.end();

    QImage sdf = signed_distance_transform_2d(img);
    QOpenGLTexture *texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    texture->setSize(sdf.width(), sdf.height());
    texture->setFormat(QOpenGLTexture::R8_UNorm);
    texture->allocateStorage();
    texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8,
                     sdf.mirrored().constBits());
    texture->setMinificationFilter(QOpenGLTexture::Linear);
    texture->setMagnificationFilter(QOpenGLTexture::Linear);
    texture->setWrapMode(QOpenGLTexture::ClampToEdge);
    b.addVertices(
        {BillboardVertex(position, QVector2D(pixelsWide, pixelsHigh), {0, 1}),
         BillboardVertex(position, QVector2D(pixelsWide, pixelsHigh), {0, 0}),
         BillboardVertex(position, QVector2D(pixelsWide, pixelsHigh), {1, 1}),
         BillboardVertex(position, QVector2D(pixelsWide, pixelsHigh), {1, 0})},
        text, texture);
  } else {
    b.addVertices({BillboardVertex(position, QVector2D(1, 1), {0, 1}),
                   BillboardVertex(position, QVector2D(1, 1), {0, 0}),
                   BillboardVertex(position, QVector2D(1, 1), {1, 1}),
                   BillboardVertex(position, QVector2D(1, 1), {1, 0})},
                  text);
  }
}

void addCylinderToCylinderRenderer(CylinderImpostorRenderer *r,
                                   const QVector3D &pointA,
                                   const QVector3D &pointB,
                                   const QColor &colorA, const QColor &colorB,
                                   float radius, const QVector3D &id,
                                   bool selectedA, bool selectedB) {
  QVector3D colA(selectedA ? -colorA.redF() : colorA.redF(), colorA.greenF(),
                 colorA.blueF());
  QVector3D colB(selectedB ? -colorB.redF() : colorB.redF(), colorB.greenF(),
                 colorB.blueF());
  r->addVertices({CylinderImpostorVertex(pointA, pointB, colA, colB,
                                         QVector3D(-1, 1, -1), id, radius),
                  CylinderImpostorVertex(pointA, pointB, colA, colB,
                                         QVector3D(-1, -1, -1), id, radius),
                  CylinderImpostorVertex(pointA, pointB, colA, colB,
                                         QVector3D(1, 1, -1), id, radius),
                  CylinderImpostorVertex(pointA, pointB, colA, colB,
                                         QVector3D(1, 1, 1), id, radius),
                  CylinderImpostorVertex(pointA, pointB, colA, colB,
                                         QVector3D(1, -1, -1), id, radius),
                  CylinderImpostorVertex(pointA, pointB, colA, colB,
                                         QVector3D(1, -1, 1), id, radius)});
}

void addCylinderToCylinderRenderer(CylinderRenderer *r, const QVector3D &pointA,
                                   const QVector3D &pointB,
                                   const QColor &colorA, const QColor &colorB,
                                   float radius, const QVector3D &id,
                                   bool selectedA, bool selectedB) {
  QVector3D colA(selectedA ? -colorA.redF() : colorA.redF(), colorA.greenF(),
                 colorA.blueF());
  QVector3D colB(selectedB ? -colorB.redF() : colorB.redF(), colorB.greenF(),
                 colorB.blueF());
  r->addInstance(CylinderInstance(radius, pointA, pointB, colA, colB, id, id));
}

void viewDownVector(const QVector3D &v, QMatrix4x4 &mat) {
  /* For the rotation:
   rotation axis = v x (0,0,1) = (v.y, -v.x, 0)
   rotation angle = acos ( v dot (0,0,1) ) = acos(v.z)
   */
  QMatrix3x3 rot;
  // must normalize!
  auto n = v.normalized();
  // vxy = (v.x, v.y)
  QVector2D vxy(n.x(), n.y());
  vxy.normalize();
  float theta = acos(n.z()) * 180 / PI;
  QQuaternion q =
      QQuaternion::fromAxisAndAngle(QVector3D(vxy.y(), -vxy.x(), 0.0), theta);
  QVector3D scale(mat.column(0).length(), mat.column(1).length(),
                  mat.column(2).length());
  mat.setToIdentity();
  mat(0, 0) = scale.x();
  mat(1, 1) = scale.y();
  mat(2, 2) = scale.z();
  mat.rotate(q);
}

std::vector<PointCloudVertex>
makePointCloudVertices(const Mesh &pointCloud, ColorSettings colorSettings) {
  const auto numVertices = pointCloud.numberOfVertices();
  if (numVertices <= 0)
    return {};

  std::vector<PointCloudVertex> vertices;
  const auto &pc_vertices = pointCloud.vertices();
  const auto &prop = pointCloud.vertexProperty(colorSettings.property);
  if (prop.size() == 0) {
    colorSettings.minValue = 0.0f;
    colorSettings.maxValue = 0.0f;
  } else if (colorSettings.findRange) {
    colorSettings.minValue = prop.minCoeff();
    colorSettings.maxValue = prop.maxCoeff();
  }

  QVector3D vec;
  QVector3D col;
  for (auto i = 0; i < numVertices; i++) {
    vec.setX(static_cast<float>(pc_vertices(0, i)));
    vec.setY(static_cast<float>(pc_vertices(1, i)));
    vec.setZ(static_cast<float>(pc_vertices(2, i)));
    QColor color = Qt::black;

    if (prop.rows() > i) {
      float x = (static_cast<float>(prop(i)) - colorSettings.minValue) /
                (colorSettings.maxValue - colorSettings.minValue);
      color = linearColorMap(x, colorSettings.colorMap);
    }

    col.setX(color.redF());
    col.setY(color.greenF());
    col.setZ(color.blueF());
    vertices.emplace_back(vec, col);
  }
  return vertices;
}

} // namespace cx::graphics
