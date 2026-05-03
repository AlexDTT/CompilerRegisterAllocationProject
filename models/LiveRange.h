/**
 * @file LiveRange.h
 * @brief Data model for a single live range of a variable.
 */

#ifndef LIVE_RANGE_H
#define LIVE_RANGE_H

#include <string>
#include <vector>

/**
 * @struct ProgramPoint
 * @brief A single program point (instruction line number) within a live range.
 *
 * A live range's start point carries the '+' marker (variable is defined/written).
 * A live range's end point carries the '-' marker (variable is used for the last time).
 * Intermediate points have marker '\0'.
 */
struct ProgramPoint
{
  int line;    ///< 1-based instruction line number.
  char marker; ///< '+' = first definition, '-' = last use, '\0' = intermediate.

  ProgramPoint(int l, char m = '\0') : line(l), marker(m) {}

  bool operator<(const ProgramPoint &o) const { return line < o.line; }
  bool operator==(const ProgramPoint &o) const { return line == o.line; }
};

/**
 * @struct LiveRange
 * @brief Represents a single live range for a named variable.
 *
 * Corresponds to one input line of the form:
 * @code
 *   varname: 7+, 8, 9, 10-
 * @endcode
 * where the first point has '+' and the last has '-'.
 */
struct LiveRange
{
  std::string variable;             ///< The name of the variable this range belongs to.
  std::vector<ProgramPoint> points; ///< Ordered list of program points in this range.

  /**
   * @brief Returns the set of plain line numbers covered by this range.
   * @complexity O(N) where N is the number of points.
   */
  std::vector<int> lines() const
  {
    std::vector<int> ls;
    ls.reserve(points.size());
    for (const auto &p : points)
      ls.push_back(p.line);
    return ls;
  }
};

#endif // LIVE_RANGE_H
