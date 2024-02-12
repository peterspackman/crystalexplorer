#pragma once
#include "jobparameters.h"
#include "surface.h"
#include "surfaceproperty.h"

typedef QPair<QString, QVector<float>> SurfacePropertyProxy;

namespace sbf {
class File;
}

class SurfaceData {
public:
  static Surface *getData(const JobParameters &);
  static SurfacePropertyProxy getRequestedPropertyData(const JobParameters &);

private:
  static Surface *processSurfaceBlock(QTextStream &);
  static bool processVerticesBlock(QTextStream &, Surface *, int);
  static bool processIndicesBlock(QTextStream &, Surface *, int);
  static bool processVertexNormalsBlock(QTextStream &ts, Surface *, int);
  static bool processVertexPropertiesBlock(QTextStream &, Surface *);
  static bool processProperty(QTextStream &, Surface *, QString, int);
  static QVector<float> processPropertyData(QTextStream &, QString);
  static bool processAtomsInsideSurfaceBlock(QTextStream &, Surface *, int);
  static bool processAtomsOutsideSurfaceBlock(QTextStream &, Surface *, int);
  static bool processDiFaceAtoms(QTextStream &, Surface *, int);
  static bool processDeFaceAtoms(QTextStream &, Surface *, int);
  static bool readSurface(sbf::File &, Surface *);
  static bool readSurfaceProperties(sbf::File &, Surface *);
};
