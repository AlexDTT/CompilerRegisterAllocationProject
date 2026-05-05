/**
 * @file InterferenceGraph.h
 * @brief Builds live webs and the interference graph from parsed live ranges.
 */

#ifndef INTERFERENCE_GRAPH_H
#define INTERFERENCE_GRAPH_H

#include <map>
#include <string>
#include <vector>
#include "data_structures/Graph.h"
#include "models/LiveRange.h"
#include "models/Web.h"

/**
 * @class InterferenceGraph
 * @brief Factory and utilities for constructing webs and their interference graph.
 *
 * **Web construction** (same variable, multiple live ranges):
 * Two live ranges of the same variable are merged into a single web if they share at
 * least one program-point line number.  Transitively connected ranges all form one web.
 * This is computed with a Union-Find approach over the range indices.
 *
 * **Interference** (two different webs):
 * Webs A and B interfere if there exists a line L such that both webs are live at L,
 * *except* when one web's last use ('-') coincides with the other web's first definition
 * ('+') at that same line — in that case they do not interfere (the register can be
 * reused).
 *
 * The resulting graph uses `Graph<int>` where the vertex info is the web ID (0-based).
 * Interference is represented as bidirectional (undirected) edges.
 */
class InterferenceGraph
{
public:
  /**
   * @brief Builds the web list from parsed live ranges.
   *
   * Ranges of the same variable whose program-point sets overlap are merged into a
   * single web using Union-Find on range indices.
   *
   * @param rawRanges Map from variable name to its list of LiveRange objects.
   * @return Ordered list of webs (web IDs are 0-based indices into this vector).
   * @complexity O(V * R^2 * P log P) where V is the number of variables, R the
   *             maximum number of ranges per variable, and P the maximum number of
   *             program points per range. The log factor comes from the set used
   *             by the range-overlap test.
   */
  static std::vector<Web> buildWebs(
      const std::map<std::string, std::vector<LiveRange>> &rawRanges);

  /**
   * @brief Builds the interference graph from the web list.
   *
   * One vertex per web (info = web.id).  An undirected edge (bidirectional pair) is
   * added between every pair of webs that interfere.
   *
   * @param webs The list of webs produced by buildWebs().
   * @return The interference graph.  The caller owns the returned graph.
   * @complexity O(W^2 * P + E * W) where W is the number of webs, P is the maximum
   *             number of points inspected per web pair, and E is the number of
   *             interference edges. The E * W term comes from Graph<int>'s linear
   *             endpoint lookup while inserting bidirectional edges.
   */
  static Graph<int> buildGraph(const std::vector<Web> &webs);

  /**
   * @brief Checks whether two webs interfere.
   *
   * Interference exists if there is a shared line L where both webs are live, with the
   * exception that if one web has marker '-' (last use) and the other has marker '+'
   * (first definition) at L, they do not interfere.
   *
   * @param a First web.
   * @param b Second web.
   * @return true if the webs interfere, false otherwise.
   * @complexity O(P_a + P_b), building marker maps for both webs and probing by
   *             shared line number.
   */
  static bool interferes(const Web &a, const Web &b);

  /**
   * @brief Prints the interference graph to stdout for debugging.
   * @param graph The interference graph.
   * @param webs  The web list (for variable-name annotations).
   * @complexity O(W + E) where W is the number of webs and E the number of edges.
   */
  static void printGraph(const Graph<int> &graph, const std::vector<Web> &webs);

  /**
   * @brief Exports the interference graph in DOT format to @p filename.
   * @param graph    The interference graph.
   * @param webs     The web list (for vertex labels).
   * @param filename Destination file path.
   * @complexity O(W + E)
   */
  static void exportDOT(const Graph<int> &graph,
                        const std::vector<Web> &webs,
                        const std::string &filename);
};

#endif // INTERFERENCE_GRAPH_H
