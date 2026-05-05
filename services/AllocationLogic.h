/**
 * @file AllocationLogic.h
 * @brief Orchestration layer: runs the selected algorithm and writes output.
 */

#ifndef ALLOCATION_LOGIC_H
#define ALLOCATION_LOGIC_H

#include <map>
#include <string>
#include <vector>
#include "algorithms/GraphColoring.h"
#include "data_structures/Graph.h"
#include "models/Parameters.h"
#include "models/Web.h"

/**
 * @class AllocationLogic
 * @brief Central service that ties together web construction, interference graph
 *        building, algorithm dispatch, and output serialisation.
 */
class AllocationLogic
{
public:
  /**
   * @brief Runs the register allocation specified by @p params.
   *
   * Dispatches to the appropriate GraphColoring method based on
   * params.algorithm, then writes the result to params.outputFile via OutputWriter.
   *
   * @param webs         Webs produced by InterferenceGraph::buildWebs().
   * @param graph        Interference graph produced by InterferenceGraph::buildGraph().
   * @param params       Configuration (registers, algorithm, output file).
   * @return AllocationResult from the chosen algorithm.
   * @complexity Depends on the selected algorithm variant.
   */
  static AllocationResult runAllocation(std::vector<Web> &webs,
                                        Graph<int> &graph,
                                        const Parameters &params);

  /**
   * @brief Prints a human-readable summary of the allocation result to stdout.
   *
   * Lists each web with its variable name, constituent ranges, and the assigned
   * register (or 'M' for memory-spilled webs).
   *
   * @param webs   The web list.
   * @param result The allocation result to summarise.
   * @complexity O(P + W log A) where P is the total number of program points
   *             printed, W is the number of webs, and A is the number of
   *             allocation entries queried.
   */
  static void printResult(const std::vector<Web> &webs,
                          const AllocationResult &result);

  /**
   * @brief Computes the maximum clique size (lower bound on chromatic number) of the
   *        interference graph using a greedy heuristic.
   *
   * This gives a quick lower bound on the minimum number of registers required.
   *
   * @param graph The interference graph.
   * @param webs  The web list.
   * @return Estimated lower bound on the chromatic number.
   * @complexity O(W^3 + W * E) in the worst case with the current vector-backed
   *             Graph<int>, where W is the number of webs and E is the number of
   *             directed adjacency entries.
   */
  static int estimateChromatic(const Graph<int> &graph,
                               const std::vector<Web> &webs);
};

#endif // ALLOCATION_LOGIC_H
