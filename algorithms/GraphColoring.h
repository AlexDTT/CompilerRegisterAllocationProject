/**
 * @file GraphColoring.h
 * @brief Graph-coloring-based register allocation algorithms.
 *
 * Four algorithm variants are provided:
 *  - **Basic**: Chaitin-style greedy graph coloring.  When no spilling or splitting is
 *    allowed and the graph cannot be colored with the given number of registers, the
 *    allocation is reported as infeasible.
 *  - **Spilling**: Greedy coloring with selective web spilling (committing webs to memory)
 *    to reduce the graph's chromatic number.
 *  - **Splitting**: Greedy coloring with selective web splitting (breaking webs into
 *    sub-webs with fewer interferences) to enable coloring.
 *  - **Free**: A custom allocation strategy (student-defined).
 */

#ifndef GRAPH_COLORING_H
#define GRAPH_COLORING_H

#include <map>
#include <vector>
#include "data_structures/Graph.h"
#include "models/Parameters.h"
#include "models/Web.h"

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
   * @brief Basic greedy Chaitin-style graph coloring.
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
   * When basic coloring fails, iteratively selects up to @p maxSpills webs to commit
   * to memory (spill) and retries coloring on the reduced graph.  The spill-candidate
   * selection heuristic should minimize the total number of spills while maximizing
   * the chance of a successful coloring (e.g., prefer high-degree webs).
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
   * When basic coloring fails, iteratively splits up to @p maxSplits webs into two
   * derived sub-webs, reducing interference edges and hopefully allowing a coloring
   * with the same number of registers.  The split-point selection heuristic has a
   * strong influence on the result.
   *
   * After splitting, the web list and interference graph must be rebuilt before
   * reattempting coloring.
   *
   * @param graph        The original interference graph.
   * @param webs         Ordered list of webs (may be extended with derived webs).
   * @param numRegisters Number of available physical registers (K).
   * @param maxSplits    Maximum number of webs that may be split.
   * @return AllocationResult for the (possibly extended) web list.
   * @complexity O((maxSplits + 1) * (W^2 * P + E * W + W * (W^2 + E))) in the
   *             worst case, where P is the maximum number of points compared
   *             per web pair. Each split rebuilds the interference graph and
   *             reruns coloring.
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
   * Free-form implementation: any approach is allowed as long as two interfering webs
   * are never assigned the same register.  Describe the rationale in the documentation.
   *
   * @param graph        The interference graph.
   * @param webs         Ordered list of webs.
   * @param numRegisters Number of available physical registers (K).
   * @return AllocationResult with the register assignment.
   * @complexity O(W * (W^2 + E)) with the vector-backed Graph<int>, because each
   *             DSATUR selection round scans the remaining vertices and their
   *             adjacency lists.
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
   * @complexity O(E_v) where E_v is the number of edges incident to @p id.
   */
  static int activeDegree(const Graph<int> &graph, int id,
                          const std::set<int> &active);

  /**
   * @brief Selects the best spill candidate from @p active based on a heuristic.
   *
   * Default heuristic: highest active degree (most constrained node).  Can be extended
   * to consider spill cost (live-range length, usage frequency).
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
