#pragma once
#include <occ/core/graph.h>

/**
 * Class representing and holding data for a graph edge for a
 * molecular dimer in 3D periodic boundary conditions.
 */
struct PeriodicDimerEdge {
  enum class Connection { HydrogenBond, Undefined };

  double nearestAtomDistance{0.0};
  double centroidDistance{0.0};
  double centerOfMassDistance{0.0};
  size_t sourceIndex{0}, targetIndex{0};
  int asymmetricDimerIndex{-1};
  int h{0}, k{0}, l{0};
  Connection connectionType{Connection::Undefined};
};

/**
 * Class representing and holding data for a graph vertex in 3D periodic
 * boundary conditions.
 */
struct PeriodicDimerVertex {
  size_t unitCellIndex{0};
};

using PeriodicDimerGraph =
    occ::core::graph::Graph<PeriodicDimerVertex, PeriodicDimerEdge>;
