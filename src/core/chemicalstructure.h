#pragma once
#include "atomflags.h"
#include "fragment.h"
#include "generic_atom_index.h"
#include "pair_energy_results.h"
#include "molecular_wavefunction.h"
#include "object_tree_model.h"
#include <Eigen/Dense>
#include <QVariant>
#include <QColor>
#include <QStringList>
#include <QVector3D>
#include <memory>
#include <occ/core/bondgraph.h>
#include <occ/core/linear_algebra.h>

using Transform = Eigen::Isometry3d;

struct FragmentPairs {
  struct SymmetryRelatedPair {
      FragmentDimer fragments;
      int uniquePairIndex{-1};
  };
  std::vector<FragmentDimer> uniquePairs;

  using MoleculeNeighbors = std::vector<SymmetryRelatedPair>;
  std::vector<MoleculeNeighbors> pairs;
};


class ChemicalStructure : public QObject {
  Q_OBJECT
public:

  struct FragmentState {
      int charge{0};
      int multiplicity{1};
  };

  using FragmentSymmetryRelation = std::pair<int, Transform>;

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
  void clearAtoms();
  void addAtoms(const std::vector<QString> &elementSymbols,
                const std::vector<occ::Vec3> &positions,
                const std::vector<QString> &labels = {});

  QStringList uniqueElementSymbols() const;
  std::vector<int> hydrogenBondDonors() const;
  QStringList uniqueHydrogenDonorElements() const;

  // assumed to be CIF for now
  inline void setFileContents(const QByteArray &contents) { m_fileContents = contents; }
  inline const QByteArray &fileContents() const { return m_fileContents; }

  inline void setFilename(const QString &filename) { m_filename = filename; }
  inline const QString &filename() const { return m_filename; }

  virtual void setShowVanDerWaalsContactAtoms(bool state = false);
  virtual void updateBondGraph();

  inline const occ::Mat3N &atomicPositions() const { return m_atomicPositions; }
  inline const occ::IVec &atomicNumbers() const { return m_atomicNumbers; }
  inline const auto &labels() const { return m_labels; }
  inline int numberOfAtoms() const { return m_atomicNumbers.rows(); }

  inline void setName(const QString &name) { m_name = name; }
  inline const auto &name() const { return m_name; }

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

  virtual bool getTransformation(const std::vector<GenericAtomIndex> &from, const std::vector<GenericAtomIndex> &to,
				 Eigen::Isometry3d &result) const;


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

  inline virtual occ::Vec3 cellAngles() const {
    return occ::Vec3(M_PI/2, M_PI/2, M_PI/2);
  }

  inline virtual occ::Vec3 cellLengths() const {
    return occ::Vec3(1.0, 1.0, 1.0);
  }

  virtual std::vector<int> completedFragments() const;
  virtual std::vector<int> selectedFragments() const;

  virtual FragmentState getSymmetryUniqueFragmentState(int) const;
  virtual void setSymmetryUniqueFragmentState(int, FragmentState);

  virtual const std::vector<Fragment> &symmetryUniqueFragments() const;
  virtual const std::vector<FragmentState> &symmetryUniqueFragmentStates() const;

  virtual Fragment makeFragment(const std::vector<GenericAtomIndex> &) const;
  virtual const std::vector<Fragment>& getFragments() const;

  occ::Vec covalentRadii() const;
  occ::Vec vdwRadii() const;

  virtual inline size_t numberOfFragments() const { return m_fragments.size(); }
  virtual int fragmentIndexForAtom(int) const;
  virtual void deleteFragmentContainingAtomIndex(int atomIndex);
  virtual void deleteIncompleteFragments();
  virtual bool hasIncompleteFragments() const;
  virtual bool hasIncompleteSelectedFragments() const;
  virtual const std::vector<int> &atomsForFragment(int) const;
  virtual std::vector<GenericAtomIndex> atomIndicesForFragment(int) const;
  virtual const std::pair<int, int> &atomsForBond(int) const;
  virtual const std::vector<std::pair<int, int>> &hydrogenBonds() const;
  virtual const std::vector<std::pair<int, int>> &covalentBonds() const;
  virtual const std::vector<std::pair<int, int>> &vdwContacts() const;

  FragmentSymmetryRelation findUniqueFragment(const std::vector<GenericAtomIndex> &) const;
  FragmentPairs findFragmentPairs() const;

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
  void toggleFlagForAllAtoms(AtomFlag);

  [[nodiscard]] inline PairInteractionResults *interactions() { return m_interactions; }

  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const;
  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsSurroundingAtoms(const std::vector<GenericAtomIndex> &, float radius) const;
  [[nodiscard]] virtual std::vector<GenericAtomIndex> atomsWithFlags(const AtomFlags &flags) const;

  [[nodiscard]] virtual std::vector<WavefunctionAndTransform> wavefunctionsAndTransformsForAtoms(const std::vector<GenericAtomIndex> &);

  [[nodiscard]] inline ObjectTreeModel* treeModel() { return m_treeModel; }
  [[nodiscard]] QString formulaSumForAtoms(const std::vector<GenericAtomIndex> &idxs, bool richText) const;

  [[nodiscard]] virtual QString chemicalFormula(bool richText = true) const;

signals:
  void childAdded(QObject *);
  void childRemoved(QObject *);

protected:
  void connectChildSignals(QObject *child);
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
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
  QString m_name{"structure"};

  // these are here for optimisation purposes, they need to be reconstructed if
  // the atoms are changed anyway.
  occ::core::graph::BondGraph m_bondGraph;
  std::vector<occ::core::graph::BondGraph::VertexDescriptor>
      m_bondGraphVertices;
  std::vector<occ::core::graph::BondGraph::EdgeDescriptor> m_bondGraphEdges;

  // all of these must be updated when the bondGraph is updated
  std::vector<Fragment> m_fragments;
  std::vector<Fragment> m_symmetryUniqueFragments;
  std::vector<FragmentState> m_symmetryUniqueFragmentStates;

  std::vector<int> m_fragmentForAtom;
  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;
  bool m_bondsNeedUpdate{true};

  QString m_filename;
  QByteArray m_fileContents;

  PairInteractionResults *m_interactions{nullptr};
  ObjectTreeModel * m_treeModel{nullptr};
};
