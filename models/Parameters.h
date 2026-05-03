/**
 * @file Parameters.h
 * @brief Configuration parameters parsed from the registers/config input file.
 */

#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>

/**
 * @enum AlgorithmType
 * @brief Selects which register allocation algorithm variant to run.
 */
enum class AlgorithmType
{
  Basic,     ///< Basic greedy graph-coloring. No spilling or splitting.
  Spilling,  ///< Graph coloring with web spilling (commit webs to memory).
  Splitting, ///< Graph coloring with web splitting (split webs into sub-webs).
  Free       ///< Custom allocation algorithm.
};

/**
 * @struct Parameters
 * @brief Holds the configuration read from the registers/config input file.
 *
 * Expected file format:
 * @code
 *   # comment
 *   registers: N
 *   algorithm: basic
 *   # or: algorithm: spilling, 2
 *   # or: algorithm: splitting, 2
 *   # or: algorithm: free
 * @endcode
 */
struct Parameters
{
  int numRegisters = 0;                           ///< Maximum number of available physical registers.
  AlgorithmType algorithm = AlgorithmType::Basic; ///< Algorithm variant to use.
  int algorithmParam = 0;                         ///< K parameter for spilling/splitting algorithms.
  std::string outputFile = "allocation.txt";      ///< Path to the output allocation file.
};

#endif // PARAMETERS_H
