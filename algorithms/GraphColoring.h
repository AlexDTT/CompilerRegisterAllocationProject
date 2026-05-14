/**
 * @file GraphColoring.h
 * @brief Graph-coloring-based register allocation algorithms.
 *
 * Four algorithm variants are provided:
 *  - **Basic**: simplify/select greedy graph coloring. When the graph cannot be
 *    colored with the configured number of registers without recovery actions, the
 *    allocation is reported as infeasible.
 *  - **Spilling**: Greedy coloring with selective web spilling (committing webs to memory)
 *    to reduce the graph's chromatic number.
 *  - **Splitting**: selective web splitting that preserves the original live-range
 *    markers and tries the split points that most reduce interference.
 *  - **Free**: custom hybrid coloring with graph-class fast paths, DSatur ordering,
 *    and a bounded exact branch-and-bound pass for small/medium graphs.
 */

#ifndef GRAPH_COLORING_H
#define GRAPH_COLORING_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include "data_structures/Graph.h"
#include "models/Parameters.h"
#include "models/Web.h"

/**
 * @struct SplitRecord
 * @brief Processing-friendly description of one performed web split.
 */
struct SplitRecord
{
  int sourceWebId = -1; ///< Web selected for splitting before the operation.
  int leftWebId = -1;   ///< First derived web id. The implementation reuses sourceWebId.
  int rightWebId = -1;  ///< Second derived web id created by the split.
};

/**
 * @struct AllocationResult
 * @brief The outcome of a register allocation run.
 */
struct AllocationResult
{
  /// Maps web ID → physical register index (0-based), or -1 if the web was spilled.
  std::map<int, int> webToRegister;

  /// True when the selected algorithm produced a valid allocation result.
  /// For the basic algorithm this requires all webs to be colored.
  /// For spilling/free algorithms, selected memory assignments are allowed.
  bool feasible = false;

  /// Number of physical registers actually used.
  int registersUsed = 0;

  /// Number of webs assigned to memory.
  int spilledWebs = 0;

  /// Web ids deliberately selected for memory by spilling-capable algorithms.
  std::set<int> selectedSpills;

  /// Records of web splits performed by splitting-capable algorithms.
  std::vector<SplitRecord> splitRecords;
};

/**
 * @class GraphColoring
 * @brief Static utility class implementing all register-allocation algorithm variants.
 */
class GraphColoring
{
public:
  // ------------------------------------------------------------------
  // T2.1 – Basic greedy graph coloring
  // ------------------------------------------------------------------

  /**
   * @brief Basic simplify/select graph coloring.
   *
   * Repeatedly removes vertices with degree < numRegisters from a working copy of the
   * graph, pushing them onto a stack.  If all remaining vertices have degree ≥
   * numRegisters a spill candidate is chosen (web with highest degree) and marked as
   * spilled (webToRegister = -1).  The stack is then popped and each vertex is
   * greedily assigned the lowest-numbered color not used by its already-colored
   * neighbors.
   *
   * @param graph        The interference graph (web IDs as vertex info).
   * @param webs         Ordered list of webs.
   * @param numRegisters Number of available physical registers (K).
   * @return AllocationResult with the register assignment.
   * @complexity O(W * (W^2 + E)) where W is the number of webs and E is the
   *             number of directed adjacency entries in the current Graph<int>.
   *             The extra W factor comes from repeated simplify passes and the
   *             course graph's O(W) findVertex lookup.
   *
   */
  static AllocationResult basicColoring(const Graph<int> &graph,
                                        const std::vector<Web> &webs,
                                        int numRegisters);

  // ------------------------------------------------------------------
  // T2.2 – Greedy coloring with web spilling
  // ------------------------------------------------------------------

  /**
   * @brief Graph coloring with selective web spilling.
   *
   * Iteratively tries reduced graphs with zero, one, ..., @p maxSpills forced
   * memory assignments. Each new spill candidate is the highest-degree active web,
   * because removing a highly constrained web usually removes more interference
   * edges and gives the remaining graph the best chance of becoming colorable.
   *
   * @param graph        The interference graph.
   * @param webs         Ordered list of webs.
   * @param numRegisters Number of available physical registers (K).
   * @param maxSpills    Maximum number of webs that may be spilled.
   * @return AllocationResult (spilled webs have webToRegister = -1).
   * @complexity O((maxSpills + 1) * W * (W^2 + E)) with the implemented
   *             high-degree spill heuristic and vector-backed Graph<int>.
   *
   */
  static AllocationResult spillingColoring(const Graph<int> &graph,
                                           const std::vector<Web> &webs,
                                           int numRegisters,
                                           int maxSpills);

  // ------------------------------------------------------------------
  // T2.3 – Greedy coloring with web splitting
  // ------------------------------------------------------------------

  /**
   * @brief Graph coloring with selective web splitting.
   *
   * Iteratively splits up to @p maxSplits webs into two derived sub-webs and rebuilds
   * the interference graph.  Each iteration evaluates all legal split positions and
   * chooses the split that first makes the graph K-colorable; if none does, it chooses
   * the split that leaves the fewest interference edges and then the lowest maximum
   * degree.
   *
   * Split boundaries preserve the original markers exactly.  For example, splitting
   * `1+,2,3,4-` into `1+,2` and `3,4-` does not invent a synthetic `3+` marker.
   * Candidate splits whose two derived webs still interfere with each other are
   * discarded.
   *
   * @param graph        The original interference graph.
   * @param webs         Ordered list of webs (may be extended with derived webs).
   * @param numRegisters Number of available physical registers (K).
   * @param maxSplits    Maximum number of webs that may be split.
   * @return AllocationResult for the (possibly extended) web list.
   * @complexity O(maxSplits * Q * (W^2 * P + E * W + W * (W^2 + E))) in the
   *             worst case, where Q is the number of candidate split positions
   *             considered in one iteration.  Each candidate rebuilds an
   *             interference graph and runs the K-colorability test.
   *
   */
  static AllocationResult splittingColoring(Graph<int> &graph,
                                            std::vector<Web> &webs,
                                            int numRegisters,
                                            int maxSplits);

  // ------------------------------------------------------------------
  // T2.4 – Custom algorithm
  // ------------------------------------------------------------------

  /**
   * @brief Custom register allocation algorithm.
   *
   * This allocator first recognizes simple graph classes that can be colored
   * optimally by direct algorithms:
   *  - edgeless graphs use one register;
   *  - complete graphs color up to K webs and spill the unavoidable remainder;
   *  - bipartite graphs are 2-colored by BFS when K >= 2.
   *
   * For the remaining graphs it runs DSatur-style greedy coloring (highest
   * saturation degree, then highest degree).  For graphs up to the implementation's
   * small/medium threshold it then runs branch-and-bound over the same DSatur order
   * to minimize the number of spilled webs.  Larger graphs keep the polynomial
   * DSatur result to avoid exponential runtimes in the demo tool.
   *
   * @param graph        The interference graph.
   * @param webs         Ordered list of webs.
   * @param numRegisters Number of available physical registers (K).
   * @return AllocationResult with the register assignment.
   * @complexity Fast paths are O(W + E) after adjacency extraction.  The DSatur
   *             fallback is O(W^2 + E log W) after extraction.  The exact
   *             small/medium pass has exponential worst-case complexity
   *             O((K + 1)^W * (W + E)), but it is guarded by a fixed vertex
   *             threshold.
   *
   */
  static AllocationResult freeColoring(const Graph<int> &graph,
                                       const std::vector<Web> &webs,
                                       int numRegisters);

private:
  /**
   * @brief Returns the degree of vertex @p id in the sub-graph defined by @p active.
   * @param graph  The full interference graph.
   * @param id     Web ID of the vertex.
   * @param active Set of currently active (non-removed) web IDs.
   * @return Number of active neighbors.
   * @complexity O(W + E_v), where W is the number of graph vertices and E_v is
   *             the number of edges incident to @p id. The W term is the linear
   *             Graph<int>::findVertex lookup.
   */
  static int activeDegree(const Graph<int> &graph, int id,
                          const std::set<int> &active);

  /**
   * @brief Selects the best spill candidate from @p active based on a heuristic.
   *
   * Heuristic: choose the highest active degree, then the lowest web id as a
   * deterministic tie-breaker. This removes the web with the most current
   * interference constraints.
   *
   * @param graph  The full interference graph.
   * @param active Set of currently active web IDs.
   * @return Web ID of the chosen spill candidate.
   * @complexity O(W^2 + E) for a full active set, because activeDegree performs
   *             an O(W) vertex lookup and scans each active adjacency list.
   */
  static int pickSpillCandidate(const Graph<int> &graph,
                                const std::set<int> &active);
};

#endif // GRAPH_COLORING_H
