#pragma once
#include "atomflags.h"
#include "interactions.h"
#include "generic_atom_index.h"
#include <Eigen/Dense>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QColor>
#include <QStringList>
#include <QVector3D>
#include <memory>
#include <occ/core/bondgraph.h>
#include <occ/core/linear_algebra.h>


class ChemicalStructure : public QAbstractItemModel {
  Q_OBJECT
public:
  enum class StructureType {
    Cluster, // 0D
    Wire,    // 1D periodic
    Surface, // 2D periodic
    Crystal  // 3D periodic
  };

  enum class AtomColoring { Element, Fragment, Index };

  explicit ChemicalStructure(QObject *parent = nullptr);

  void setAtoms(const std::vector<QString> &elementSymbols,
                const std::vector<occ::Vec3> &positions,
                const std::vector<QString> &labels = {});
  void addAtoms(const std::vector<QString> &elementSymbols,
                const std::vector<occ::Vec3> &positions,
                const std::vector<QString> &labels = {});

  QStringList uniqueElementSymbols() const;

  virtual void setShowVanDerWaalsContactAtoms(bool state = false);
  virtual void updateBondGraph();

  inline const occ::Mat3N &atomicPositions() const { return m_atomicPositions; }
  inline const occ::IVec &atomicNumbers() const { return m_atomicNumbers; }
  inline const auto &labels() const { return m_labels; }
  inline int numberOfAtoms() const { return m_atomicNumbers.rows(); }

  // colors
  QColor atomColor(int atomIndex) const;
  void overrideAtomColor(int index, const QColor &);
  void resetAtomColorOverrides();
  void setAtomColoring(AtomColoring);

  virtual inline StructureType structureType() const {
    return StructureType::Cluster;
  }

  virtual occ::IVec atomicNumbersForIndices(const std::vector<GenericAtomIndex> &) const;

  virtual occ::Mat3N atomicPositionsForIndices(const std::vector<GenericAtomIndex> &) const;


  // fragments
  virtual void selectFragmentContaining(int);
  virtual void completeFragmentContaining(int);
  virtual void completeAllFragments();
  virtual void packUnitCells(const QPair<QVector3D, QVector3D> &);

  virtual void expandAtomsWithinRadius(float radius, bool selected);
  virtual void resetAtomsAndBonds(bool toSelection = false);

  virtual inline const occ::Vec3 &origin() const { return m_origin; }
  virtual void resetOrigin();
  virtual void setOrigin(const occ::Vec3 &);
  virtual float radius() const;
  inline virtual occ::Mat3 cellVectors() const {
    return occ::Mat3::Identity(3, 3);
  }

  occ::Vec covalentRadii() const;
  occ::Vec vdwRadii() const;

  virtual int fragmentIndexForAtom(int) const;
  virtual void deleteFragmentContainingAtomIndex(int atomIndex);
  virtual void deleteIncompleteFragments();
  virtual bool hasIncompleteFragments() const;
  virtual const std::vector<int> &atomsForFragment(int) const;
  virtual const std::pair<int, int> &atomsForBond(int) const;
  virtual const std::vector<std::pair<int, int>> &hydrogenBonds() const;
  virtual const std::vector<std::pair<int, int>> &covalentBonds() const;
  virtual const std::vector<std::pair<int, int>> &vdwContacts() const;


  // flags
  const AtomFlags &atomFlags(int) const;
  AtomFlags &atomFlags(int);
  void setAtomFlags(int, const AtomFlags &);
  void setAtomFlag(int idx, AtomFlag flag, bool on = true) {
    m_flags[idx].setFlag(flag, on);
  }
  inline bool testAtomFlag(int idx, AtomFlag flag) const {
    return m_flags[idx].testFlag(flag);
  }
  bool atomFlagsSet(int index, const AtomFlags &flags) const;
  bool anyAtomHasFlags(const AtomFlags &) const;
  bool allAtomsHaveFlags(const AtomFlags &) const;
  bool atomsHaveFlags(const std::vector<int> &, const AtomFlags &) const;
  std::vector<int> atomIndicesWithFlags(const AtomFlags &) const;
  void setFlagForAllAtoms(AtomFlag, bool on = true);
  void setFlagForAtoms(const std::vector<int> &, AtomFlag, bool on = true);
  void setFlagForAtomsFiltered(const AtomFlag &flagToSet, const AtomFlag &query,
                               bool on = true);

  [[nodiscard]] inline const DimerInteractions * interactions() const { return m_interactions; }
  [[nodiscard]] inline DimerInteractions * interactions() { return m_interactions; }

  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const;
  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsWithFlags(const AtomFlags &flags) const;


  // Abstract Item Model methods
  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  int columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex &index) const override;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

signals:
  void childAdded(QObject *);
  void childRemoved(QObject *);

protected:
  void childEvent(QChildEvent *childEvent) override;

private:
  int topLevelItemsCount() const;
  void deleteAtoms(const std::vector<int> &atomIndices);
  void deleteAtom(int atomIndex);
  void guessBondsBasedOnDistances();

  template <class Function>
  void depth_first_traversal(int atomId, Function &func) {
    m_bondGraph.depth_first_traversal(m_bondGraphVertices[atomId], func);
  }
  Eigen::Vector3d m_origin{0.0, 0.0, 0.0};

  AtomColoring m_atomColoring{AtomColoring::Element};
  ankerl::unordered_dense::map<int, QColor> m_atomColorOverrides;
  std::vector<QString> m_labels;
  occ::Mat3N m_atomicPositions;
  Eigen::VectorXi m_atomicNumbers;
  std::vector<AtomFlags> m_flags;
  std::vector<QColor> m_atomColors;

  // these are here for optimisation purposes, they need to be reconstructed if
  // the atoms are changed anyway.
  occ::core::graph::BondGraph m_bondGraph;
  std::vector<occ::core::graph::BondGraph::VertexDescriptor>
      m_bondGraphVertices;
  std::vector<occ::core::graph::BondGraph::EdgeDescriptor> m_bondGraphEdges;

  // all of these must be updated when the bondGraph is updated
  std::vector<std::vector<int>> m_fragments;
  std::vector<int> m_fragmentForAtom;
  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;
  bool m_bondsNeedUpdate{true};
  DimerInteractions *m_interactions{nullptr};
};
