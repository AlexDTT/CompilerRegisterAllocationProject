/**
 * @file OutputWriter.cpp
 * @brief Implementation of OutputWriter.
 */

#include "io/OutputWriter.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <set>
#include <sstream>

// ---------------------------------------------------------------------------
// OutputWriter::formatWeb
// ---------------------------------------------------------------------------
std::string OutputWriter::formatWeb(const Web &web)
{
  std::map<int, std::set<char>> markersByLine;
  for (const auto &range : web.ranges)
  {
    for (const auto &point : range.points)
      markersByLine[point.line].insert(point.marker);
  }

  std::ostringstream oss;
  bool first = true;
  for (const auto &[line, markers] : markersByLine)
  {
    if (!first)
      oss << ',';

    char marker = '\0';
    const bool hasPlus = markers.count('+') > 0;
    const bool hasMinus = markers.count('-') > 0;
    if (hasPlus && hasMinus)
      marker = '\0';
    else if (hasPlus)
      marker = '+';
    else if (hasMinus)
      marker = '-';

    oss << line;
    if (marker != '\0')
      oss << marker;
    first = false;
  }
  return oss.str();
}

// ---------------------------------------------------------------------------
// OutputWriter::write
// ---------------------------------------------------------------------------
bool OutputWriter::write(const std::string &filename,
                         const std::vector<Web> &webs,
                         const std::map<int, int> &webToRegister)
{
  std::ofstream out(filename);
  if (!out.is_open())
  {
    std::cerr << "Error: cannot write to '" << filename << "'.\n";
    return false;
  }

  // ---- Web section -------------------------------------------------------
  out << "# Total number of webs followed by the listing of the program points of each one\n";
  out << "# program points in each web are sorted in ascending order\n";
  out << "webs: " << webs.size() << "\n";

  std::vector<Web> orderedWebs = webs;
  std::sort(orderedWebs.begin(), orderedWebs.end(),
            [](const Web &lhs, const Web &rhs)
            {
              return lhs.id < rhs.id;
            });

  for (const auto &web : orderedWebs)
  {
    out << "web" << web.id << ": " << formatWeb(web) << "\n";
  }

  // ---- Register section --------------------------------------------------
  // Count how many distinct registers are actually assigned (excluding spills)
  std::set<int> usedRegs;
  for (const auto &[webId, reg] : webToRegister)
  {
    if (reg >= 0)
      usedRegs.insert(reg);
  }

  bool allSpilled = usedRegs.empty();
  if (allSpilled)
  {
    std::cerr << "Warning: register allocation was not feasible – all webs spilled to memory.\n";
  }

  out << "# Total number of registers used, followed by assignment to webs\n";
  out << "registers: " << (allSpilled ? 0 : (int)usedRegs.size()) << "\n";

  // Group webs by register for output
  // First output register assignments, then memory spills
  std::map<int, std::vector<int>> regToWebs;
  std::vector<int> spilledWebs;
  for (const auto &web : orderedWebs)
  {
    auto it = webToRegister.find(web.id);
    int reg = (it != webToRegister.end()) ? it->second : -1;
    if (reg >= 0)
      regToWebs[reg].push_back(web.id);
    else
      spilledWebs.push_back(web.id);
  }

  for (const auto &[reg, wids] : regToWebs)
  {
    for (int wid : wids)
    {
      out << "r" << reg << ": web" << wid << "\n";
    }
  }
  for (int wid : spilledWebs)
  {
    out << "M: web" << wid << "\n";
  }

  return true;
}
