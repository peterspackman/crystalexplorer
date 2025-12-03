#pragma once
#include "chemicalstructure.h"
#include "isosurface_parameters.h"
#include "taskmanager.h"
#include <QObject>
#include <QProcessEnvironment>

namespace volume {

class IsosurfaceCalculator : public QObject {
  Q_OBJECT
public:
  IsosurfaceCalculator(QObject *parent = nullptr);

  void setTaskManager(TaskManager *);
  void start(isosurface::Parameters);

signals:
  void calculationComplete(isosurface::Result);
  void errorOccurred(QString);

private slots:
  void surfaceComplete();

private:
  TaskManager *m_taskManager{nullptr};
  ChemicalStructure *m_structure{nullptr};
  bool m_deleteWorkingFiles{false};
  QString m_occExecutable{"occ"};
  QProcessEnvironment m_environment;
  QString m_name;
  QStringList m_fileNames;
  isosurface::Parameters m_parameters;
  std::vector<GenericAtomIndex> m_atomsInside;
  std::vector<GenericAtomIndex> m_atomsOutside;
  occ::IVec m_nums_inside;
  occ::IVec m_nums_outside;
};

} // namespace volume
