#pragma once
#include "chemicalstructure.h"
#include "isosurface_parameters.h"
#include "taskmanager.h"
#include <QProcessEnvironment>
#include <QObject>

namespace volume {

class IsosurfaceCalculator : public QObject {
  Q_OBJECT
public:
  IsosurfaceCalculator(QObject *parent = nullptr);

  void setTaskManager(TaskManager *);
  void start(isosurface::Parameters);
  QString surfaceName();

signals:
  void calculationComplete(isosurface::Result);

private slots:
  void surfaceComplete();

private:
  TaskManager *m_taskManager{nullptr};
  ChemicalStructure *m_structure{nullptr};
  bool m_deleteWorkingFiles{false};
  QString m_occExecutable{"occ"};
  QProcessEnvironment m_environment;
  QString m_name;
  QString m_filename;
  QString m_defaultProperty{"dnorm"};
  isosurface::Parameters m_parameters;
  std::vector<GenericAtomIndex> m_atomsInside;
  std::vector<GenericAtomIndex> m_atomsOutside;
  occ::IVec m_nums_inside;
  occ::IVec m_nums_outside;
};

} // namespace volume
