#include "slabstructure.h"
#include "crystalstructure.h"
#include <QDebug>
#include <occ/crystal/crystal.h>
#include <fmt/core.h>
#include <queue>

SlabStructure::SlabStructure(QObject *parent) : PeriodicStructure(parent) {
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
    qWarning() << "Atom-based cutting not yet implemented, using molecule preservation";
    slabMolecules = surface.find_molecule_cell_translations(unitCellMols, depthScale, options.cutOffset);
  }
  
  qDebug() << "Found" << slabMolecules.size() << "molecules in surface cut";
  
  // Convert molecules to atomic data and set up the base structure
  clearAtoms();
  std::vector<QString> elementSymbols;
  std::vector<occ::Vec3> positions;
  std::vector<QString> labels;
  
  // Clear base class data
  m_periodicAtomOffsets.clear();
  m_periodicAtomMap.clear();
  m_fragments.clear();
  m_fragmentForAtom.clear();
  
  // Store base slab data
  m_baseSlabPositions.resize(3, 0);
  m_baseSlabNumbers.resize(0);
  m_baseSlabLabels.clear();
  
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
    frag.atomIndices.clear();
    
    for (int i = 0; i < elements.size(); i++) {
      elementSymbols.push_back(QString::fromStdString(elements[i].symbol()));
      positions.push_back(molPositions.col(i));
      labels.push_back(QString("M%1A%2").arg(mol.unit_cell_molecule_idx()).arg(i));
      
      // Set up fragment data
      frag.atomicNumbers(i) = elements[i].atomic_number();
      frag.positions.col(i) = molPositions.col(i);
      
      // Create atom index (use sequential unique ID, no periodic offsets for base atoms)
      GenericAtomIndex atomIdx{atomIndex, 0, 0, 0};
      frag.atomIndices.push_back(atomIdx);
      
      // Store in base class data structures
      m_periodicAtomOffsets.push_back(atomIdx);
      m_periodicAtomMap[atomIdx] = atomIndex;
      
      atomIndex++;
    }
    
    // Store the fragment
    m_fragments[frag.index] = frag;
    fragmentIndex++;
  }
  
  // Set atoms (this will trigger ChemicalStructure setup)
  setAtoms(elementSymbols, positions, labels);
  
  // Store base slab data for periodic expansion
  m_baseSlabPositions = atomicPositions();
  m_baseSlabNumbers = atomicNumbers();
  m_baseSlabLabels = labels;
  
  // Set up atom-to-fragment mapping
  fragmentIndex = 0;
  atomIndex = 0;
  m_fragmentForAtom.resize(numberOfAtoms());
  
  for (const auto &mol : slabMolecules) {
    for (int i = 0; i < mol.size(); i++) {
      if (atomIndex < m_fragmentForAtom.size()) {
        m_fragmentForAtom[atomIndex] = FragmentIndex{fragmentIndex};
      }
      atomIndex++;
    }
    fragmentIndex++;
  }
  
  // Build the connectivity graph for this slab
  buildSlabConnectivity();
  
  // Update bond graph using base class method
  updateBondGraph();
  
  // Re-assign atoms to their molecular fragments after bond graph update
  // The base updateBondGraph() might reassign based on bond connectivity,
  // but we want to preserve the molecular fragment assignments
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
  
  qDebug() << "Surface cut created with" << numberOfAtoms() << "atoms and" 
           << m_periodicAtomOffsets.size() << "indices in" << m_fragments.size() << "fragments";
  emit atomsChanged();
}

occ::Mat3 SlabStructure::cellVectors() const {
  return m_surfaceVectors;
}

occ::Vec3 SlabStructure::cellAngles() const {
  // Calculate angles from the surface vectors
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
  switch (conversion) {
  case CoordinateConversion::FracToCart:
    return m_surfaceVectors * pos;
  case CoordinateConversion::CartToFrac:
    return m_surfaceVectors.inverse() * pos;
  }
  return pos;
}

FragmentIndex SlabStructure::fragmentIndexForGeneralAtom(GenericAtomIndex idx) const {
  // Use base class implementation but check our specific atom map
  auto it = m_periodicAtomMap.find(idx);
  if (it != m_periodicAtomMap.end()) {
    int atomIndex = it->second;
    if (atomIndex < static_cast<int>(m_fragmentForAtom.size())) {
      return m_fragmentForAtom[atomIndex];
    }
  }
  return FragmentIndex{-1};
}

void SlabStructure::buildSlab(SlabGenerationOptions options) {
  qDebug() << "SlabStructure::buildSlab called - use buildFromCrystal instead";
}

CellIndexSet SlabStructure::occupiedCells() const {
  CellIndexSet result;
  
  // Convert positions to fractional coordinates using slab basis
  occ::Mat3N pos_frac = m_surfaceVectors.inverse() * atomicPositions();
  
  auto conv = [](double x) { return static_cast<int>(std::floor(x)); };
  
  for (int i = 0; i < pos_frac.cols(); i++) {
    // Only consider surface cell indices (a,b), set z to 0 for slabs
    CellIndex idx{conv(pos_frac(0, i)), conv(pos_frac(1, i)), 0};
    result.insert(idx);
  }
  
  return result;
}

// Slab property setters
void SlabStructure::setSlabThickness(double thickness) {
  if (thickness != m_slabThickness) {
    m_slabThickness = thickness;
  }
}

void SlabStructure::setCutOffset(double offset) {
  if (offset != m_cutOffset) {
    m_cutOffset = offset;
  }
}

void SlabStructure::setMillerPlane(const occ::crystal::HKL &hkl) {
  m_millerPlane = hkl;
}

void SlabStructure::setTermination(const QString &termination) {
  m_termination = termination;
}

// Serialization
nlohmann::json SlabStructure::toJson() const {
  auto json = ChemicalStructure::toJson();
  
  json["structure_type"] = "surface_cut";
  json["slab_thickness"] = m_slabThickness;
  json["cut_offset"] = m_cutOffset;
  json["miller_plane"] = {m_millerPlane.h, m_millerPlane.k, m_millerPlane.l};
  json["termination"] = m_termination.toStdString();
  json["atomIndices"] = m_periodicAtomOffsets;
  
  // Store surface vectors
  json["surface_vectors"] = {
    {m_surfaceVectors(0,0), m_surfaceVectors(1,0), m_surfaceVectors(2,0)},
    {m_surfaceVectors(0,1), m_surfaceVectors(1,1), m_surfaceVectors(2,1)},
    {m_surfaceVectors(0,2), m_surfaceVectors(1,2), m_surfaceVectors(2,2)}
  };
  
  return json;
}

bool SlabStructure::fromJson(const nlohmann::json &json) {
  if (!fromJsonBase(json)) {
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
    
    if (json.contains("atomIndices")) {
      json.at("atomIndices").get_to(m_periodicAtomOffsets);
      // rebuild atom map
      m_periodicAtomMap.clear();
      for (int i = 0; i < m_periodicAtomOffsets.size(); i++) {
        m_periodicAtomMap.insert({m_periodicAtomOffsets[i], i});
      }
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

// PeriodicStructure virtual method implementations for 2D slabs

void SlabStructure::addPeriodicAtoms(const std::vector<GenericAtomIndex> &unfilteredIndices,
                                    const AtomFlags &flags) {
  qDebug() << "SlabStructure::addPeriodicAtoms called with" << unfilteredIndices.size() << "indices";
  
  // Filter out already existing indices
  std::vector<GenericAtomIndex> indices;
  indices.reserve(unfilteredIndices.size());
  setFlagForAtoms(unfilteredIndices, AtomFlag::Contact, false);

  std::copy_if(unfilteredIndices.begin(), unfilteredIndices.end(),
               std::back_inserter(indices), [&](const GenericAtomIndex &index) {
                 return m_periodicAtomMap.find(index) == m_periodicAtomMap.end();
               });
               
  qDebug() << "After filtering, have" << indices.size() << "indices to add";

  const int numAtomsBefore = numberOfAtoms();
  
  std::vector<occ::Vec3> positionsToAdd;
  std::vector<QString> elementSymbols;
  std::vector<QString> labelsToAdd;

  for (const auto &idx : indices) {
    // Find base atom data by looking for the base unique index (without periodic offsets)
    GenericAtomIndex baseIdx{idx.unique, 0, 0, 0};
    int baseAtomIndex = genericIndexToIndex(baseIdx);
    
    if (baseAtomIndex == -1) {
      qDebug() << "Warning: Could not find base atom for index" << idx.unique;
      continue;
    }
    
    // Get base atom properties
    if (baseAtomIndex >= m_baseSlabPositions.cols() || 
        baseAtomIndex >= m_baseSlabNumbers.rows() ||
        baseAtomIndex >= m_baseSlabLabels.size()) {
      qDebug() << "Warning: Base atom index out of range:" << baseAtomIndex;
      continue;
    }
    
    occ::Vec3 basePos = m_baseSlabPositions.col(baseAtomIndex);
    int baseAtomicNum = m_baseSlabNumbers(baseAtomIndex);
    QString baseLabel = m_baseSlabLabels[baseAtomIndex];
    
    // Calculate position using 2D slab periodicity (only x,y shifts, not z)
    occ::Vec3 shiftVec = idx.x * m_surfaceVectors.col(0) + idx.y * m_surfaceVectors.col(1);
    occ::Vec3 newPos = basePos + shiftVec;
    
    positionsToAdd.push_back(newPos);
    elementSymbols.push_back(QString::fromStdString(occ::core::Element(baseAtomicNum).symbol()));
    
    QString label = QString("S%1_%2_%3").arg(idx.unique).arg(idx.x).arg(idx.y);
    labelsToAdd.push_back(label);
    
    // Update mapping
    m_periodicAtomOffsets.push_back(idx);
    m_periodicAtomMap.insert({idx, numAtomsBefore + positionsToAdd.size() - 1});
  }

  // Add atoms to the structure
  addAtoms(elementSymbols, positionsToAdd, labelsToAdd);
  
  // Set flags for new atoms
  for (int i = 0; i < indices.size(); i++) {
    setAtomFlags(indices[i], flags);
  }
  
  qDebug() << "Added" << indices.size() << "slab atoms";
  emit atomsChanged();
}

void SlabStructure::resetAtomsAndBonds(bool toSelection) {
  if (toSelection) {
    // Use base class implementation for selection reset
    PeriodicStructure::resetAtomsAndBonds(true);
    return;
  }
  
  qDebug() << "SlabStructure::resetAtomsAndBonds - resetting to initial slab molecules";
  
  // Clear current atoms
  clearAtoms();
  m_periodicAtomOffsets.clear();
  m_periodicAtomMap.clear();
  m_fragments.clear();
  m_fragmentForAtom.clear();
  
  // Rebuild from stored base slab data
  if (m_baseSlabPositions.cols() == 0 || m_baseSlabNumbers.rows() == 0) {
    qWarning() << "No base slab data available for reset";
    return;
  }
  
  std::vector<QString> elementSymbols;
  std::vector<occ::Vec3> positions;
  std::vector<QString> labels = m_baseSlabLabels;
  
  // Convert base slab data back to atom format
  for (int i = 0; i < m_baseSlabPositions.cols(); i++) {
    positions.push_back(m_baseSlabPositions.col(i));
    elementSymbols.push_back(QString::fromStdString(
        occ::core::Element(m_baseSlabNumbers(i)).symbol()));
    
    // Create base atom index (no periodic offset)
    GenericAtomIndex atomIdx{i, 0, 0, 0};
    m_periodicAtomOffsets.push_back(atomIdx);
    m_periodicAtomMap[atomIdx] = i;
  }
  
  // Set atoms and rebuild structure
  setAtoms(elementSymbols, positions, labels);
  
  // Rebuild connectivity and fragments
  buildSlabConnectivity();
  updateBondGraph();
  
  qDebug() << "Reset complete - now have" << numberOfAtoms() << "atoms in initial slab state";
  emit atomsChanged();
}

void SlabStructure::removePeriodicContactAtoms() {
  qDebug() << "SlabStructure::removePeriodicContactAtoms called";
  
  std::vector<int> indicesToRemove;
  
  for (int i = 0; i < numberOfAtoms(); i++) {
    if (i < m_periodicAtomOffsets.size() && testAtomFlag(m_periodicAtomOffsets[i], AtomFlag::Contact)) {
      indicesToRemove.push_back(i);
    }
  }
  
  if (!indicesToRemove.empty()) {
    const auto selectedAtoms = atomsWithFlags(AtomFlag::Selected);
    deleteAtomsByOffset(indicesToRemove);
    
    // Ensure selection doesn't change
    for (const auto &idx : selectedAtoms) {
      setAtomFlag(idx, AtomFlag::Selected);
    }
    
    qDebug() << "Removed" << indicesToRemove.size() << "contact atoms from slab";
    emit atomsChanged();
  }
}

void SlabStructure::deleteAtomsByOffset(const std::vector<int> &atomIndices) {
  qDebug() << "SlabStructure::deleteAtomsByOffset called with" << atomIndices.size() << "indices";
  
  const int originalNumAtoms = numberOfAtoms();

  ankerl::unordered_dense::set<int> uniqueIndices;
  for (int idx : std::as_const(atomIndices)) {
    if (idx < originalNumAtoms)
      uniqueIndices.insert(idx);
  }

  std::vector<QString> newElementSymbols;
  std::vector<occ::Vec3> newPositions;
  std::vector<QString> newLabels;
  std::vector<GenericAtomIndex> periodicAtomOffsets;
  m_periodicAtomMap.clear();
  
  const auto &currentPositions = atomicPositions();
  const auto &currentLabels = labels();
  const auto &currentNumbers = atomicNumbers();

  int atomIndex = 0;
  for (int i = 0; i < originalNumAtoms; i++) {
    if (uniqueIndices.contains(i))
      continue;
    
    if (i < m_periodicAtomOffsets.size()) {
      periodicAtomOffsets.push_back(m_periodicAtomOffsets[i]);
      m_periodicAtomMap.insert({m_periodicAtomOffsets[i], atomIndex});
    }
    
    newPositions.push_back(currentPositions.col(i));
    newElementSymbols.push_back(QString::fromStdString(occ::core::Element(currentNumbers(i)).symbol()));
    if (i < currentLabels.size()) {
      newLabels.push_back(currentLabels[i]);
    }
    atomIndex++;
  }
  
  m_periodicAtomOffsets = periodicAtomOffsets;
  setAtoms(newElementSymbols, newPositions, newLabels);
  
  qDebug() << "Deleted atoms, now have" << numberOfAtoms() << "atoms";
}


std::vector<GenericAtomIndex>
SlabStructure::findAtomsWithinRadius(const std::vector<GenericAtomIndex> &centerAtoms,
                                    float radius) const {
  using GenericAtomIndexSet = ankerl::unordered_dense::set<GenericAtomIndex, GenericAtomIndexHash>;
  GenericAtomIndexSet surrounding;
  
  for (const auto &centerIdx : centerAtoms) {
    auto location = m_periodicAtomMap.find(centerIdx);
    if (location == m_periodicAtomMap.end()) continue;
    
    int centerAtomIndex = location->second;
    if (centerAtomIndex >= numberOfAtoms()) continue;
    
    occ::Vec3 centerPos = atomicPositions().col(centerAtomIndex);
    
    // Calculate search range based on radius and cell vectors
    double cellALength = m_surfaceVectors.col(0).norm();
    double cellBLength = m_surfaceVectors.col(1).norm();
    int maxH = static_cast<int>(std::ceil(radius / cellALength)) + 1;
    int maxK = static_cast<int>(std::ceil(radius / cellBLength)) + 1;
    
    // Search in 2D periodic neighbors only (surface directions)
    for (int h = -maxH; h <= maxH; h++) {
      for (int k = -maxK; k <= maxK; k++) {
        occ::Vec3 cellShift = h * m_surfaceVectors.col(0) + k * m_surfaceVectors.col(1);
        
        // Check all base atoms (those with no periodic offset)
        for (int i = 0; i < numberOfAtoms(); i++) {
          GenericAtomIndex testIdx = indexToGenericIndex(i);
          
          // Only consider base atoms for expansion
          if (testIdx.x != 0 || testIdx.y != 0 || testIdx.z != 0) continue;
          
          occ::Vec3 testPos = atomicPositions().col(i) + cellShift;
          
          double distance = (centerPos - testPos).norm();
          if (distance <= radius && distance > 1e-6) { // Avoid self-inclusion
            // Create periodic image index (2D only, z stays 0)
            GenericAtomIndex periodicIdx{testIdx.unique, testIdx.x + h, testIdx.y + k, testIdx.z};
            surrounding.insert(periodicIdx);
          }
        }
      }
    }
  }
  
  return std::vector<GenericAtomIndex>(surrounding.begin(), surrounding.end());
}

// Private helper methods

void SlabStructure::calculateSurfaceVectors(const OccCrystal &crystal) {
  // Use Surface class to calculate proper surface vectors
  OccSurface surface(m_millerPlane, crystal);
  
  // Get the surface basis matrix (includes vacuum direction)
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

const occ::core::graph::PeriodicBondGraph& SlabStructure::getUnitCellConnectivity() const {
  return m_slabConnectivity;
}

void SlabStructure::buildSlabConnectivity() const {
  qDebug() << "Building slab connectivity graph";
  
  // Clear existing graph
  m_slabConnectivity = occ::core::graph::PeriodicBondGraph{};
  
  // Add vertices for each unique base atom
  ankerl::unordered_dense::map<int, typename occ::core::graph::PeriodicBondGraph::VertexDescriptor> vertexMap;
  
  for (int i = 0; i < numberOfAtoms(); i++) {
    GenericAtomIndex idx = indexToGenericIndex(i);
    if (idx.x == 0 && idx.y == 0 && idx.z == 0) { // Only base atoms
      if (vertexMap.find(idx.unique) == vertexMap.end()) {
        occ::core::graph::PeriodicVertex vertex;
        vertex.uc_idx = idx.unique;
        auto vDesc = m_slabConnectivity.add_vertex(vertex);
        vertexMap[idx.unique] = vDesc;
      }
    }
  }
  
  // Add edges based on distance (both covalent and contact), similar to UnitCellConnectivityBuilder
  const double covalentTolerance = 0.4;
  const double vdwTolerance = 0.6;
  
  for (int i = 0; i < numberOfAtoms(); i++) {
    GenericAtomIndex idxI = indexToGenericIndex(i);
    if (idxI.x != 0 || idxI.y != 0 || idxI.z != 0) continue; // Only base atoms
    if (testAtomFlag(idxI, AtomFlag::Contact)) continue;
    
    occ::Vec3 posI = atomicPositions().col(i);
    int atomicNumI = atomicNumbers()(i);
    occ::core::Element elemI(atomicNumI);
    double covalentRadiusI = elemI.covalent_radius();
    double vdwRadiusI = elemI.van_der_waals_radius();
    
    auto sourceVertex = vertexMap[idxI.unique];
    
    // Check all other base atoms and periodic images
    for (int j = i + 1; j < numberOfAtoms(); j++) { // j = i + 1 to avoid duplicates
      GenericAtomIndex idxJ = indexToGenericIndex(j);
      if (idxJ.x != 0 || idxJ.y != 0 || idxJ.z != 0) continue; // Only base atoms
      if (testAtomFlag(idxJ, AtomFlag::Contact)) continue;
      
      int atomicNumJ = atomicNumbers()(j);
      occ::core::Element elemJ(atomicNumJ);
      double covalentRadiusJ = elemJ.covalent_radius();
      double vdwRadiusJ = elemJ.van_der_waals_radius();
      
      // Calculate thresholds like UnitCellConnectivityBuilder
      double covalentThreshold = covalentRadiusI + covalentRadiusJ + covalentTolerance;
      double vdwThreshold = vdwRadiusI + vdwRadiusJ + vdwTolerance;
      
      auto targetVertex = vertexMap[idxJ.unique];
      occ::Vec3 posJ = atomicPositions().col(j);
      
      // Calculate search range based on VdW threshold (largest possible distance)
      double cellALength = m_surfaceVectors.col(0).norm();
      double cellBLength = m_surfaceVectors.col(1).norm();
      int maxH = static_cast<int>(std::ceil(vdwThreshold / cellALength)) + 1;
      int maxK = static_cast<int>(std::ceil(vdwThreshold / cellBLength)) + 1;
      
      // Check direct connection (h=0, k=0) and all periodic images in one loop
      for (int h = -maxH; h <= maxH; h++) {
        for (int k = -maxK; k <= maxK; k++) {
          occ::Vec3 cellShift = h * m_surfaceVectors.col(0) + k * m_surfaceVectors.col(1);
          occ::Vec3 periodicPosJ = posJ + cellShift;
          double distance = (posI - periodicPosJ).norm();
          
          // Classify connection type based on distance
          occ::core::graph::PeriodicEdge::Connection connectionType = occ::core::graph::PeriodicEdge::Connection::DontBond;
          
          if (distance < covalentThreshold && distance > 0.1) {
            connectionType = occ::core::graph::PeriodicEdge::Connection::CovalentBond;
          } else if (distance < vdwThreshold && distance > 2.0) { // Minimum 2.0 Å for contacts
            connectionType = occ::core::graph::PeriodicEdge::Connection::CloseContact;
          }
          
          if (connectionType != occ::core::graph::PeriodicEdge::Connection::DontBond) {
            // Add forward edge
            occ::core::graph::PeriodicEdge forwardEdge;
            forwardEdge.source = idxI.unique;
            forwardEdge.target = idxJ.unique;
            forwardEdge.h = h;
            forwardEdge.k = k;
            forwardEdge.l = 0; // 2D slab, no z periodicity
            forwardEdge.dist = distance;
            forwardEdge.connectionType = connectionType;
            m_slabConnectivity.add_edge(sourceVertex, targetVertex, forwardEdge);
            
            // Add reverse edge
            occ::core::graph::PeriodicEdge reverseEdge;
            reverseEdge.source = idxJ.unique;
            reverseEdge.target = idxI.unique;
            reverseEdge.h = -h;
            reverseEdge.k = -k;
            reverseEdge.l = 0;
            reverseEdge.dist = distance;
            reverseEdge.connectionType = connectionType;
            m_slabConnectivity.add_edge(targetVertex, sourceVertex, reverseEdge);
          }
        }
      }
    }
  }
  
  qDebug() << "Built slab connectivity with" << m_slabConnectivity.num_vertices() 
           << "vertices and" << m_slabConnectivity.num_edges() << "edges";
}

void SlabStructure::updateSlabFragments() {
  // Update the fragment mapping after changes
  // For slabs, fragments are typically complete molecules, so just update the mapping
  
  m_fragmentForAtom.clear();
  m_fragmentForAtom.resize(numberOfAtoms(), FragmentIndex{-1});
  
  // Map atoms to their fragments based on existing fragment data
  for (const auto &[fragIndex, frag] : m_fragments) {
    for (const auto &atomIdx : frag.atomIndices) {
      int atomIndex = genericIndexToIndex(atomIdx);
      if (atomIndex >= 0 && atomIndex < m_fragmentForAtom.size()) {
        m_fragmentForAtom[atomIndex] = fragIndex;
      }
    }
  }
}

Fragment SlabStructure::makeSlabFragmentFromFragmentIndex(FragmentIndex idx) const {
  // For slabs, create fragment by looking up the base fragment and applying 2D periodic shift
  FragmentIndex baseIndex{idx.u, 0, 0, 0};
  
  auto it = m_fragments.find(baseIndex);
  if (it == m_fragments.end()) {
    qWarning() << "Fragment not found for index" << idx.u;
    return Fragment{};
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