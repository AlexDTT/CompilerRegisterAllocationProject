/**
 * @file RegisterAllocApp.h
 * @brief Main application controller: CLI and batch-mode entry points.
 */

#ifndef REGISTER_ALLOC_APP_H
#define REGISTER_ALLOC_APP_H

#include <map>
#include <string>
#include <vector>
#include "data_structures/Graph.h"
#include "data_structures/InterferenceGraph.h"
#include "models/LiveRange.h"
#include "models/Parameters.h"
#include "models/Web.h"
#include "services/AllocationLogic.h"

/**
 * @class RegisterAllocApp
 * @brief Top-level application controller.
 *
 * Supports two execution modes:
 *  - **Interactive**: a menu-driven CLI for loading files, inspecting webs and the
 *    interference graph, running algorithms, and writing output.
 *  - **Batch**: non-interactive, invoked as
 *    @code
 *      ./register_alloc -b ranges.txt registers.txt allocation.txt
 *    @endcode
 *    where the last argument is the output file path.
 */
class RegisterAllocApp
{
public:
  RegisterAllocApp();

  /**
   * @brief Entry point called from main().
   *
   * Examines @p argc / @p argv to decide between batch and interactive mode,
   * then delegates accordingly.
   *
   * @param argc Argument count.
   * @param argv Argument vector.
   * @return 0 on success, non-zero on error.
   */
  int run(int argc, char *argv[]);

private:
  // ------------------------------------------------------------------
  // Application state
  // ------------------------------------------------------------------
  std::map<std::string, std::vector<LiveRange>> mRanges; ///< Parsed live ranges.
  std::vector<Web> mWebs;                                ///< Constructed webs.
  Graph<int> mGraph;                                     ///< Interference graph.
  Parameters mParams;                                    ///< Current config.
  bool mDataLoaded;                                      ///< True after ranges file loaded.
  bool mConfigLoaded;                                    ///< True after config file loaded.
  bool mGraphBuilt;                                      ///< True after graph is built.
  AllocationResult mLastResult;                          ///< Most recent allocation result.

  // ------------------------------------------------------------------
  // Helper utilities (match MaxflowProject style)
  // ------------------------------------------------------------------
  void printSep(char c = '-', int n = 57) const;
  void waitEnter() const;
  int readInt(const std::string &prompt) const;
  std::string readLine(const std::string &prompt) const;

  // ------------------------------------------------------------------
  // Core logic
  // ------------------------------------------------------------------
  bool loadRangesFile(const std::string &path);
  bool loadConfigFile(const std::string &path);
  void buildGraphFromData();
  void runAllocation();

  // ------------------------------------------------------------------
  // Execution modes
  // ------------------------------------------------------------------
  void runInteractiveMode();
  int runBatchMode(const std::string &rangesFile,
                   const std::string &configFile,
                   const std::string &outputFile);

  // ------------------------------------------------------------------
  // Interactive menus
  // ------------------------------------------------------------------
  void printMainMenu() const;
  void menuDataManagement();
  void menuConfiguration();
  void menuAlgorithms();
  void menuVisualization();

  // ------------------------------------------------------------------
  // Data management actions
  // ------------------------------------------------------------------
  void doLoadInputPairFromFolder();
  void doLoadInputPairManual();
  void doLoadRangesFile();
  void doLoadConfigFile();
  void doViewWebs() const;
  void doViewParameters() const;

  // ------------------------------------------------------------------
  // Algorithm actions
  // ------------------------------------------------------------------
  void doRunAllocation();

  // ------------------------------------------------------------------
  // Visualization actions
  // ------------------------------------------------------------------
  void doViewGraphTerminal() const;
  void doExportGraphDOT() const;
  void doExportAllocationDOT() const;
  void doShowAllocationResult() const;
};

#endif // REGISTER_ALLOC_APP_H
