/**
 * @file InterferenceGraph.cpp
 * @brief Implementation of web construction and interference graph building.
 */

#include "data_structures/InterferenceGraph.h"

#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Union-Find helpers (local to this translation unit)
// ---------------------------------------------------------------------------
namespace
{

  struct UnionFind
  {
    std::vector<int> parent, rank_;

    explicit UnionFind(int n) : parent(n), rank_(n, 0)
    {
      for (int i = 0; i < n; ++i)
        parent[i] = i;
    }

    int find(int x)
    {
      if (parent[x] != x)
        parent[x] = find(parent[x]);
      return parent[x];
    }

    void unite(int a, int b)
    {
      a = find(a);
      b = find(b);
      if (a == b)
        return;
      if (rank_[a] < rank_[b])
        std::swap(a, b);
      parent[b] = a;
      if (rank_[a] == rank_[b])
        ++rank_[a];
    }
  };

  /// Returns true if two LiveRange objects share at least one line number.
  bool rangesOverlap(const LiveRange &a, const LiveRange &b)
  {
    std::set<int> linesA;
    for (const auto &p : a.points)
      linesA.insert(p.line);
    for (const auto &p : b.points)
      if (linesA.count(p.line))
        return true;
    return false;
  }

  int firstLineOfRange(const LiveRange &range)
  {
    int best = std::numeric_limits<int>::max();
    for (const auto &point : range.points)
      best = std::min(best, point.line);
    return best;
  }

  int firstLineOfWeb(const Web &web)
  {
    int best = std::numeric_limits<int>::max();
    for (const auto &range : web.ranges)
      best = std::min(best, firstLineOfRange(range));
    return best;
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

  std::string formatWebPointsForLabel(const Web &web)
  {
    std::map<int, std::set<char>> markersByLine;
    for (const auto &range : web.ranges)
      for (const auto &point : range.points)
        markersByLine[point.line].insert(point.marker);

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

} // anonymous namespace

// ---------------------------------------------------------------------------
// InterferenceGraph::buildWebs
// ---------------------------------------------------------------------------
std::vector<Web> InterferenceGraph::buildWebs(
    const std::map<std::string, std::vector<LiveRange>> &rawRanges)
{
  std::vector<Web> webs;
  int nextId = 0;

  for (const auto &[varName, ranges] : rawRanges)
  {
    const auto &varRanges = ranges;
    int n = static_cast<int>(varRanges.size());
    UnionFind uf(n);

    // Merge ranges that share at least one line number.
    for (int i = 0; i < n; ++i)
      for (int j = i + 1; j < n; ++j)
        if (rangesOverlap(varRanges[i], varRanges[j]))
          uf.unite(i, j);

    // Group ranges by their root component.
    std::map<int, std::vector<int>> components;
    for (int i = 0; i < n; ++i)
      components[uf.find(i)].push_back(i);

    for (auto &[root, indices] : components)
    {
      Web web;
      web.id = nextId++;
      web.variable = varName;

      std::sort(indices.begin(), indices.end(),
                [&](int lhs, int rhs)
                {
                  return firstLineOfRange(varRanges[lhs]) < firstLineOfRange(varRanges[rhs]);
                });

      for (int idx : indices)
        web.ranges.push_back(varRanges[idx]);
      webs.push_back(std::move(web));
    }
  }

  std::sort(webs.begin(), webs.end(),
            [](const Web &lhs, const Web &rhs)
            {
              if (lhs.variable != rhs.variable)
                return lhs.variable < rhs.variable;
              if (firstLineOfWeb(lhs) != firstLineOfWeb(rhs))
                return firstLineOfWeb(lhs) < firstLineOfWeb(rhs);
              return lhs.id < rhs.id;
            });

  for (size_t i = 0; i < webs.size(); ++i)
    webs[i].id = static_cast<int>(i);

  return webs;
}

// ---------------------------------------------------------------------------
// InterferenceGraph::interferes
// ---------------------------------------------------------------------------
bool InterferenceGraph::interferes(const Web &a, const Web &b)
{
  // Build a map: line -> set of markers for web a, and web b.
  std::unordered_map<int, std::set<char>> markersA, markersB;

  for (const auto &r : a.ranges)
    for (const auto &p : r.points)
      markersA[p.line].insert(p.marker);

  for (const auto &r : b.ranges)
    for (const auto &p : r.points)
      markersB[p.line].insert(p.marker);

  // Check every line in web a against web b.
  for (const auto &[line, mA] : markersA)
  {
    auto it = markersB.find(line);
    if (it == markersB.end())
      continue; // not in b at all

    const auto &mB = it->second;

    // Non-interference rule: if a ends here (only '-') and b starts here (only '+'),
    // or vice-versa, they do not interfere at this line.
    bool aOnlyEnd = (mA.size() == 1 && mA.count('-'));
    bool aOnlyStart = (mA.size() == 1 && mA.count('+'));
    bool bOnlyEnd = (mB.size() == 1 && mB.count('-'));
    bool bOnlyStart = (mB.size() == 1 && mB.count('+'));

    if ((aOnlyEnd && bOnlyStart) || (aOnlyStart && bOnlyEnd))
    {
      // Definition meets last-use at the same instruction: no interference.
      continue;
    }

    // Any other overlap is an interference.
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// InterferenceGraph::buildGraph
// ---------------------------------------------------------------------------
Graph<int> InterferenceGraph::buildGraph(const std::vector<Web> &webs)
{
  Graph<int> graph;

  // Add one vertex per web.
  for (const auto &web : webs)
    graph.addVertex(web.id);

  // Add undirected edges between interfering webs.
  for (size_t i = 0; i < webs.size(); ++i)
  {
    for (size_t j = i + 1; j < webs.size(); ++j)
    {
      if (interferes(webs[i], webs[j]))
      {
        graph.addBidirectionalEdge(webs[i].id, webs[j].id, 1.0);
      }
    }
  }

  return graph;
}

// ---------------------------------------------------------------------------
// InterferenceGraph::printGraph
// ---------------------------------------------------------------------------
void InterferenceGraph::printGraph(const Graph<int> &graph, const std::vector<Web> &webs)
{
  std::cout << "Interference Graph (" << graph.getNumVertex() << " webs):\n";
  for (const auto &web : webs)
  {
    Vertex<int> *v = graph.findVertex(web.id);
    if (!v)
      continue;
    std::cout << "  web" << web.id << " (" << web.variable << ") --> [";
    bool first = true;
    for (const auto *e : v->getAdj())
    {
      if (!first)
        std::cout << ", ";
      std::cout << "web" << e->getDest()->getInfo();
      first = false;
    }
    std::cout << "]\n";
  }
}

// ---------------------------------------------------------------------------
// InterferenceGraph::exportDOT
// ---------------------------------------------------------------------------
void InterferenceGraph::exportDOT(const Graph<int> &graph,
                                  const std::vector<Web> &webs,
                                  const std::string &filename)
{
  std::ofstream out(filename);
  if (!out.is_open())
  {
    std::cerr << "Error: cannot write DOT file '" << filename << "'.\n";
    return;
  }

  out << "graph interference {\n";
  out << "    graph [bgcolor=\"transparent\", color=\"#38393b\", rankdir=LR, "
         "fontname=\"DejaVu Sans\", fontcolor=\"#d2dbde\", labelloc=t, "
         "label=\"Interference Graph\"];\n";
  out << "    node [shape=ellipse, style=\"filled,bold\", fontname=\"DejaVu Sans\", "
         "fontcolor=\"#f8f9fa\", fillcolor=\"#252628\", color=\"#859399\", penwidth=1.8];\n";
  out << "    edge [color=\"#859399\", penwidth=1.4];\n\n";

  // Vertices with labels
  for (const auto &web : webs)
  {
    Vertex<int> *vertex = graph.findVertex(web.id);
    const int degree = vertex ? static_cast<int>(vertex->getAdj().size()) : 0;
    const std::string label = "web" + std::to_string(web.id) + "\\n" +
                              dotEscape(web.variable) + "\\n" +
                              "deg=" + std::to_string(degree) + "\\n" +
                              dotEscape(formatWebPointsForLabel(web));
    out << "    web" << web.id
        << " [label=\"" << label << "\"];\n";
  }

  // Edges (undirected – print each pair once)
  std::set<std::pair<int, int>> printed;
  for (const auto &web : webs)
  {
    Vertex<int> *v = graph.findVertex(web.id);
    if (!v)
      continue;
    for (const auto *e : v->getAdj())
    {
      int u = web.id;
      int w = e->getDest()->getInfo();
      if (u > w)
        std::swap(u, w);
      if (printed.insert({u, w}).second)
        out << "    web" << u << " -- web" << w << ";\n";
    }
  }

  out << "\n";
  out << "    subgraph cluster_legend {\n";
  out << "        label=\"Legend\";\n";
  out << "        fontcolor=\"#d2dbde\";\n";
  out << "        color=\"#38393b\";\n";
  out << "        style=\"rounded,filled\";\n";
  out << "        fillcolor=\"#252628\";\n";
  out << "        key_node [label=\"web\\nvariable\\ndegree\\npoints\", fillcolor=\"#252628\"];\n";
  out << "        key_edge [label=\"edge = cannot share a register\", shape=note, fillcolor=\"#38393b\"];\n";
  out << "    }\n";

  out << "}\n";
  std::cout << "DOT graph exported to '" << filename << "'.\n";
}
