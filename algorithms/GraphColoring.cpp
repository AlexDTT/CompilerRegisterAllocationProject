/**
 * @file GraphColoring.cpp
 * @brief Implementations of graph-coloring register allocation algorithms.
 */

#include "algorithms/GraphColoring.h"

#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <limits>

#include "data_structures/InterferenceGraph.h"

namespace
{
  std::set<int> collectWebIds(const std::vector<Web> &webs)
  {
    std::set<int> ids;
    for (const auto &w : webs)
      ids.insert(w.id);
    return ids;
  }

  int maxWebId(const std::vector<Web> &webs)
  {
    int best = -1;
    for (const auto &w : webs)
      best = std::max(best, w.id);
    return best;
  }

  bool colorsRespectInterference(const Graph<int> &graph,
                                 const std::map<int, int> &webToRegister)
  {
    for (const auto &[id, color] : webToRegister)
    {
      if (color < 0)
        continue;

      Vertex<int> *v = graph.findVertex(id);
      if (!v)
        continue;

      for (const auto *e : v->getAdj())
      {
        int nid = e->getDest()->getInfo();
        auto it = webToRegister.find(nid);
        if (it != webToRegister.end() && it->second >= 0 && it->second == color)
        {
          return false;
        }
      }
    }
    return true;
  }

  AllocationResult finalizeResult(const std::vector<Web> &webs,
                                  const std::map<int, int> &assignments,
                                  bool allowSpills)
  {
    AllocationResult res;
    std::set<int> used;
    int spillCount = 0;

    for (const auto &w : webs)
    {
      auto it = assignments.find(w.id);
      int c = (it == assignments.end()) ? -1 : it->second;
      res.webToRegister[w.id] = c;
      if (c >= 0)
        used.insert(c);
      else
        ++spillCount;
    }

    res.registersUsed = (int)used.size();
    res.spilledWebs = spillCount;
    res.feasible = allowSpills || spillCount == 0;
    return res;
  }

  // Try to color an active induced subgraph with K colors and no extra spills.
  // Returns true on success and writes color assignments into outAssignments.
  bool kColorSubgraph(const Graph<int> &graph,
                      int numRegisters,
                      const std::set<int> &activeIds,
                      std::map<int, int> &outAssignments)
  {
    if (activeIds.empty())
    {
      outAssignments.clear();
      return true;
    }
    if (numRegisters <= 0)
      return false;

    std::set<int> active = activeIds;
    std::vector<int> stack;

    while (!active.empty())
    {
      bool removedAny = false;
      std::vector<int> removable;
      for (int id : active)
      {
        int deg = 0;
        Vertex<int> *v = graph.findVertex(id);
        if (v)
        {
          for (const auto *e : v->getAdj())
          {
            if (active.count(e->getDest()->getInfo()))
              ++deg;
          }
        }
        if (deg < numRegisters)
          removable.push_back(id);
      }

      if (!removable.empty())
      {
        removedAny = true;
        for (int id : removable)
        {
          stack.push_back(id);
          active.erase(id);
        }
      }

      if (!removedAny)
      {
        // Not K-simplifiable without spilling.
        return false;
      }
    }

    outAssignments.clear();
    while (!stack.empty())
    {
      int id = stack.back();
      stack.pop_back();

      std::set<int> used;
      Vertex<int> *v = graph.findVertex(id);
      if (v)
      {
        for (const auto *e : v->getAdj())
        {
          int nid = e->getDest()->getInfo();
          auto it = outAssignments.find(nid);
          if (it != outAssignments.end() && it->second >= 0)
            used.insert(it->second);
        }
      }

      int chosen = -1;
      for (int c = 0; c < numRegisters; ++c)
      {
        if (!used.count(c))
        {
          chosen = c;
          break;
        }
      }
      if (chosen < 0)
        return false;
      outAssignments[id] = chosen;
    }
    return true;
  }

  // Split the selected web into two derived webs. Returns false if split is impossible.
  bool splitWebById(std::vector<Web> &webs, int webId)
  {
    int idx = -1;
    for (size_t i = 0; i < webs.size(); ++i)
    {
      if (webs[i].id == webId)
      {
        idx = (int)i;
        break;
      }
    }
    if (idx < 0)
      return false;

    const Web original = webs[idx];
    if (original.ranges.empty())
      return false;

    Web left;
    Web right;
    left.id = original.id;
    right.id = maxWebId(webs) + 1;
    left.variable = original.variable;
    right.variable = original.variable;

    if (original.ranges.size() >= 2)
    {
      std::vector<LiveRange> sortedRanges = original.ranges;
      std::sort(sortedRanges.begin(), sortedRanges.end(),
                [](const LiveRange &a, const LiveRange &b)
                {
                  int aLine = a.points.empty() ? std::numeric_limits<int>::max() : a.points.front().line;
                  int bLine = b.points.empty() ? std::numeric_limits<int>::max() : b.points.front().line;
                  return aLine < bLine;
                });

      size_t splitIndex = sortedRanges.size() / 2;
      for (size_t i = 0; i < sortedRanges.size(); ++i)
      {
        if (i < splitIndex)
          left.ranges.push_back(sortedRanges[i]);
        else
          right.ranges.push_back(sortedRanges[i]);
      }
      if (right.ranges.empty())
      {
        right.ranges.push_back(left.ranges.back());
        left.ranges.pop_back();
      }
    }
    else
    {
      // Single range: split the sequence of program points in two contiguous pieces.
      const LiveRange &r = original.ranges.front();
      if (r.points.size() < 2)
        return false;

      size_t mid = r.points.size() / 2;
      if (mid == 0 || mid >= r.points.size())
        return false;

      LiveRange a;
      LiveRange b;
      a.variable = original.variable;
      b.variable = original.variable;

      for (size_t i = 0; i < mid; ++i)
        a.points.push_back(r.points[i]);
      for (size_t i = mid; i < r.points.size(); ++i)
        b.points.push_back(r.points[i]);

      if (a.points.empty() || b.points.empty())
        return false;

      // Normalise endpoints to satisfy parser/output conventions.
      a.points.front().marker = '+';
      a.points.back().marker = '-';
      b.points.front().marker = '+';
      b.points.back().marker = '-';

      left.ranges.push_back(std::move(a));
      right.ranges.push_back(std::move(b));
    }

    if (left.ranges.empty() || right.ranges.empty())
      return false;

    webs[idx] = std::move(left);
    webs.push_back(std::move(right));
    return true;
  }

} // namespace

// ---------------------------------------------------------------------------
// activeDegree
// ---------------------------------------------------------------------------
int GraphColoring::activeDegree(const Graph<int> &graph, int id,
                                const std::set<int> &active)
{
  Vertex<int> *v = graph.findVertex(id);
  if (!v)
    return 0;
  int deg = 0;
  for (const auto *e : v->getAdj())
  {
    if (active.count(e->getDest()->getInfo()))
      ++deg;
  }
  return deg;
}

// ---------------------------------------------------------------------------
// pickSpillCandidate
// ---------------------------------------------------------------------------
int GraphColoring::pickSpillCandidate(const Graph<int> &graph,
                                      const std::set<int> &active)
{
  int bestId = *active.begin();
  int bestDeg = -1;
  for (int id : active)
  {
    int d = activeDegree(graph, id, active);
    if (d > bestDeg || (d == bestDeg && id < bestId))
    {
      bestDeg = d;
      bestId = id;
    }
  }
  return bestId;
}

// ---------------------------------------------------------------------------
// T2.1  basicColoring
// ---------------------------------------------------------------------------
AllocationResult GraphColoring::basicColoring(const Graph<int> &graph,
                                              const std::vector<Web> &webs,
                                              int numRegisters)
{
  std::map<int, int> assignment;
  std::set<int> spilled;
  for (const auto &w : webs)
    assignment[w.id] = -1;

  if (numRegisters <= 0)
    return finalizeResult(webs, assignment, false);

  std::set<int> active = collectWebIds(webs);
  std::vector<int> stack;

  // Phase 1: simplification + spill selection.
  while (!active.empty())
  {
    bool removedAny = false;
    std::vector<int> removable;

    for (int id : active)
    {
      int deg = activeDegree(graph, id, active);
      if (deg < numRegisters)
        removable.push_back(id);
    }

    if (!removable.empty())
    {
      removedAny = true;
      for (int id : removable)
      {
        stack.push_back(id);
        active.erase(id);
      }
    }

    if (!removedAny)
    {
      int spillId = pickSpillCandidate(graph, active);
      spilled.insert(spillId);
      assignment[spillId] = -1;
      active.erase(spillId);
    }
  }

  // Phase 2: select colors while rebuilding.
  while (!stack.empty())
  {
    int id = stack.back();
    stack.pop_back();

    if (spilled.count(id))
    {
      assignment[id] = -1;
      continue;
    }

    std::set<int> used;
    Vertex<int> *v = graph.findVertex(id);
    if (v)
    {
      for (const auto *e : v->getAdj())
      {
        int nid = e->getDest()->getInfo();
        auto it = assignment.find(nid);
        if (it != assignment.end() && it->second >= 0)
          used.insert(it->second);
      }
    }

    int chosen = -1;
    for (int c = 0; c < numRegisters; ++c)
    {
      if (!used.count(c))
      {
        chosen = c;
        break;
      }
    }

    if (chosen < 0)
    {
      spilled.insert(id);
      assignment[id] = -1;
    }
    else
    {
      assignment[id] = chosen;
    }
  }

  return finalizeResult(webs, assignment, false);
}

// ---------------------------------------------------------------------------
// T2.2  spillingColoring
// ---------------------------------------------------------------------------
AllocationResult GraphColoring::spillingColoring(const Graph<int> &graph,
                                                 const std::vector<Web> &webs,
                                                 int numRegisters,
                                                 int maxSpills)
{
  maxSpills = std::max(0, maxSpills);

  if (numRegisters <= 0)
  {
    std::map<int, int> allSpilled;
    for (const auto &w : webs)
      allSpilled[w.id] = -1;
    return finalizeResult(webs, allSpilled, true);
  }

  std::set<int> forcedSpills;

  for (int allowedSpills = 0; allowedSpills <= maxSpills; ++allowedSpills)
  {
    std::set<int> active = collectWebIds(webs);
    for (int sid : forcedSpills)
      active.erase(sid);

    std::map<int, int> subAssign;
    if (kColorSubgraph(graph, numRegisters, active, subAssign))
    {
      std::map<int, int> full;
      for (const auto &w : webs)
      {
        if (forcedSpills.count(w.id))
          full[w.id] = -1;
        else
          full[w.id] = subAssign[w.id];
      }
      AllocationResult res = finalizeResult(webs, full, true);
      if (colorsRespectInterference(graph, res.webToRegister))
        return res;
    }

    if (allowedSpills == maxSpills)
      break;

    std::set<int> candidates = collectWebIds(webs);
    for (int sid : forcedSpills)
      candidates.erase(sid);
    if (candidates.empty())
      break;

    int sid = pickSpillCandidate(graph, candidates);
    forcedSpills.insert(sid);
  }

  std::map<int, int> allSpilled;
  for (const auto &w : webs)
    allSpilled[w.id] = -1;
  return finalizeResult(webs, allSpilled, false);
}

// ---------------------------------------------------------------------------
// T2.3  splittingColoring
// ---------------------------------------------------------------------------
AllocationResult GraphColoring::splittingColoring(Graph<int> &graph,
                                                  std::vector<Web> &webs,
                                                  int numRegisters,
                                                  int maxSplits)
{
  maxSplits = std::max(0, maxSplits);

  std::map<int, int> completeAssignment;
  if (kColorSubgraph(graph, numRegisters, collectWebIds(webs), completeAssignment))
    return finalizeResult(webs, completeAssignment, false);

  AllocationResult current = basicColoring(graph, webs, numRegisters);
  if (current.feasible)
    return current;

  for (int splitCount = 0; splitCount < maxSplits; ++splitCount)
  {
    // Prefer splitting currently spilled webs; tie-break by highest degree.
    int bestId = -1;
    int bestDeg = -1;

    for (const auto &w : webs)
    {
      bool spilledNow = false;
      auto it = current.webToRegister.find(w.id);
      if (it != current.webToRegister.end() && it->second < 0)
        spilledNow = true;

      if (!spilledNow && bestId >= 0)
        continue;

      Vertex<int> *v = graph.findVertex(w.id);
      int deg = v ? (int)v->getAdj().size() : 0;

      if (bestId < 0 || deg > bestDeg)
      {
        bestId = w.id;
        bestDeg = deg;
      }
    }

    if (bestId < 0)
      break;

    if (!splitWebById(webs, bestId))
      break;

    graph = InterferenceGraph::buildGraph(webs);
    std::map<int, int> splitAssignment;
    if (kColorSubgraph(graph, numRegisters, collectWebIds(webs), splitAssignment))
      return finalizeResult(webs, splitAssignment, false);

    current = basicColoring(graph, webs, numRegisters);
  }

  return current;
}

// ---------------------------------------------------------------------------
// T2.4  freeColoring
// ---------------------------------------------------------------------------
AllocationResult GraphColoring::freeColoring(const Graph<int> &graph,
                                             const std::vector<Web> &webs,
                                             int numRegisters)
{
  std::map<int, int> assignment;
  for (const auto &w : webs)
    assignment[w.id] = -1;

  if (numRegisters <= 0)
    return finalizeResult(webs, assignment, true);

  std::set<int> uncolored = collectWebIds(webs);

  while (!uncolored.empty())
  {
    int bestId = -1;
    int bestSat = -1;
    int bestDeg = -1;

    for (int id : uncolored)
    {
      std::set<int> satColors;
      int deg = 0;

      Vertex<int> *v = graph.findVertex(id);
      if (v)
      {
        for (const auto *e : v->getAdj())
        {
          int nid = e->getDest()->getInfo();
          ++deg;
          auto it = assignment.find(nid);
          if (it != assignment.end() && it->second >= 0)
            satColors.insert(it->second);
        }
      }

      int sat = (int)satColors.size();
      if (bestId < 0 || sat > bestSat || (sat == bestSat && deg > bestDeg))
      {
        bestId = id;
        bestSat = sat;
        bestDeg = deg;
      }
    }

    std::set<int> used;
    Vertex<int> *v = graph.findVertex(bestId);
    if (v)
    {
      for (const auto *e : v->getAdj())
      {
        int nid = e->getDest()->getInfo();
        auto it = assignment.find(nid);
        if (it != assignment.end() && it->second >= 0)
          used.insert(it->second);
      }
    }

    int chosen = -1;
    for (int c = 0; c < numRegisters; ++c)
    {
      if (!used.count(c))
      {
        chosen = c;
        break;
      }
    }

    assignment[bestId] = chosen; // if chosen == -1 => spill
    uncolored.erase(bestId);
  }

  AllocationResult res = finalizeResult(webs, assignment, true);
  if (!colorsRespectInterference(graph, res.webToRegister))
  {
    std::cerr << "Internal warning: freeColoring produced conflicting colors; falling back to basicColoring.\n";
    return basicColoring(graph, webs, numRegisters);
  }
  return res;
}
