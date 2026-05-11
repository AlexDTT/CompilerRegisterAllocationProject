/**
 * @file AllocationLogic.cpp
 * @brief Implementation of the AllocationLogic orchestration layer.
 */

#include "services/AllocationLogic.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <fstream>
#include <set>
#include <sstream>
#include <vector>
#include "io/OutputWriter.h"

namespace
{
  std::vector<std::string> buildMetadataLines(const AllocationResult &result,
                                              const Parameters &params)
  {
    std::vector<std::string> lines;

    if (params.algorithm == AlgorithmType::Spilling ||
        params.algorithm == AlgorithmType::Free)
    {
      lines.push_back("spills: " + std::to_string(result.selectedSpills.size()));
      for (int webId : result.selectedSpills)
        lines.push_back("spill: web" + std::to_string(webId));
    }

    if (params.algorithm == AlgorithmType::Splitting)
    {
      lines.push_back("splits: " + std::to_string(result.splitRecords.size()));
      for (const auto &record : result.splitRecords)
      {
        lines.push_back("split: web" + std::to_string(record.sourceWebId) +
                        " -> web" + std::to_string(record.leftWebId) +
                        ",web" + std::to_string(record.rightWebId));
      }
    }

    return lines;
  }

  std::string dotEscape(const std::string &text)
  {
    std::string escaped;
    escaped.reserve(text.size());
    for (char c : text)
    {
      if (c == '"' || c == '\\')
        escaped.push_back('\\');
      escaped.push_back(c);
    }
    return escaped;
  }

  std::string formatWebLinesForLabel(const Web &web)
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
        oss << ",";

      char marker = '\0';
      const bool hasPlus = markers.count('+') > 0;
      const bool hasMinus = markers.count('-') > 0;
      if (hasPlus && !hasMinus)
        marker = '+';
      else if (hasMinus && !hasPlus)
        marker = '-';

      oss << line;
      if (marker != '\0')
        oss << marker;
      first = false;
    }
    return oss.str();
  }

  std::string colorForRegister(int reg)
  {
    static const std::vector<std::string> palette = {
        "#1982d2", "#44b86f", "#f3a600", "#d67c3b",
        "#7661b3", "#46aaa5", "#cc99cd", "#98c0e3"};
    if (reg < 0)
      return "#38393b";
    return palette[static_cast<size_t>(reg) % palette.size()];
  }
} // namespace

// ---------------------------------------------------------------------------
// AllocationLogic::runAllocation
// ---------------------------------------------------------------------------
AllocationResult AllocationLogic::runAllocation(std::vector<Web> &webs,
                                                Graph<int> &graph,
                                                const Parameters &params)
{
  AllocationResult result;

  try
  {
    switch (params.algorithm)
    {
    case AlgorithmType::Basic:
      result = GraphColoring::basicColoring(graph, webs, params.numRegisters);
      break;

    case AlgorithmType::Spilling:
      result = GraphColoring::spillingColoring(
          graph, webs, params.numRegisters, params.algorithmParam);
      break;

    case AlgorithmType::Splitting:
      result = GraphColoring::splittingColoring(
          graph, webs, params.numRegisters, params.algorithmParam);
      break;

    case AlgorithmType::Free:
      result = GraphColoring::freeColoring(graph, webs, params.numRegisters);
      break;
    }
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << "Algorithm error: " << e.what() << "\n";
    // Return an "all spilled" result so the rest of the pipeline can still run.
    for (const auto &web : webs)
      result.webToRegister[web.id] = -1;
    result.feasible = false;
    result.registersUsed = 0;
  }

  if (!result.feasible)
  {
    std::cerr << "Warning: register allocation was not feasible with "
              << params.numRegisters << " register(s). All webs will be assigned to memory.\n";

    // Infeasible basic allocations are represented as an all-memory result.
    result.webToRegister.clear();
    for (const auto &web : webs)
      result.webToRegister[web.id] = -1;
    result.registersUsed = 0;
    result.spilledWebs = static_cast<int>(webs.size());
  }

  // Write output file.
  if (!params.outputFile.empty())
  {
    OutputWriter::write(params.outputFile, webs, result.webToRegister,
                        buildMetadataLines(result, params));
    std::cout << "Allocation written to '" << params.outputFile << "'.\n";
  }

  return result;
}

// ---------------------------------------------------------------------------
// AllocationLogic::printResult
// ---------------------------------------------------------------------------
void AllocationLogic::printResult(const std::vector<Web> &webs,
                                  const AllocationResult &result)
{
  std::cout << "\n--- Allocation Result ---\n";
  std::cout << "Feasible      : " << (result.feasible ? "Yes" : "No") << "\n";
  std::cout << "Registers used: " << result.registersUsed << "\n\n";
  std::cout << "Spilled webs  : " << result.spilledWebs << "\n\n";

  for (const auto &web : webs)
  {
    auto it = result.webToRegister.find(web.id);
    int reg = (it != result.webToRegister.end()) ? it->second : -1;

    std::string regStr = (reg >= 0) ? ("r" + std::to_string(reg)) : "M (memory)";
    std::cout << "  web" << web.id << " [" << web.variable << "] -> " << regStr << "\n";
    for (const auto &range : web.ranges)
    {
      std::cout << "    range: ";
      for (size_t i = 0; i < range.points.size(); ++i)
      {
        if (i > 0)
          std::cout << ',';
        std::cout << range.points[i].line;
        if (range.points[i].marker != '\0')
          std::cout << range.points[i].marker;
      }
      std::cout << "\n";
    }
  }
  std::cout << "-------------------------\n";
}

// ---------------------------------------------------------------------------
// AllocationLogic::exportAllocationDOT
// ---------------------------------------------------------------------------
bool AllocationLogic::exportAllocationDOT(const Graph<int> &graph,
                                          const std::vector<Web> &webs,
                                          const AllocationResult &result,
                                          const std::string &filename)
{
  std::ofstream out(filename);
  if (!out.is_open())
  {
    std::cerr << "Error: cannot write allocation DOT file '" << filename << "'.\n";
    return false;
  }

  std::set<int> splitDerived;
  for (const auto &record : result.splitRecords)
  {
    splitDerived.insert(record.leftWebId);
    splitDerived.insert(record.rightWebId);
  }

  out << "graph allocation {\n";
  out << "    graph [bgcolor=\"transparent\", color=\"#38393b\", rankdir=LR, "
         "fontname=\"DejaVu Sans\", fontcolor=\"#d2dbde\", labelloc=t, "
         "label=\"Colored Register Allocation\"];\n";
  out << "    node [shape=ellipse, style=\"filled,bold\", fontname=\"DejaVu Sans\", "
         "fontcolor=\"#f8f9fa\", color=\"#859399\", penwidth=1.8];\n";
  out << "    edge [color=\"#859399\", fontcolor=\"#d2dbde\", penwidth=1.3];\n\n";

  for (const auto &web : webs)
  {
    auto it = result.webToRegister.find(web.id);
    const int reg = (it == result.webToRegister.end()) ? -1 : it->second;
    const bool memory = reg < 0;
    const bool split = splitDerived.count(web.id) > 0;

    std::string label = "web" + std::to_string(web.id) + "\\n" +
                        dotEscape(web.variable) + "\\n" +
                        (memory ? "M" : "r" + std::to_string(reg)) + "\\n" +
                        dotEscape(formatWebLinesForLabel(web));

    out << "    web" << web.id
        << " [label=\"" << label << "\""
        << ", fillcolor=\"" << colorForRegister(reg) << "\""
        << ", color=\"" << (split ? "#ff8800" : (memory ? "#859399" : "#d2dbde")) << "\""
        << ", shape=" << (memory ? "box" : "ellipse")
        << ", penwidth=" << (split ? "3.0" : "1.8")
        << "];\n";
  }

  std::set<std::pair<int, int>> printed;
  for (const auto &web : webs)
  {
    Vertex<int> *vertex = graph.findVertex(web.id);
    if (!vertex)
      continue;
    for (const auto *edge : vertex->getAdj())
    {
      int u = web.id;
      int v = edge->getDest()->getInfo();
      if (u > v)
        std::swap(u, v);
      if (printed.insert({u, v}).second)
        out << "    web" << u << " -- web" << v << ";\n";
    }
  }

  out << "\n";
  out << "    subgraph cluster_legend {\n";
  out << "        label=\"Legend\";\n";
  out << "        fontcolor=\"#d2dbde\";\n";
  out << "        color=\"#38393b\";\n";
  out << "        style=\"rounded,filled\";\n";
  out << "        fillcolor=\"#252628\";\n";
  out << "        key_reg [label=\"register color\", fillcolor=\"#1982d2\", shape=ellipse];\n";
  out << "        key_mem [label=\"M = memory\", fillcolor=\"#38393b\", shape=box, color=\"#859399\"];\n";
  out << "        key_split [label=\"split-derived web\", fillcolor=\"#252628\", color=\"#ff8800\", penwidth=3.0];\n";
  out << "    }\n";

  if (!result.selectedSpills.empty() || !result.splitRecords.empty())
  {
    out << "\n";
    out << "    note [shape=note, fillcolor=\"#252628\", color=\"#ff8800\", fontcolor=\"#d2dbde\", label=\"";
    bool first = true;
    if (!result.selectedSpills.empty())
    {
      out << "spills: ";
      for (int id : result.selectedSpills)
      {
        if (!first)
          out << ", ";
        out << "web" << id;
        first = false;
      }
    }
    if (!result.splitRecords.empty())
    {
      if (!first)
        out << "\\n";
      out << "splits: ";
      for (size_t i = 0; i < result.splitRecords.size(); ++i)
      {
        if (i > 0)
          out << "; ";
        const auto &r = result.splitRecords[i];
        out << "web" << r.sourceWebId << " -> web" << r.leftWebId << ",web" << r.rightWebId;
      }
    }
    out << "\"];\n";
  }

  out << "}\n";
  std::cout << "Colored allocation DOT exported to '" << filename << "'.\n";
  return true;
}

// ---------------------------------------------------------------------------
// AllocationLogic::estimateChromatic
// ---------------------------------------------------------------------------
int AllocationLogic::estimateChromatic(const Graph<int> &graph,
                                       const std::vector<Web> &webs)
{
  // Greedy clique lower bound: start with the highest-degree vertex and greedily
  // extend the clique by adding neighbors that are adjacent to all current members.
  if (webs.empty())
    return 0;

  // Find the web with the highest degree as seed.
  int seedId = webs.front().id;
  int maxDeg = -1;
  for (const auto &web : webs)
  {
    Vertex<int> *v = graph.findVertex(web.id);
    if (!v)
      continue;
    int deg = static_cast<int>(v->getAdj().size());
    if (deg > maxDeg)
    {
      maxDeg = deg;
      seedId = web.id;
    }
  }

  std::vector<int> clique = {seedId};

  // Candidate set: neighbors of the seed.
  Vertex<int> *sv = graph.findVertex(seedId);
  if (!sv)
    return 1;

  std::vector<int> candidates;
  for (const auto *e : sv->getAdj())
    candidates.push_back(e->getDest()->getInfo());

  for (int c : candidates)
  {
    // Check that c is adjacent to all current clique members.
    Vertex<int> *cv = graph.findVertex(c);
    if (!cv)
      continue;

    bool adjToAll = true;
    for (int m : clique)
    {
      bool found = false;
      for (const auto *e : cv->getAdj())
      {
        if (e->getDest()->getInfo() == m)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        adjToAll = false;
        break;
      }
    }
    if (adjToAll)
      clique.push_back(c);
  }

  return static_cast<int>(clique.size());
}
