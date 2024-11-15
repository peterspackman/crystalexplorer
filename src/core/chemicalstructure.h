#pragma once
#include "adp.h"
#include "atomflags.h"
#include "contact_settings.h"
#include "cell_index.h"
#include "close_contact_criteria.h"
#include "fragment.h"
#include "fragment_index.h"
#include "generic_atom_index.h"
#include "hbond_criteria.h"
#include "molecular_wavefunction.h"
#include "object_tree_model.h"
#include "pair_energy_results.h"
#include "slab_options.h"
#include <Eigen/Dense>
#include <QColor>
#include <QStringList>
#include <QVariant>
#include <QVector3D>
#include <optional>
#include <functional>
#include <memory>
#include <occ/core/bondgraph.h>
#include <occ/core/linear_algebra.h>
#include "json.h"

using Transform = Eigen::Isometry3d;

using MaybeFragment = std::optional<std::reference_wrapper<const Fragment>>;

using FragmentMap =
    ankerl::unordered_dense::map<FragmentIndex, Fragment, FragmentIndexHash>;

struct FragmentPairSettings {
  FragmentIndex keyFragment = FragmentIndex{-1};
  bool allowInversion = true;
};

struct FragmentPairs {
  struct SymmetryRelatedPair {
    FragmentDimer fragments;
    int uniquePairIndex{-1};
  };
  bool allowInversion{false};
  using MoleculeNeighbors = std::vector<SymmetryRelatedPair>;

  std::vector<FragmentDimer> uniquePairs;
  ankerl::unordered_dense::map<FragmentIndex, MoleculeNeighbors,
                               FragmentIndexHash>
      pairs;
};

struct StructureFrame;

class ChemicalStructure : public QObject {
  Q_OBJECT
public:
  using FragmentSymmetryRelation = std::pair<FragmentIndex, Transform>;

  enum class StructureType {
    Cluster, // 0D
    Wire,    // 1D periodic
    Surface, // 2D periodic
    Crystal  // 3D periodic
  };

  enum class CoordinateConversion {
    CartToFrac,
    FracToCart
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
  inline void setFileContents(const QByteArray &contents) {
    m_fileContents = contents;
  }
  inline const QByteArray &fileContents() const { return m_fileContents; }

  inline void setFilename(const QString &filename) { m_filename = filename; }
  inline const QString &filename() const { return m_filename; }

  virtual void setShowContacts(const ContactSettings &settings = {});
  virtual void updateBondGraph();

  occ::Vec3 atomPosition(GenericAtomIndex) const;
  inline const occ::Mat3N &atomicPositions() const { return m_atomicPositions; }
  inline const occ::IVec &atomicNumbers() const { return m_atomicNumbers; }
  inline const auto &labels() const { return m_labels; }
  inline int numberOfAtoms() const { return m_atomicNumbers.rows(); }

  inline void setName(const QString &name) { m_name = name; }
  inline const auto &name() const { return m_name; }

  virtual occ::Mat3N convertCoordinates(const occ::Mat3N &pos, CoordinateConversion) const;

  QString getFragmentLabel(const FragmentIndex &);

  // colors
  QColor atomColor(GenericAtomIndex atomIndex) const;
  void overrideAtomColor(GenericAtomIndex, const QColor &);
  void resetAtomColorOverrides();
  void setAtomColoring(AtomColoring);

  void setColorForAtomsWithFlags(const AtomFlags &, const QColor &);

  virtual inline StructureType structureType() const {
    return StructureType::Cluster;
  }

  virtual int genericIndexToIndex(const GenericAtomIndex &) const;
  virtual GenericAtomIndex indexToGenericIndex(int) const;

  virtual occ::IVec
  atomicNumbersForIndices(const std::vector<GenericAtomIndex> &) const;

  virtual occ::Mat3N
  atomicPositionsForIndices(const std::vector<GenericAtomIndex> &) const;
  virtual std::vector<QString>
  labelsForIndices(const std::vector<GenericAtomIndex> &) const;

  virtual bool getTransformation(const std::vector<GenericAtomIndex> &from,
                                 const std::vector<GenericAtomIndex> &to,
                                 Eigen::Isometry3d &result) const;

  virtual std::vector<GenericAtomIndex>
  getAtomIndicesUnderTransformation(const std::vector<GenericAtomIndex> &idxs,
                                    const Eigen::Isometry3d &result) const;

  // fragments
  virtual void selectFragmentContaining(int);
  virtual void selectFragmentContaining(GenericAtomIndex);
  virtual void completeFragmentContaining(int);
  virtual void completeFragmentContaining(GenericAtomIndex);
  virtual void completeAllFragments();
  virtual void buildSlab(SlabGenerationOptions);

  virtual void expandAtomsWithinRadius(float radius, bool selected);
  virtual void resetAtomsAndBonds(bool toSelection = false);

  virtual inline const occ::Vec3 &origin() const { return m_origin; }
  virtual void resetOrigin();
  virtual void setOrigin(const occ::Vec3 &);
  virtual float radius() const;

  virtual void setCellVectors(const occ::Mat3 &) {}
  inline virtual occ::Mat3 cellVectors() const {
    return occ::Mat3::Identity(3, 3);
  }

  inline virtual occ::Vec3 cellAngles() const {
    return occ::Vec3(M_PI / 2, M_PI / 2, M_PI / 2);
  }

  inline virtual occ::Vec3 cellLengths() const {
    return occ::Vec3(1.0, 1.0, 1.0);
  }

  virtual std::vector<FragmentIndex> completedFragments() const;
  virtual std::vector<FragmentIndex> selectedFragments() const;

  virtual Fragment::State getSymmetryUniqueFragmentState(FragmentIndex) const;
  virtual void setSymmetryUniqueFragmentState(FragmentIndex, Fragment::State);

  virtual const FragmentMap &symmetryUniqueFragments() const;

  virtual Fragment makeFragment(const std::vector<GenericAtomIndex> &) const;
  const FragmentMap &getFragments() const;
  MaybeFragment getFragment(const FragmentIndex &) const;

  occ::Vec covalentRadii() const;
  occ::Vec vdwRadii() const;

  inline size_t numberOfFragments() const { return m_fragments.size(); }
  FragmentIndex fragmentIndexForAtom(int) const;
  virtual FragmentIndex fragmentIndexForGeneralAtom(GenericAtomIndex) const;

  MaybeFragment getFragmentForAtom(int) const;
  MaybeFragment getFragmentForAtom(GenericAtomIndex) const;

  virtual void deleteFragmentContainingAtomIndex(int atomIndex);
  virtual void deleteIncompleteFragments();
  virtual void deleteAtoms(const std::vector<GenericAtomIndex> &);
  virtual bool hasIncompleteFragments() const;
  virtual bool hasIncompleteSelectedFragments() const;

  std::vector<GenericAtomIndex>
      atomIndicesForFragment(FragmentIndex) const;
  const std::pair<int, int> &atomsForBond(int) const;
  std::pair<GenericAtomIndex, GenericAtomIndex>
  atomIndicesForBond(int) const;
  std::vector<HBondTriple>
  hydrogenBonds(const HBondCriteria & = {}) const;
  std::vector<CloseContactPair>
  closeContacts(const CloseContactCriteria & = {}) const;
  const std::vector<std::pair<int, int>> &covalentBonds() const;

  virtual CellIndexSet occupiedCells() const;

  FragmentSymmetryRelation
  findUniqueFragment(const std::vector<GenericAtomIndex> &) const;
  virtual FragmentPairs
  findFragmentPairs(FragmentPairSettings settings = {}) const;

  virtual QColor getFragmentColor(FragmentIndex) const;
  virtual void setFragmentColor(FragmentIndex, const QColor &);
  virtual void setAllFragmentColors(const FragmentColorSettings &);

  // dynamics data
  inline bool hasFrameData() const { return frameCount() > 0; }

  inline virtual int frameCount() const { return 0; }
  inline virtual void addFrame(const StructureFrame &) {}

  inline virtual void removeFrame(int index) {}
  inline virtual void setCurrentFrameIndex(int index) {}
  inline virtual int getCurrentFrameIndex() const { return 0; }

  // flags
  const AtomFlags &atomFlags(GenericAtomIndex) const;
  void setAtomFlags(GenericAtomIndex, const AtomFlags &);
  void setAtomFlag(GenericAtomIndex idx, AtomFlag flag, bool on = true);
  void toggleAtomFlag(GenericAtomIndex idx, AtomFlag flag);
  bool testAtomFlag(GenericAtomIndex idx, AtomFlag flag) const;

  bool atomFlagsSet(GenericAtomIndex index, const AtomFlags &flags) const;
  bool anyAtomHasFlags(const AtomFlags &) const;
  bool allAtomsHaveFlags(const AtomFlags &) const;
  bool atomsHaveFlags(const std::vector<GenericAtomIndex> &,
                      const AtomFlags &) const;
  void setFlagForAllAtoms(AtomFlag, bool on = true);
  void setFlagForAtoms(const std::vector<GenericAtomIndex> &, AtomFlag,
                       bool on = true);
  void setFlagForAtomsFiltered(const AtomFlag &flagToSet, const AtomFlag &query,
                               bool on = true);
  void toggleFlagForAllAtoms(AtomFlag);

  // pair interactions
  [[nodiscard]] inline const PairInteractions *pairInteractions() const {
    return m_interactions;
  }

  [[nodiscard]] inline PairInteractions *pairInteractions() {
    return m_interactions;
  }

  // atom filtering
  [[nodiscard]] virtual std::vector<GenericAtomIndex>
  atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const;
  [[nodiscard]] virtual std::vector<GenericAtomIndex>
  atomsSurroundingAtoms(const std::vector<GenericAtomIndex> &,
                        float radius) const;
  [[nodiscard]] virtual std::vector<GenericAtomIndex>
  atomsWithFlags(const AtomFlags &flags, bool set = true) const;

  [[nodiscard]] virtual std::vector<WavefunctionAndTransform>
  wavefunctionsAndTransformsForAtoms(const std::vector<GenericAtomIndex> &);

  [[nodiscard]] inline ObjectTreeModel *treeModel() { return m_treeModel; }

  [[nodiscard]] QString getFragmentLabelForAtoms(const std::vector<GenericAtomIndex> &idxs);
  [[nodiscard]] QString
  formulaSumForAtoms(const std::vector<GenericAtomIndex> &idxs,
                     bool richText) const;

  [[nodiscard]] virtual QString chemicalFormula(bool richText = true) const;
  [[nodiscard]] virtual std::vector<AtomicDisplacementParameters>
  atomicDisplacementParametersForAtoms(
      const std::vector<GenericAtomIndex> &) const;

  [[nodiscard]] virtual AtomicDisplacementParameters
      atomicDisplacementParameters(GenericAtomIndex) const;

  std::vector<GenericAtomIndex> atomIndices() const;
  [[nodiscard]] virtual nlohmann::json toJson() const;
  virtual bool fromJson(const nlohmann::json &);

signals:
  void atomsChanged();
  void childAdded(QObject *);
  void childRemoved(QObject *);

protected:
  void connectChildSignals(QObject *child);
  bool eventFilter(QObject *obj, QEvent *event) override;

  occ::Mat3N m_atomicPositions;
  Eigen::VectorXi m_atomicNumbers;
  std::vector<QString> m_labels;

  FragmentMap m_fragments;
  ankerl::unordered_dense::map<FragmentIndex, QString, FragmentIndexHash> m_fragmentLabels;
  std::vector<FragmentIndex> m_fragmentForAtom;
  FragmentMap m_symmetryUniqueFragments;

  std::vector<std::pair<int, int>> m_covalentBonds;
  std::vector<std::pair<int, int>> m_vdwContacts;
  std::vector<std::pair<int, int>> m_hydrogenBonds;

  ankerl::unordered_dense::map<GenericAtomIndex, AtomFlags, GenericAtomIndexHash> m_flags;
  Eigen::Vector3d m_origin{0.0, 0.0, 0.0};

private:
  void deleteAtomsByOffset(const std::vector<int> &atomIndices);
  void deleteAtom(int atomIndex);
  void guessBondsBasedOnDistances();

  template <class Function>
  void depth_first_traversal(int atomId, Function &func) {
    m_bondGraph.depth_first_traversal(m_bondGraphVertices[atomId], func);
  }

  AtomColoring m_atomColoring{AtomColoring::Element};
  ankerl::unordered_dense::map<GenericAtomIndex, QColor, GenericAtomIndexHash>
      m_atomColorOverrides;
  std::vector<QColor> m_atomColors;
  QString m_name{"structure"};

  // these are here for optimisation purposes, they need to be reconstructed if
  // the atoms are changed anyway.
  occ::core::graph::BondGraph m_bondGraph;
  std::vector<occ::core::graph::BondGraph::VertexDescriptor>
      m_bondGraphVertices;
  std::vector<occ::core::graph::BondGraph::EdgeDescriptor> m_bondGraphEdges;

  bool m_bondsNeedUpdate{true};

  QString m_filename;
  QByteArray m_fileContents;

  PairInteractions *m_interactions{nullptr};
  ObjectTreeModel *m_treeModel{nullptr};
};
