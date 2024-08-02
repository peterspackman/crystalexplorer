#include "dynamicstructure.h"
#include "occ/core/element.h"

// Implementation

DynamicStructure::DynamicStructure(QObject *parent)
    : ChemicalStructure(parent) {}

void DynamicStructure::addFrame(ChemicalStructure *frame) {
  m_frames.append(frame);
  frame->setParent(this); // Set parent for proper memory management
  if (m_currentFrameIndex == -1) {
    setCurrentFrameIndex(0);
  }
  emit frameAdded(m_frames.size() - 1);
}

void DynamicStructure::removeFrame(int index) {
  if (index >= 0 && index < m_frames.size()) {
    delete m_frames.takeAt(index);
    if (m_currentFrameIndex >= m_frames.size()) {
      setCurrentFrameIndex(m_frames.size() - 1);
    } else if (m_currentFrameIndex > index) {
      --m_currentFrameIndex;
    }
    emit frameRemoved(index);
  }
}

void DynamicStructure::setCurrentFrameIndex(int index) {
  if (index >= 0 && index < m_frames.size() && index != m_currentFrameIndex) {
    m_currentFrameIndex = index;
    updateFromCurrentFrame();
    emit currentFrameChanged(m_currentFrameIndex);
  }
}

ChemicalStructure *DynamicStructure::currentFrame() {
  return (m_currentFrameIndex >= 0) ? m_frames[m_currentFrameIndex] : nullptr;
}

const ChemicalStructure *DynamicStructure::currentFrame() const {
  return (m_currentFrameIndex >= 0) ? m_frames[m_currentFrameIndex] : nullptr;
}

void DynamicStructure::updateFromCurrentFrame() {
  if (auto frame = currentFrame()) {
    const auto numAtoms = frame->numberOfAtoms();
    auto pos = frame->atomicPositions();
    auto nums = frame->atomicNumbers();
    auto labels = frame->labels();

    std::vector<QString> elementSymbols(numAtoms);
    std::vector<occ::Vec3> positions(numAtoms);

    for (int i = 0; i < numAtoms; i++) {
      elementSymbols[i] =
          QString::fromStdString(occ::core::Element(nums(i)).symbol());
      positions[i] = pos.col(i);
    }
    clearAtoms();
    setAtoms(elementSymbols, positions, labels);

    for (int i = 0; i < numAtoms; i++) {
      auto idx = indexToGenericIndex(i);
      setAtomFlags(idx, frame->atomFlags(idx));
    }
    updateBondGraph();
  }
}

ChemicalStructure::StructureType DynamicStructure::structureType() const {
  if (auto frame = currentFrame()) {
    return frame->structureType();
  }
  return ChemicalStructure::StructureType::Cluster;
}
