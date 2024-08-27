#pragma once
#include "scene.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QTabWidget>
#include <QWidget>

enum class AtomDescription {
  SiteLabel,
  UnitCellShift,
  Hybrid,
  Coordinates,
  CartesianInfo,
  FractionalInfo
};

class AtomInfoDocument : public QWidget {
  Q_OBJECT

public:
  explicit AtomInfoDocument(QWidget *parent = nullptr);

  void updateScene(Scene *scene);
  QTextDocument *document(int index) const;

private:
  Scene *m_scene{nullptr};
  QTabWidget *m_tabWidget{nullptr};
  QTextEdit *m_cartesianCoordinates{nullptr};
  QTextEdit *m_fractionalCoordinates{nullptr};


  void setupUI();
  void populateDocument();
  void resetCursorsToBeginning();

  void insertAtomicCoordinates(QTextEdit *, AtomDescription atomDescription);
  void
  insertAtomicCoordinatesSection(QTextEdit *, const QString &title,
                                 ChemicalStructure *structure,
                                 const std::vector<GenericAtomIndex> &atoms,
                                 AtomDescription atomDescription);
  void insertAtomicCoordinatesHeader(QTextCursor &, const QString &title,
                                     int numAtoms,
                                     AtomDescription atomDescription);
  void insertAtomicCoordinates(QTextCursor &, ChemicalStructure *structure,
                               const std::vector<GenericAtomIndex> &atoms,
                               AtomDescription atomDescription);
};
