/**
 * @file OutputWriter.h
 * @brief Writes the register allocation result to a text file.
 */

#ifndef OUTPUT_WRITER_H
#define OUTPUT_WRITER_H

#include <map>
#include <string>
#include <vector>
#include "models/Web.h"

/**
 * @class OutputWriter
 * @brief Serialises allocation results to the human-readable output format.
 *
 * Output format:
 * @code
 *   # Total number of webs followed by the listing of the program points of each one
 *   # program points in each web are sorted in ascending order
 *   webs: 4
 *   web0: 1+,2,3,4,5,6-
 *   web1: 9+,10,11,12,13,14-,20+
 *   ...
 *   # Total number of registers used, followed by assignment to webs
 *   registers: 2
 *   r0: web0
 *   r0: web1
 *   r1: web2
 *   # or, for spilled webs:
 *   M: web3
 *   # optional metadata for alternative algorithms:
 *   spills: 1
 *   spill: web3
 *   splits: 1
 *   split: web0 -> web0,web4
 * @endcode
 */
class OutputWriter
{
public:
  /**
   * @brief Writes the full allocation result to @p filename.
   *
   * @param filename      Destination file path.
   * @param webs          The ordered list of webs.
   * @param webToRegister Map from web ID to physical register index (0-based).
   *                      A value of -1 indicates the web was spilled to memory.
   * @param metadataLines Optional processing-friendly metadata records for
   *                      alternative algorithms, such as selected spills/splits.
   * @return true on success, false if the file could not be created.
   * @complexity O(W log W + P log P + A + M) where W is the number of webs,
   *             P is the total number of program points across all webs, A is
   *             the number of allocation entries, and M is the number of
   *             metadata lines.
   */
  static bool write(const std::string &filename,
                    const std::vector<Web> &webs,
                    const std::map<int, int> &webToRegister,
                    const std::vector<std::string> &metadataLines = {});

private:
  /**
   * @brief Formats a whole web as a single sorted program-point list.
   * @param web The web to format.
   * @return Formatted string.
   * @complexity O(P log P) where P is the number of points across the web.
   */
  static std::string formatWeb(const Web &web);
};

#endif // OUTPUT_WRITER_H
