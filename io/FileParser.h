/**
 * @file FileParser.h
 * @brief Parses the live-ranges input file and the registers/config file.
 */

#ifndef FILE_PARSER_H
#define FILE_PARSER_H

#include <map>
#include <string>
#include <vector>
#include "models/LiveRange.h"
#include "models/Parameters.h"

/**
 * @class FileParser
 * @brief Utility class with static methods to parse the two input files required
 *        by the compiler register allocation tool.
 *
 * Two distinct file formats are handled:
 *  - **Ranges file**: maps variable names to their live ranges.
 *  - **Config file**: specifies the number of registers and the algorithm to use.
 */
class FileParser
{
public:
  /**
   * @brief Parses the live-ranges input file.
   *
   * Expected format (one live range per line):
   * @code
   *   # comment
   *   varname: 7+, 8, 9, 10-
   *   varname: 15+, 16, 17-
   *   other:   3+, 4-
   * @endcode
   * A '+' suffix on a line number marks the start (definition); '-' marks the end (last use).
   * Ranges without '+'/'-' on their endpoints are allowed (intersection-only ranges).
   * Malformed point tokens and duplicate line numbers within one range are rejected.
   * Validation rule: for each variable, at least one '+' and at least one '-' must
   * appear across all ranges of that variable.
   *
   * @param filename Path to the ranges text file.
   * @param[out] ranges Map from variable name to the list of its parsed live ranges.
   * @return true on success, false if the file could not be opened or contains fatal errors.
   * @complexity O(L * P) where L is the number of lines and P the average number of
   *             program points per line.
   */
  static bool parseRanges(const std::string &filename,
                          std::map<std::string, std::vector<LiveRange>> &ranges);

  /**
   * @brief Parses the registers / config input file.
   *
   * Expected format:
   * @code
   *   # comment
   *   registers: N
   *   algorithm: basic
   *   # or: algorithm: spilling, 2
   *   # or: algorithm: splitting, 2
   *   # or: algorithm: free
   *   # or: algorithm: free_split
   * @endcode
   *
   * @param filename Path to the config text file.
   * @param[out] params Populated Parameters struct.
   * @return true on success, false if the file could not be opened or has invalid content.
   * @complexity O(L) where L is the number of lines in the config file.
   */
  static bool parseConfig(const std::string &filename, Parameters &params);

private:
  /**
   * @brief Strips leading/trailing whitespace and removes everything after '#'.
   * @param s Input string.
   * @return Cleaned string.
   * @complexity O(N) where N is the string length.
   */
  static std::string cleanLine(const std::string &s);

  /**
   * @brief Parses a comma-separated list of program points such as "7+,8,9,10-".
   * @param token The raw token string.
   * @param[out] points Populated vector of ProgramPoint.
   * @return true if all tokens were valid.
   * @complexity O(P) where P is the number of comma-separated entries.
   */
  static bool parsePoints(const std::string &token,
                          std::vector<ProgramPoint> &points);
};

#endif // FILE_PARSER_H
