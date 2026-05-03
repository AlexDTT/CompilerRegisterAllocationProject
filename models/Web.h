/**
 * @file Web.h
 * @brief Data model for a live web (merged live ranges of a variable).
 */

#ifndef WEB_H
#define WEB_H

#include <string>
#include <set>
#include <vector>
#include "models/LiveRange.h"

/**
 * @struct Web
 * @brief A live web is the union of one or more live ranges of the same variable
 *        whose program-point sets overlap.
 *
 * The web is the fundamental unit used to build the interference graph.
 * Two webs interfere if there exists at least one program point where both are
 * simultaneously live (with the subtlety that a definition at the same point as
 * a last-use does not count as interference).
 */
struct Web
{
  int id;                        ///< Unique numeric web identifier (0-based).
  std::string variable;          ///< The variable name this web belongs to.
  std::vector<LiveRange> ranges; ///< Constituent live ranges merged into this web.

  /**
   * @brief Returns the set of all line numbers covered by this web.
   * @complexity O(N) where N is the total number of program points across all ranges.
   */
  std::set<int> liveLines() const
  {
    std::set<int> ls;
    for (const auto &r : ranges)
      for (const auto &p : r.points)
        ls.insert(p.line);
    return ls;
  }

  /**
   * @brief Returns the marker for a given line number within this web, or '\0' if
   *        the line does not appear or has a plain (intermediate) marker.
   *
   * If the same line has both '+' and '-' markers across different constituent ranges
   * (which can happen when ranges are merged), '+' takes precedence over '-', and both
   * take precedence over '\0'.
   *
   * @param line The line number to query.
   * @complexity O(N) where N is the total number of program points.
   */
  char markerAt(int line) const
  {
    char best = '\0';
    for (const auto &r : ranges)
    {
      for (const auto &p : r.points)
      {
        if (p.line == line)
        {
          if (p.marker == '+')
            return '+'; // highest priority
          if (p.marker == '-')
            best = '-';
        }
      }
    }
    return best;
  }

  /**
   * @brief Checks whether this web is live (covers) a given line.
   * @param line The line number.
   * @return true if the line appears in any constituent range.
   * @complexity O(N)
   */
  bool isLiveAt(int line) const
  {
    for (const auto &r : ranges)
      for (const auto &p : r.points)
        if (p.line == line)
          return true;
    return false;
  }
};

#endif // WEB_H
