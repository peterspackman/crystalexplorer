#include "slabstructure.h"
#include "crystalstructure.h"
#include <QDebug>
#include <occ/crystal/crystal.h>
#include <fmt/core.h>

SlabStructure::SlabStructure(QObject *parent) : ChemicalStructure(parent) {
  // Initialize default surface vectors (will be updated when slab is created)
  m_surfaceVectors = occ::Mat3::Identity();
}

void SlabStructure::buildFromCrystal(const CrystalStructure &crystal,
                                    const CrystalSurfaceCutOptions &options) {
  qDebug() << "Building surface cut from crystal with Miller plane" 
           << options.millerPlane.h << options.millerPlane.k << options.millerPlane.l;
  
  // Store parameters for potential regeneration
  m_parentCrystal = crystal.occCrystal();
  m_lastOptions = options;
  m_millerPlane = options.millerPlane;
  m_slabThickness = options.thickness;
  m_cutOffset = options.cutOffset;
  m_termination = options.termination;
  
  // Calculate surface vectors and setup surface
  calculateSurfaceVectors(crystal.occCrystal());
  
  // Create Surface object for molecule-preserving slab generation
  OccSurface surface(m_millerPlane, m_parentCrystal);
  
  // Get unit cell molecules for molecular slab generation
  const auto &unitCellMols = m_parentCrystal.unit_cell_molecules();
  
  // Calculate appropriate depth based on thickness
  double depthScale = 1.0;
  if (options.thickness > 0) {
    depthScale = std::max(1.0, options.thickness / surface.depth());
  }
  
  // Find molecules in the slab using the Surface class
  std::vector<occ::core::Molecule> slabMolecules;
  if (options.preserveMolecules) {
    slabMolecules = surface.find_molecule_cell_translations(unitCellMols, depthScale, options.cutOffset);
  } else {
    // TODO: implement atom-based cutting if needed
    qWarning() << "Atom-based cutting not yet implemented, using molecule preservation";
    slabMolecules = surface.find_molecule_cell_translations(unitCellMols, depthScale, options.cutOffset);
  }
  
  qDebug() << "Found" << slabMolecules.size() << "molecules in surface cut";
  
  // Convert molecules to atomic data and set up fragments
  clearAtoms();
  std::vector<QString> elementSymbols;
  std::vector<occ::Vec3> positions;
  std::vector<QString> labels;
  
  m_fragments.clear();
  m_fragmentForAtom.clear();
  m_slabAtomIndices.clear();
  m_slabAtomMap.clear();
  
  int atomIndex = 0;
  int fragmentIndex = 0;
  
  for (const auto &mol : slabMolecules) {
    const auto &molPositions = mol.positions();
    const auto &elements = mol.elements();
    
    // Create fragment for this molecule
    Fragment frag;
    frag.index = FragmentIndex{fragmentIndex};
    frag.atomicNumbers.resize(elements.size());
    frag.positions.resize(3, elements.size());
    
    std::vector<GenericAtomIndex> moleculeAtomIndices;
    
    for (int i = 0; i < elements.size(); i++) {
      elementSymbols.push_back(QString::fromStdString(elements[i].symbol()));
      positions.push_back(molPositions.col(i));
      labels.push_back(QString("M%1A%2").arg(mol.unit_cell_molecule_idx()).arg(i));
      
      // Set up fragment data
      frag.atomicNumbers(i) = elements[i].atomic_number();
      frag.positions.col(i) = molPositions.col(i);
      
      // Create atom index (simplified - using sequential index for slabs)
      GenericAtomIndex atomIdx{atomIndex, 0, 0, 0};
      frag.atomIndices.push_back(atomIdx);
      moleculeAtomIndices.push_back(atomIdx);
      
      qDebug() << "Adding atom" << atomIndex << "with symbol" 
               << QString::fromStdString(elements[i].symbol())
               << "to fragment" << fragmentIndex;
      
      atomIndex++;
    }
    
    // Store the fragment
    m_fragments[frag.index] = frag;
    fragmentIndex++;
  }
  
  // Mirror CrystalStructure approach: set up indexing then call setAtoms
  m_slabAtomIndices.clear();
  m_slabAtomMap.clear();
  
  // Use the same indexing as was set up in fragments
  int baseAtomIndex = 0;
  for (const auto &mol : slabMolecules) {
    for (int i = 0; i < mol.size(); i++) {
      GenericAtomIndex idx{baseAtomIndex, 0, 0, 0}; // Base atoms have no periodic offset
      m_slabAtomIndices.push_back(idx);
      m_slabAtomMap[idx] = baseAtomIndex;
      baseAtomIndex++;
    }
  }
  
  // Set atoms (this is like CrystalStructure pattern)
  setAtoms(elementSymbols, positions, labels);
  
  // Set up atom-to-fragment mapping
  fragmentIndex = 0;
  atomIndex = 0;
  
  for (const auto &mol : slabMolecules) {
    for (int i = 0; i < mol.size(); i++) {
      if (atomIndex < m_fragmentForAtom.size()) {
        m_fragmentForAtom[atomIndex] = FragmentIndex{fragmentIndex};
      }
      atomIndex++;
    }
    fragmentIndex++;
  }
  
  // Set up bonds within molecules
  updateBondGraph();
  
  // Validate indexing consistency
  if (m_slabAtomIndices.size() != numberOfAtoms()) {
    qWarning() << "Slab indexing size mismatch:" << m_slabAtomIndices.size() << "vs" << numberOfAtoms();
    m_slabAtomIndices.resize(numberOfAtoms());
  }
  
  qDebug() << "Surface cut created with" << numberOfAtoms() << "atoms and" 
           << m_slabAtomIndices.size() << "indices";
  emit atomsChanged();
}

occ::Mat3 SlabStructure::cellVectors() const {
  return m_surfaceVectors;
}

occ::Vec3 SlabStructure::cellAngles() const {
  // Calculate angles from the surface vectors
  // For a slab, we typically have 90° in the vacuum direction
  occ::Vec3 a = m_surfaceVectors.col(0);
  occ::Vec3 b = m_surfaceVectors.col(1);
  occ::Vec3 c = m_surfaceVectors.col(2);
  
  double alpha = std::acos(b.dot(c) / (b.norm() * c.norm())) * 180.0 / M_PI;
  double beta = std::acos(a.dot(c) / (a.norm() * c.norm())) * 180.0 / M_PI;
  double gamma = std::acos(a.dot(b) / (a.norm() * b.norm())) * 180.0 / M_PI;
  
  return occ::Vec3(alpha, beta, gamma);
}

occ::Vec3 SlabStructure::cellLengths() const {
  return occ::Vec3(
    m_surfaceVectors.col(0).norm(),
    m_surfaceVectors.col(1).norm(),
    m_surfaceVectors.col(2).norm()
  );
}

occ::Mat3N SlabStructure::convertCoordinates(const occ::Mat3N &pos,
                                            CoordinateConversion conversion) const {
  // Convert between fractional and Cartesian coordinates for 2D slab
  switch (conversion) {
  case CoordinateConversion::FracToCart:
    return m_surfaceVectors * pos;
  case CoordinateConversion::CartToFrac:
    return m_surfaceVectors.inverse() * pos;
  }
  return pos;
}

FragmentIndex SlabStructure::fragmentIndexForGeneralAtom(GenericAtomIndex idx) const {
  // For slabs, use the atom map to find the actual atom index, then get its fragment
  auto it = m_slabAtomMap.find(idx);
  if (it != m_slabAtomMap.end()) {
    int atomIndex = it->second;
    if (atomIndex < static_cast<int>(m_fragmentForAtom.size())) {
      return m_fragmentForAtom[atomIndex];
    }
  }
  return FragmentIndex{-1};
}

int SlabStructure::genericIndexToIndex(const GenericAtomIndex &idx) const {
  auto it = m_slabAtomMap.find(idx);
  return (it == m_slabAtomMap.end()) ? -1 : it->second;
}

GenericAtomIndex SlabStructure::indexToGenericIndex(int index) const {
  if (index >= 0 && index < static_cast<int>(m_slabAtomIndices.size())) {
    return m_slabAtomIndices[index];
  }
  return GenericAtomIndex{-1, 0, 0, 0};
}

void SlabStructure::setSlabThickness(double thickness) {
  if (thickness != m_slabThickness) {
    m_slabThickness = thickness;
    // TODO: Trigger regeneration if needed
  }
}

void SlabStructure::setCutOffset(double offset) {
  if (offset != m_cutOffset) {
    m_cutOffset = offset;
    // TODO: Trigger regeneration if needed
  }
}

void SlabStructure::setMillerPlane(const occ::crystal::HKL &hkl) {
  m_millerPlane = hkl;
}

void SlabStructure::setTermination(const QString &termination) {
  m_termination = termination;
}


nlohmann::json SlabStructure::toJson() const {
  auto json = ChemicalStructure::toJson();
  
  json["structure_type"] = "surface_cut";
  json["slab_thickness"] = m_slabThickness;
  json["cut_offset"] = m_cutOffset;
  json["miller_plane"] = {m_millerPlane.h, m_millerPlane.k, m_millerPlane.l};
  json["termination"] = m_termination.toStdString();
  
  // Store surface vectors
  json["surface_vectors"] = {
    {m_surfaceVectors(0,0), m_surfaceVectors(1,0), m_surfaceVectors(2,0)},
    {m_surfaceVectors(0,1), m_surfaceVectors(1,1), m_surfaceVectors(2,1)},
    {m_surfaceVectors(0,2), m_surfaceVectors(1,2), m_surfaceVectors(2,2)}
  };
  
  return json;
}

bool SlabStructure::fromJson(const nlohmann::json &json) {
  if (!ChemicalStructure::fromJson(json)) {
    return false;
  }
  
  try {
    if (json.contains("slab_thickness")) {
      m_slabThickness = json["slab_thickness"];
    }
    if (json.contains("cut_offset")) {
      m_cutOffset = json["cut_offset"];
    }
    if (json.contains("miller_plane") && json["miller_plane"].is_array() && 
        json["miller_plane"].size() == 3) {
      auto hkl = json["miller_plane"];
      m_millerPlane = occ::crystal::HKL(hkl[0], hkl[1], hkl[2]);
    }
    if (json.contains("termination")) {
      m_termination = QString::fromStdString(json["termination"]);
    }
    
    // Load surface vectors if available
    if (json.contains("surface_vectors") && json["surface_vectors"].is_array() &&
        json["surface_vectors"].size() == 3) {
      auto vectors = json["surface_vectors"];
      for (int i = 0; i < 3; ++i) {
        if (vectors[i].is_array() && vectors[i].size() == 3) {
          m_surfaceVectors(0, i) = vectors[i][0];
          m_surfaceVectors(1, i) = vectors[i][1];
          m_surfaceVectors(2, i) = vectors[i][2];
        }
      }
    }
    
    return true;
  } catch (const std::exception &e) {
    qWarning() << "Error loading SlabStructure from JSON:" << e.what();
    return false;
  }
}


void SlabStructure::calculateSurfaceVectors(const OccCrystal &crystal) {
  // Use Surface class to calculate proper surface vectors
  OccSurface surface(m_millerPlane, crystal);
  
  // Get the surface basis matrix (just the 2D surface vectors)
  occ::Mat3 surfaceBasis = surface.basis_matrix(1.0);
  
  // Set the surface vectors using the calculated basis
  m_surfaceVectors.col(0) = surfaceBasis.col(0);  // a vector (in surface)
  m_surfaceVectors.col(1) = surfaceBasis.col(1);  // b vector (in surface)
  m_surfaceVectors.col(2) = surfaceBasis.col(2);  // depth vector (normal to surface)
  
  qDebug() << "Surface vectors calculated for Miller plane" 
           << m_millerPlane.h << m_millerPlane.k << m_millerPlane.l;
  qDebug() << "a:" << m_surfaceVectors(0,0) << m_surfaceVectors(1,0) << m_surfaceVectors(2,0);
  qDebug() << "b:" << m_surfaceVectors(0,1) << m_surfaceVectors(1,1) << m_surfaceVectors(2,1);
  qDebug() << "c:" << m_surfaceVectors(0,2) << m_surfaceVectors(1,2) << m_surfaceVectors(2,2);
}

// Override methods from ChemicalStructure adapted for 2D periodicity

void SlabStructure::expandAtomsWithinRadius(float radius, bool selected) {
  // For slabs, we expand in the 2D surface directions (a,b), not in the vacuum direction (c)
  std::vector<GenericAtomIndex> selectedAtoms;
  if (selected) {
    resetAtomsAndBonds(true);
    selectedAtoms = atomsWithFlags(AtomFlag::Selected);
    if (std::abs(radius) < 1e-3)
      return;
  }

  // Find atoms to add within radius using 2D periodicity
  std::vector<GenericAtomIndex> centerAtoms;
  if (selected) {
    centerAtoms = selectedAtoms;
  } else {
    // Use all existing atoms as centers
    for (int i = 0; i < numberOfAtoms(); i++) {
      centerAtoms.push_back(indexToGenericIndex(i));
    }
  }
  
  std::vector<GenericAtomIndex> surroundingAtoms = atomsSurroundingAtoms(centerAtoms, radius);
  
  // Filter to only add new atoms (not already present)
  std::vector<GenericAtomIndex> atomsToAdd;
  for (const auto& idx : surroundingAtoms) {
    if (genericIndexToIndex(idx) == -1) { // Not already present
      atomsToAdd.push_back(idx);
    }
  }
  
  if (!atomsToAdd.empty()) {
    addSlabAtoms(atomsToAdd, AtomFlag::NoFlag);
    qDebug() << "Expanded slab with" << atomsToAdd.size() << "atoms within radius" << radius;
    emit atomsChanged();
  }
  
  updateBondGraph();
  
  // Restore selection
  setFlagForAtoms(selectedAtoms, AtomFlag::Selected);
}

void SlabStructure::completeFragmentContaining(GenericAtomIndex index) {
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  
  FragmentIndex fragmentIndex = fragmentIndexForGeneralAtom(index);
  const auto& frag = getFragment(fragmentIndex);
  if (!frag) return;
  
  qDebug() << "Completing fragment containing atom index:" 
           << index.unique << index.x << index.y << index.z
           << "Fragment index:" << fragmentIndex.u << fragmentIndex.h << fragmentIndex.k << fragmentIndex.l;
  
  // For slabs, fragments are molecular and should already be complete
  // Just add contact atoms if needed
  if (haveContactAtoms) {
    addSlabContactAtoms();
  }
  updateBondGraph();
}

void SlabStructure::completeFragmentContaining(int atomIndex) {
  if (atomIndex < 0 || atomIndex >= numberOfAtoms())
    return;
  GenericAtomIndex idx = indexToGenericIndex(atomIndex);
  completeFragmentContaining(idx);
}

void SlabStructure::setShowContacts(const ContactSettings &settings) {
  m_contactSettings = settings;
  if (settings.show) {
    addSlabContactAtoms();
    updateBondGraph();
    // addSlabContactAtoms emits atomsChanged() if atoms are added
  } else {
    removeSlabContactAtoms();
    updateBondGraph();
    // removeSlabContactAtoms emits atomsChanged() if atoms are removed
  }
}

CellIndexSet SlabStructure::occupiedCells() const {
  CellIndexSet result;
  
  // Convert positions to fractional coordinates using slab basis
  occ::Mat3N pos_frac = m_surfaceVectors.inverse() * atomicPositions();
  
  auto conv = [](double x) { return static_cast<int>(std::floor(x)); };
  
  for (int i = 0; i < pos_frac.cols(); i++) {
    // Only consider surface cell indices (a,b), not vacuum direction (c)
    CellIndex idx{conv(pos_frac(0, i)), conv(pos_frac(1, i)), 0};
    result.insert(idx);
  }
  
  return result;
}

std::vector<GenericAtomIndex> 
SlabStructure::atomsSurroundingAtoms(const std::vector<GenericAtomIndex> &idxs, float radius) const {
  using GenericAtomIndexSet = ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>;
  GenericAtomIndexSet surrounding;
  
  for (const auto &centerIdx : idxs) {
    occ::Vec3 centerPos = atomPosition(centerIdx);
    
    // Search in 2D periodic neighbors only (surface directions)
    for (int h = -2; h <= 2; h++) {
      for (int k = -2; k <= 2; k++) {
        occ::Vec3 cellShift = h * m_surfaceVectors.col(0) + k * m_surfaceVectors.col(1);
        
        for (int i = 0; i < numberOfAtoms(); i++) {
          GenericAtomIndex testIdx = indexToGenericIndex(i);
          occ::Vec3 testPos = atomPosition(testIdx) + cellShift;
          
          double distance = (centerPos - testPos).norm();
          if (distance <= radius && distance > 1e-6) { // Avoid self-inclusion
            // Create periodic image index
            GenericAtomIndex periodicIdx{testIdx.unique, testIdx.x + h, testIdx.y + k, testIdx.z};
            surrounding.insert(periodicIdx);
          }
        }
      }
    }
  }
  
  return std::vector<GenericAtomIndex>(surrounding.begin(), surrounding.end());
}

std::vector<GenericAtomIndex> 
SlabStructure::atomsSurroundingAtomsWithFlags(const AtomFlags &flags, float radius) const {
  std::vector<GenericAtomIndex> flaggedAtoms = atomsWithFlags(flags);
  return atomsSurroundingAtoms(flaggedAtoms, radius);
}

void SlabStructure::buildSlab(SlabGenerationOptions options) {
  // For SlabStructure, this would regenerate the slab with new thickness/parameters
  // For now, just log that this isn't the primary way to build slabs
  qDebug() << "SlabStructure::buildSlab called - use buildFromCrystal instead";
}

// Helper methods for slab-specific atom management

void SlabStructure::addSlabAtoms(const std::vector<GenericAtomIndex> &unfilteredIndices, const AtomFlags &flags) {
  // Mirror CrystalStructure::addAtomsByCrystalIndex exactly but for 2D periodicity
  
  qDebug() << "addSlabAtoms called with" << unfilteredIndices.size() << "indices";
  for (const auto& idx : unfilteredIndices) {
    qDebug() << "  Request to add:" << idx.unique << idx.x << idx.y << idx.z;
  }
  
  // Filter out already existing indices
  std::vector<GenericAtomIndex> indices;
  indices.reserve(unfilteredIndices.size());
  setFlagForAtoms(unfilteredIndices, AtomFlag::Contact, false);

  std::copy_if(unfilteredIndices.begin(), unfilteredIndices.end(),
               std::back_inserter(indices), [&](const GenericAtomIndex &index) {
                 return m_slabAtomMap.find(index) == m_slabAtomMap.end();
               });
               
  qDebug() << "After filtering, have" << indices.size() << "indices to add";

  occ::IVec nums(indices.size());
  occ::Mat3N pos(3, indices.size());
  std::vector<QString> l;
  const int numAtomsBefore = numberOfAtoms();

  {
    int i = 0;
    for (const auto &idx : indices) {
      // Find base atom data by looking for the base unique index (without periodic offsets)
      GenericAtomIndex baseIdx{idx.unique, 0, 0, 0};
      int baseAtomIndex = genericIndexToIndex(baseIdx);
      
      if (baseAtomIndex == -1) {
        qDebug() << "Warning: Could not find base atom for index" << idx.unique;
        qDebug() << "Available atoms:";
        for (int j = 0; j < numberOfAtoms(); j++) {
          GenericAtomIndex existing = indexToGenericIndex(j);
          qDebug() << "  Atom" << j << ":" << existing.unique << existing.x << existing.y << existing.z;
        }
        continue;
      }
      
      occ::Vec3 basePos = atomPosition(baseIdx);
      int baseAtomicNum = atomicNumbers()(baseAtomIndex);
      QString baseLabel;
      if (baseAtomIndex < labels().size()) {
        baseLabel = labels()[baseAtomIndex];
      }
      
      nums(i) = baseAtomicNum;
      // Calculate position using 2D slab periodicity (only x,y shifts, not z)
      occ::Vec3 shiftVec = idx.x * m_surfaceVectors.col(0) + idx.y * m_surfaceVectors.col(1);
      pos.col(i) = basePos + shiftVec;
      
      QString label = QString("S%1_%2_%3").arg(idx.unique).arg(idx.x).arg(idx.y);
      l.push_back(label);
      m_slabAtomIndices.push_back(idx);
      m_slabAtomMap.insert({idx, numAtomsBefore + i});
      i++;
    }
  }

  std::vector<occ::Vec3> positionsToAdd;
  std::vector<QString> elementSymbols;
  
  for (int i = 0; i < pos.cols(); i++) {
    positionsToAdd.push_back(pos.col(i));
    elementSymbols.push_back(QString::fromStdString(occ::core::Element(nums(i)).symbol()));
  }

  addAtoms(elementSymbols, positionsToAdd, l);
  
  // Set flags for new atoms
  for (int i = 0; i < indices.size(); i++) {
    setAtomFlags(indices[i], flags);
  }
  
  qDebug() << "Added" << indices.size() << "slab atoms";
  emit atomsChanged();
}

void SlabStructure::addSlabContactAtoms() {
  // Add VdW contact atoms considering 2D periodicity
  using GenericAtomIndexSet = ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>;
  GenericAtomIndexSet contactAtomsToAdd;
  
  const double maxContactDistance = 4.0; // Reasonable VdW contact distance in Angstroms
  
  // For each existing atom, find potential contact atoms in neighboring 2D cells
  for (int i = 0; i < numberOfAtoms(); i++) {
    GenericAtomIndex sourceIdx = indexToGenericIndex(i);
    
    // Skip if this atom is already a contact atom
    if (testAtomFlag(sourceIdx, AtomFlag::Contact)) continue;
    
    occ::Vec3 sourcePos = atomPosition(sourceIdx);
    double sourceVdwRadius = vdwRadii()(i);
    
    // Check neighboring surface cells for potential contacts
    for (int h = -1; h <= 1; h++) {
      for (int k = -1; k <= 1; k++) {
        if (h == 0 && k == 0) continue; // Skip same cell
        
        occ::Vec3 cellShift = h * m_surfaceVectors.col(0) + k * m_surfaceVectors.col(1);
        
        // Check all existing atoms in this shifted cell
        for (int j = 0; j < numberOfAtoms(); j++) {
          GenericAtomIndex targetIdx = indexToGenericIndex(j);
          if (testAtomFlag(targetIdx, AtomFlag::Contact)) continue;
          
          occ::Vec3 targetPos = atomPosition(targetIdx) + cellShift;
          double targetVdwRadius = vdwRadii()(j);
          
          double distance = (sourcePos - targetPos).norm();
          double contactDistance = sourceVdwRadius + targetVdwRadius;
          
          // Check if within contact distance
          if (distance <= contactDistance && distance <= maxContactDistance) {
            // Create contact atom at shifted position
            GenericAtomIndex contactIdx{targetIdx.unique, targetIdx.x + h, targetIdx.y + k, targetIdx.z};
            contactAtomsToAdd.insert(contactIdx);
          }
        }
      }
    }
  }
  
  // Add the contact atoms
  if (!contactAtomsToAdd.empty()) {
    std::vector<GenericAtomIndex> contactAtoms(contactAtomsToAdd.begin(), contactAtomsToAdd.end());
    addSlabAtoms(contactAtoms, AtomFlag::Contact);
    
    qDebug() << "Added" << contactAtoms.size() << "contact atoms to slab";
    // Note: addSlabAtoms already emits atomsChanged()
  }
}

void SlabStructure::removeSlabContactAtoms() {
  // Remove atoms marked as contacts - mirror CrystalStructure approach
  std::vector<int> indicesToRemove;
  
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (i < m_slabAtomIndices.size() && testAtomFlag(m_slabAtomIndices[i], AtomFlag::Contact)) {
      indicesToRemove.push_back(i);
    }
  }
  
  if (!indicesToRemove.empty()) {
    const auto selectedAtoms = atomsWithFlags(AtomFlag::Selected);
    deleteSlabAtomsByOffset(indicesToRemove);
    
    // Ensure selection doesn't change
    for (const auto &idx : selectedAtoms) {
      setAtomFlag(idx, AtomFlag::Selected);
    }
    
    emit atomsChanged();
  }
}

void SlabStructure::deleteSlabAtomsByOffset(const std::vector<int> &atomIndices) {
  // Mirror CrystalStructure::deleteAtomsByOffset exactly
  const int originalNumAtoms = numberOfAtoms();

  ankerl::unordered_dense::set<int> uniqueIndices;
  for (int idx : std::as_const(atomIndices)) {
    if (idx < originalNumAtoms)
      uniqueIndices.insert(idx);
  }

  std::vector<QString> newElementSymbols;
  std::vector<occ::Vec3> newPositions;
  std::vector<QString> newLabels;
  std::vector<GenericAtomIndex> slabAtomIndices;
  m_slabAtomMap.clear();
  
  const auto &currentPositions = atomicPositions();
  const auto &currentLabels = labels();
  const auto &currentNumbers = atomicNumbers();

  int atomIndex = 0;
  for (int i = 0; i < originalNumAtoms; i++) {
    if (uniqueIndices.contains(i))
      continue;
    
    if (i < m_slabAtomIndices.size()) {
      slabAtomIndices.push_back(m_slabAtomIndices[i]);
      m_slabAtomMap.insert({m_slabAtomIndices[i], atomIndex});
    }
    
    newPositions.push_back(currentPositions.col(i));
    newElementSymbols.push_back(QString::fromStdString(occ::core::Element(currentNumbers(i)).symbol()));
    if (i < currentLabels.size()) {
      newLabels.push_back(currentLabels[i]);
    }
    atomIndex++;
  }
  
  m_slabAtomIndices = slabAtomIndices;
  setAtoms(newElementSymbols, newPositions, newLabels);
}

Fragment SlabStructure::makeSlabFragmentFromFragmentIndex(FragmentIndex idx) const {
  // Adapt CrystalStructure::makeFragmentFromFragmentIndex for 2D periodicity
  FragmentIndex baseIndex{idx.u, 0, 0, 0};
  
  auto it = m_fragments.find(baseIndex);
  if (it == m_fragments.end()) {
    qWarning() << "Fragment not found for index" << idx.u;
    return Fragment{}; // Return empty fragment
  }
  
  Fragment result = it->second;
  
  // Apply 2D periodic shift (only x,y - no z shift for slabs)
  for (auto &atomIndex : result.atomIndices) {
    atomIndex.x += idx.h;
    atomIndex.y += idx.k;
    // Note: No z shift for slab - atomIndex.z stays unchanged
  }

  result.positions = atomicPositionsForIndices(result.atomIndices);
  result.index = idx;
  
  // Apply 2D translation using surface vectors (not 3D)
  occ::Vec3 translation_cart = idx.h * m_surfaceVectors.col(0) + idx.k * m_surfaceVectors.col(1);
  Eigen::Translation<double, 3> t(translation_cart);
  result.asymmetricFragmentTransform *= t;
  
  return result;
}

bool SlabStructure::hasIncompleteFragments() const {
  // For slab structures, check if any 2D periodic neighbors are missing
  // Since slabs are generated from complete molecules, incompleteness would come
  // from missing periodic images due to radius expansion
  
  // Simple approach: assume fragments are complete for slabs since they come from
  // complete molecules. Any incompleteness would be at the slab edges.
  // This could be enhanced to check bond connectivity across 2D periodic boundaries.
  
  return false; // For now, assume slab fragments are always complete
}

bool SlabStructure::hasIncompleteSelectedFragments() const {
  // Check if any selected atoms have incomplete fragments
  auto selectedAtoms = atomsWithFlags(AtomFlag::Selected);
  
  for (const auto& atomIdx : selectedAtoms) {
    FragmentIndex fragIdx = fragmentIndexForGeneralAtom(atomIdx);
    // Could check if this fragment has all its 2D periodic neighbors
    // For now, assume selected fragments are complete
  }
  
  return false;
}

void SlabStructure::completeAllFragments() {
  // For slabs, fragments come from complete molecules, so they should already be complete
  // This method mainly needs to handle contact atoms and bonding
  bool haveContactAtoms = anyAtomHasFlags(AtomFlag::Contact);
  const auto selectedAtoms = atomsWithFlags(AtomFlag::Selected);

  qDebug() << "Completing all fragments for slab with" << m_fragments.size() << "fragments";

  // For slabs, we don't need to add periodic neighbors of fragments
  // since the Surface class already provides complete molecules
  
  if (haveContactAtoms) {
    addSlabContactAtoms();
  }

  // Restore selection
  for (const auto &idx : selectedAtoms) {
    setAtomFlag(idx, AtomFlag::Selected);
  }

  updateBondGraph();
}