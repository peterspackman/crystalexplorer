#pragma once
#include <occ/core/graph.h>

namespace occ::core::graph {

/**
 * Class representing and holding data for a graph edge in 3D periodic boundary
 * conditions.
 */
struct PeriodicEdge {
  enum class Connection { CovalentBond, HydrogenBond, CloseContact };

  double dist{0.0};
  size_t source{0}, target{0};
  size_t source_asym_idx{0}, target_asym_idx{0};
  int h{0}, k{0}, l{0};
  Connection connectionType{Connection::CovalentBond};
};

/**
 * Class representing and holding data for a graph vertex in 3D periodic
 * boundary conditions.
 */
struct PeriodicVertex {
  size_t uc_idx{0};
};

using PeriodicBondGraph = Graph<PeriodicVertex, PeriodicEdge>;

struct Edge {
  enum class Connection : int { CovalentBond, HydrogenBond, CloseContact };
  double dist{0.0};
  size_t source{0}, target{0};
  Connection connectionType{Connection::CovalentBond};
};

struct Vertex {
  size_t index{0};
};

using BondGraph = Graph<Vertex, Edge>;

} // namespace occ::core::graph
