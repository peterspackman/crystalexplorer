#pragma once
#include <QVector3D>
#include <QString>
#include "meshinstance.h"


struct SelectedAtom {
  int index{-1};
  int atomicNumber{-1};
  QVector3D position;
  QString label;
  QString fragmentLabel;
};

struct SelectedBond {
  int index{-1};
  SelectedAtom a;
  SelectedAtom b;
  QString fragmentLabel;
};

struct SelectedSurface {
  int index{-1};
  int faceIndex{-1};
  MeshInstance *surface{nullptr};
  float propertyValue{0.0};
  QString property{"None"};
};


using SelectionInfoVariant = std::variant<std::monostate, SelectedAtom, SelectedBond, SelectedSurface>;

QString getSelectionInformationLabelText(const SelectionInfoVariant &);
