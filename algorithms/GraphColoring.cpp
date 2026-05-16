/**
 * @file GraphColoring.cpp
 * @brief Implementations of graph-coloring register allocation algorithms.
 */

#include "algorithms/GraphColoring.h"

#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <functional>
#include <limits>
#include <queue>

#include "data_structures/InterferenceGraph.h"

namespace
{
  // When the number of webs is <= this limit, freeColoringNoSplitting runs an exact
  // backtracking search (exactMinSpillColoring) to minimize spills, after
  // the faster DSATUR heuristic gives it a starting solution. Above this
  // cutoff the exact search is skipped; it is exponential and quickly
  // becomes impractical.
  constexpr size_t EXACT_FREE_SEARCH_LIMIT = 25;
  constexpr size_t EXACT_FREE_SPLIT_VERIFY_LIMIT = 18;

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

  std::vector<int> sortedWebIds(const std::vector<Web> &webs)
  {
    std::vector<int> ids;
    ids.reserve(webs.size());
    for (const auto &w : webs)
      ids.push_back(w.id);
    std::sort(ids.begin(), ids.end());
    return ids;
  }

  std::map<int, std::set<int>> buildAdjacency(const Graph<int> &graph,
                                              const std::vector<Web> &webs)
  {
    std::map<int, std::set<int>> adj;
    for (const auto &web : webs)
      adj[web.id];

    for (const auto &web : webs)
    {
      Vertex<int> *vertex = graph.findVertex(web.id);
      if (!vertex)
        continue;

      for (const auto *edge : vertex->getAdj())
      {
        int neighbor = edge->getDest()->getInfo();
        if (adj.count(neighbor))
          adj[web.id].insert(neighbor);
      }
    }
    return adj;
  }

  int edgeCount(const std::map<int, std::set<int>> &adj)
  {
    int directed = 0;
    for (const auto &[id, neighbors] : adj)
    {
      (void)id;
      directed += static_cast<int>(neighbors.size());
    }
    return directed / 2;
  }

  int maxDegree(const std::map<int, std::set<int>> &adj)
  {
    int best = 0;
    for (const auto &[id, neighbors] : adj)
    {
      (void)id;
      best = std::max(best, static_cast<int>(neighbors.size()));
    }
    return best;
  }

  bool isEdgeless(const std::map<int, std::set<int>> &adj)
  {
    return edgeCount(adj) == 0;
  }

  bool isCompleteGraph(const std::map<int, std::set<int>> &adj)
  {
    const int n = static_cast<int>(adj.size());
    if (n <= 1)
      return true;
    for (const auto &[id, neighbors] : adj)
    {
      (void)id;
      if (static_cast<int>(neighbors.size()) != n - 1)
        return false;
    }
    return true;
  }

  bool tryBipartiteColoring(const std::map<int, std::set<int>> &adj,
                            int numRegisters,
                            std::map<int, int> &assignment)
  {
    if (numRegisters < 2)
      return false;

    assignment.clear();
    for (const auto &[id, neighbors] : adj)
    {
      (void)neighbors;
      assignment[id] = -2;
    }

    for (const auto &[start, neighbors] : adj)
    {
      (void)neighbors;
      if (assignment[start] != -2)
        continue;

      assignment[start] = 0;
      std::queue<int> q;
      q.push(start);

      while (!q.empty())
      {
        int u = q.front();
        q.pop();
        for (int v : adj.at(u))
        {
          if (assignment[v] == -2)
          {
            assignment[v] = 1 - assignment[u];
            q.push(v);
          }
          else if (assignment[v] == assignment[u])
          {
            assignment.clear();
            return false;
          }
        }
      }
    }
    return true;
  }

  int countSpills(const std::map<int, int> &assignment)
  {
    int spills = 0;
    for (const auto &[id, reg] : assignment)
    {
      (void)id;
      if (reg < 0)
        ++spills;
    }
    return spills;
  }

  std::map<int, int> dsaturSpillHeuristic(const std::map<int, std::set<int>> &adj,
                                          const std::vector<int> &ids,
                                          int numRegisters)
  {
    std::map<int, int> assignment;
    for (int id : ids)
      assignment[id] = -2; // -2 means unassigned; -1 means intentionally spilled.

    std::set<int> uncolored(ids.begin(), ids.end());
    while (!uncolored.empty())
    {
      int bestId = -1;
      int bestSat = -1;
      int bestDegree = -1;
      int bestUncoloredDegree = -1;

      for (int id : uncolored)
      {
        std::set<int> satColors;
        int uncoloredDegree = 0;

        for (int neighbor : adj.at(id))
        {
          auto it = assignment.find(neighbor);
          if (it != assignment.end() && it->second >= 0)
            satColors.insert(it->second);
          else if (it != assignment.end() && it->second == -2)
            ++uncoloredDegree;
        }

        int sat = static_cast<int>(satColors.size());
        int degree = static_cast<int>(adj.at(id).size());
        if (bestId < 0 ||
            sat > bestSat ||
            (sat == bestSat && degree > bestDegree) ||
            (sat == bestSat && degree == bestDegree && uncoloredDegree > bestUncoloredDegree) ||
            (sat == bestSat && degree == bestDegree && uncoloredDegree == bestUncoloredDegree && id < bestId))
        {
          bestId = id;
          bestSat = sat;
          bestDegree = degree;
          bestUncoloredDegree = uncoloredDegree;
        }
      }

      std::set<int> used;
      for (int neighbor : adj.at(bestId))
      {
        auto it = assignment.find(neighbor);
        if (it != assignment.end() && it->second >= 0)
          used.insert(it->second);
      }

      int chosen = -1;
      for (int color = 0; color < numRegisters; ++color)
      {
        if (!used.count(color))
        {
          chosen = color;
          break;
        }
      }

      assignment[bestId] = chosen;
      uncolored.erase(bestId);
    }

    return assignment;
  }

  std::map<int, int> exactMinSpillColoring(const std::map<int, std::set<int>> &adj,
                                           const std::vector<int> &ids,
                                           int numRegisters,
                                           const std::map<int, int> &initial)
  {
    std::map<int, int> best = initial;
    int bestSpills = countSpills(best);

    std::map<int, int> partial;
    for (int id : ids)
      partial[id] = -2;

    auto chooseVertex = [&]() -> int
    {
      int bestId = -1;
      int bestSat = -1;
      int bestDegree = -1;
      int bestUncoloredDegree = -1;

      for (int id : ids)
      {
        if (partial[id] != -2)
          continue;

        std::set<int> satColors;
        int uncoloredDegree = 0;
        for (int neighbor : adj.at(id))
        {
          if (partial[neighbor] >= 0)
            satColors.insert(partial[neighbor]);
          else if (partial[neighbor] == -2)
            ++uncoloredDegree;
        }

        int sat = static_cast<int>(satColors.size());
        int degree = static_cast<int>(adj.at(id).size());
        if (bestId < 0 ||
            sat > bestSat ||
            (sat == bestSat && degree > bestDegree) ||
            (sat == bestSat && degree == bestDegree && uncoloredDegree > bestUncoloredDegree) ||
            (sat == bestSat && degree == bestDegree && uncoloredDegree == bestUncoloredDegree && id < bestId))
        {
          bestId = id;
          bestSat = sat;
          bestDegree = degree;
          bestUncoloredDegree = uncoloredDegree;
        }
      }
      return bestId;
    };

    std::function<void(int, int)> search = [&](int remaining, int spills)
    {
      if (spills >= bestSpills)
        return;
      if (remaining == 0)
      {
        best = partial;
        bestSpills = spills;
        return;
      }

      int id = chooseVertex();
      if (id < 0)
        return;

      std::set<int> blocked;
      for (int neighbor : adj.at(id))
      {
        if (partial[neighbor] >= 0)
          blocked.insert(partial[neighbor]);
      }

      for (int color = 0; color < numRegisters; ++color)
      {
        if (blocked.count(color))
          continue;

        partial[id] = color;
        search(remaining - 1, spills);
        if (bestSpills == 0)
        {
          partial[id] = -2;
          return;
        }
      }

      if (spills + 1 < bestSpills)
      {
        partial[id] = -1;
        search(remaining - 1, spills + 1);
      }
      partial[id] = -2;
    };

    search(static_cast<int>(ids.size()), 0);
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

  struct SplitChoice
  {
    int sourceIndex = -1;
    Web left;
    Web right;
    SplitRecord record;
    bool colorable = false;
    AllocationResult result;
    bool sourceStillSpilled = true;
    int edges = std::numeric_limits<int>::max();
    int maximumDegree = std::numeric_limits<int>::max();
  };

  std::vector<std::pair<Web, Web>> enumerateSplitsForWeb(const Web &original,
                                                         int rightWebId)
  {
    std::vector<std::pair<Web, Web>> candidates;
    if (original.ranges.empty())
      return candidates;

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

      for (size_t splitIndex = 1; splitIndex < sortedRanges.size(); ++splitIndex)
      {
        Web left;
        Web right;
        left.id = original.id;
        right.id = rightWebId;
        left.variable = original.variable;
        right.variable = original.variable;

        for (size_t i = 0; i < sortedRanges.size(); ++i)
        {
          if (i < splitIndex)
            left.ranges.push_back(sortedRanges[i]);
          else
            right.ranges.push_back(sortedRanges[i]);
        }

        candidates.push_back({std::move(left), std::move(right)});
      }
    }
    else
    {
      // Single range: split the sequence of program points in two contiguous pieces.
      // Markers are copied exactly.  A split boundary is not a real definition/use.
      const LiveRange &r = original.ranges.front();
      if (r.points.size() < 2)
        return candidates;

      for (size_t splitIndex = 1; splitIndex < r.points.size(); ++splitIndex)
      {
        LiveRange a;
        LiveRange b;
        a.variable = original.variable;
        b.variable = original.variable;

        for (size_t i = 0; i < splitIndex; ++i)
          a.points.push_back(r.points[i]);
        for (size_t i = splitIndex; i < r.points.size(); ++i)
          b.points.push_back(r.points[i]);

        Web left;
        Web right;
        left.id = original.id;
        right.id = rightWebId;
        left.variable = original.variable;
        right.variable = original.variable;
        left.ranges.push_back(std::move(a));
        right.ranges.push_back(std::move(b));
        candidates.push_back({std::move(left), std::move(right)});
      }
    }

    return candidates;
  }

  bool splitChoiceIsBetter(const SplitChoice &candidate,
                           const SplitChoice &best)
  {
    if (best.sourceIndex < 0)
      return true;
    if (candidate.colorable != best.colorable)
      return candidate.colorable;
    if (candidate.edges != best.edges)
      return candidate.edges < best.edges;
    if (candidate.maximumDegree != best.maximumDegree)
      return candidate.maximumDegree < best.maximumDegree;
    return candidate.record.sourceWebId < best.record.sourceWebId;
  }

  bool chooseBestSplit(const std::vector<Web> &webs,
                       int numRegisters,
                       SplitChoice &best)
  {
    const int rightWebId = maxWebId(webs) + 1;

    for (size_t index = 0; index < webs.size(); ++index)
    {
      const Web &source = webs[index];
      auto candidates = enumerateSplitsForWeb(source, rightWebId);

      for (auto &[left, right] : candidates)
      {
        if (InterferenceGraph::interferes(left, right))
          continue;

        std::vector<Web> trial = webs;
        trial[index] = left;
        trial.push_back(right);

        Graph<int> trialGraph = InterferenceGraph::buildGraph(trial);
        std::map<int, int> assignment;
        const bool colorable = kColorSubgraph(
            trialGraph, numRegisters, collectWebIds(trial), assignment);
        const auto adj = buildAdjacency(trialGraph, trial);

        SplitChoice candidate;
        candidate.sourceIndex = static_cast<int>(index);
        candidate.left = left;
        candidate.right = right;
        candidate.record = {source.id, left.id, right.id};
        candidate.colorable = colorable;
        candidate.edges = edgeCount(adj);
        candidate.maximumDegree = maxDegree(adj);

        if (splitChoiceIsBetter(candidate, best))
          best = std::move(candidate);
      }
    }

    return best.sourceIndex >= 0;
  }

  bool splitImprovesFreeResult(const AllocationResult &candidate,
                               const AllocationResult &current)
  {
    if (candidate.spilledWebs != current.spilledWebs)
      return candidate.spilledWebs < current.spilledWebs;
    if (candidate.registersUsed != current.registersUsed)
      return candidate.registersUsed < current.registersUsed;
    return false;
  }

  bool splitIsWorthVerifying(const AllocationResult &candidate,
                             const AllocationResult &current)
  {
    if (candidate.spilledWebs != current.spilledWebs)
      return candidate.spilledWebs < current.spilledWebs;
    return candidate.registersUsed <= current.registersUsed;
  }

  bool freeSplitChoiceIsBetter(const SplitChoice &candidate,
                               const SplitChoice &best)
  {
    if (best.sourceIndex < 0)
      return true;
    if (candidate.result.spilledWebs != best.result.spilledWebs)
      return candidate.result.spilledWebs < best.result.spilledWebs;
    if (candidate.result.registersUsed != best.result.registersUsed)
      return candidate.result.registersUsed < best.result.registersUsed;
    if (candidate.sourceStillSpilled != best.sourceStillSpilled)
      return !candidate.sourceStillSpilled;
    if (candidate.edges != best.edges)
      return candidate.edges < best.edges;
    if (candidate.maximumDegree != best.maximumDegree)
      return candidate.maximumDegree < best.maximumDegree;
    return candidate.record.sourceWebId < best.record.sourceWebId;
  }

  std::vector<size_t> freeSplitSourceIndices(const std::vector<Web> &webs,
                                             const Graph<int> &graph,
                                             const AllocationResult &current)
  {
    constexpr size_t SOURCE_LIMIT = 8;
    const auto adj = buildAdjacency(graph, webs);

    std::map<int, size_t> indexById;
    for (size_t i = 0; i < webs.size(); ++i)
      indexById[webs[i].id] = i;

    std::vector<int> sourceIds;
    std::set<int> seen;
    auto addId = [&](int id)
    {
      if (indexById.count(id) && seen.insert(id).second)
        sourceIds.push_back(id);
    };

    for (int id : current.selectedSpills)
      addId(id);

    std::vector<int> neighbors;
    for (int id : current.selectedSpills)
    {
      auto it = adj.find(id);
      if (it == adj.end())
        continue;
      for (int neighbor : it->second)
        neighbors.push_back(neighbor);
    }
    std::sort(neighbors.begin(), neighbors.end(),
              [&](int a, int b)
              {
                const int da = adj.count(a) ? static_cast<int>(adj.at(a).size()) : 0;
                const int db = adj.count(b) ? static_cast<int>(adj.at(b).size()) : 0;
                if (da != db)
                  return da > db;
                return a < b;
              });
    for (int id : neighbors)
      addId(id);

    if (sourceIds.empty())
    {
      std::vector<int> byDegree;
      for (const auto &[id, neighborsForId] : adj)
      {
        (void)neighborsForId;
        byDegree.push_back(id);
      }
      std::sort(byDegree.begin(), byDegree.end(),
                [&](int a, int b)
                {
                  const int da = adj.count(a) ? static_cast<int>(adj.at(a).size()) : 0;
                  const int db = adj.count(b) ? static_cast<int>(adj.at(b).size()) : 0;
                  if (da != db)
                    return da > db;
                  return a < b;
                });
      for (int id : byDegree)
        addId(id);
    }

    if (sourceIds.size() > SOURCE_LIMIT)
      sourceIds.resize(SOURCE_LIMIT);

    std::vector<size_t> indices;
    indices.reserve(sourceIds.size());
    for (int id : sourceIds)
      indices.push_back(indexById.at(id));
    return indices;
  }

  AllocationResult freeColoringNoSplittingImpl(const Graph<int> &graph,
                                               const std::vector<Web> &webs,
                                               int numRegisters,
                                               bool runExactSearch);

  bool chooseBestFreeSplit(const Graph<int> &graph,
                           const std::vector<Web> &webs,
                           int numRegisters,
                           const AllocationResult &current,
                           SplitChoice &best)
  {
    const int rightWebId = maxWebId(webs) + 1;
    const std::vector<size_t> sourceIndices = freeSplitSourceIndices(webs, graph, current);

    for (size_t index : sourceIndices)
    {
      const Web &source = webs[index];
      auto candidates = enumerateSplitsForWeb(source, rightWebId);

      for (auto &[left, right] : candidates)
      {
        if (InterferenceGraph::interferes(left, right))
          continue;

        std::vector<Web> trial = webs;
        trial[index] = left;
        trial.push_back(right);

        Graph<int> trialGraph = InterferenceGraph::buildGraph(trial);
        AllocationResult trialResult =
            freeColoringNoSplittingImpl(trialGraph, trial, numRegisters, false);
        if (!splitIsWorthVerifying(trialResult, current))
          continue;

        const auto adj = buildAdjacency(trialGraph, trial);

        SplitChoice candidate;
        candidate.sourceIndex = static_cast<int>(index);
        candidate.left = left;
        candidate.right = right;
        candidate.record = {source.id, left.id, right.id};
        candidate.result = std::move(trialResult);
        candidate.sourceStillSpilled = candidate.result.webToRegister[source.id] < 0;
        candidate.edges = edgeCount(adj);
        candidate.maximumDegree = maxDegree(adj);

        if (freeSplitChoiceIsBetter(candidate, best))
          best = std::move(candidate);
      }
    }

    if (best.sourceIndex < 0)
      return false;

    std::vector<Web> verified = webs;
    verified[static_cast<size_t>(best.sourceIndex)] = best.left;
    verified.push_back(best.right);
    Graph<int> verifiedGraph = InterferenceGraph::buildGraph(verified);
    const bool runExactVerifier = verified.size() <= EXACT_FREE_SPLIT_VERIFY_LIMIT;
    AllocationResult verifiedResult =
        freeColoringNoSplittingImpl(verifiedGraph, verified, numRegisters, runExactVerifier);
    if (!splitImprovesFreeResult(verifiedResult, current))
    {
      best = SplitChoice{};
      return false;
    }

    best.result = std::move(verifiedResult);
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
      {
        res.selectedSpills = forcedSpills;
        return res;
      }
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

  std::vector<SplitRecord> splitRecords;

  for (int splitCount = 0; splitCount < maxSplits; ++splitCount)
  {
    SplitChoice choice;
    if (!chooseBestSplit(webs, numRegisters, choice))
      break;

    webs[static_cast<size_t>(choice.sourceIndex)] = std::move(choice.left);
    webs.push_back(std::move(choice.right));
    splitRecords.push_back(choice.record);

    graph = InterferenceGraph::buildGraph(webs);
    std::map<int, int> splitAssignment;
    if (kColorSubgraph(graph, numRegisters, collectWebIds(webs), splitAssignment))
    {
      AllocationResult res = finalizeResult(webs, splitAssignment, false);
      res.splitRecords = splitRecords;
      return res;
    }

    current = basicColoring(graph, webs, numRegisters);
    current.splitRecords = splitRecords;
  }

  current.splitRecords = splitRecords;
  return current;
}

namespace
{
  AllocationResult freeColoringNoSplittingImpl(const Graph<int> &graph,
                                               const std::vector<Web> &webs,
                                               int numRegisters,
                                               bool runExactSearch)
{
  std::map<int, int> assignment;
  for (const auto &w : webs)
    assignment[w.id] = -1;

  if (numRegisters <= 0)
  {
    AllocationResult res = finalizeResult(webs, assignment, true);
    for (const auto &w : webs)
      res.selectedSpills.insert(w.id);
    return res;
  }

  const std::vector<int> ids = sortedWebIds(webs);
  const auto adj = buildAdjacency(graph, webs);

  if (isEdgeless(adj))
  {
    for (int id : ids)
      assignment[id] = 0;
  }
  else if (isCompleteGraph(adj))
  {
    for (size_t i = 0; i < ids.size(); ++i)
      assignment[ids[i]] = (static_cast<int>(i) < numRegisters) ? static_cast<int>(i) : -1;
  }
  else if (!tryBipartiteColoring(adj, numRegisters, assignment))
  {
    assignment = dsaturSpillHeuristic(adj, ids, numRegisters);
    if (runExactSearch && ids.size() <= EXACT_FREE_SEARCH_LIMIT)
      assignment = exactMinSpillColoring(adj, ids, numRegisters, assignment);
  }

  AllocationResult res = finalizeResult(webs, assignment, true);
  for (const auto &[id, reg] : res.webToRegister)
  {
    if (reg < 0)
      res.selectedSpills.insert(id);
  }
  if (!colorsRespectInterference(graph, res.webToRegister))
  {
    std::cerr << "Internal warning: freeColoringNoSplitting produced conflicting colors; spilling all webs.\n";
    for (const auto &w : webs)
      assignment[w.id] = -1;
    res = finalizeResult(webs, assignment, true);
    for (const auto &w : webs)
      res.selectedSpills.insert(w.id);
  }
  return res;
}
} // namespace

// ---------------------------------------------------------------------------
// T2.4  freeColoringNoSplitting
// ---------------------------------------------------------------------------
AllocationResult GraphColoring::freeColoringNoSplitting(const Graph<int> &graph,
                                                        const std::vector<Web> &webs,
                                                        int numRegisters)
{
  return freeColoringNoSplittingImpl(graph, webs, numRegisters, true);
}

// ---------------------------------------------------------------------------
// T2.4  freeColoringWithSplitting
// ---------------------------------------------------------------------------
AllocationResult GraphColoring::freeColoringWithSplitting(Graph<int> &graph,
                                                          std::vector<Web> &webs,
                                                          int numRegisters,
                                                          int maxSplits)
{
  maxSplits = std::max(0, maxSplits);

  AllocationResult best = freeColoringNoSplitting(graph, webs, numRegisters);
  if (best.spilledWebs == 0 || maxSplits == 0)
    return best;

  std::vector<SplitRecord> splitRecords;

  for (int splitCount = 0; splitCount < maxSplits; ++splitCount)
  {
    SplitChoice choice;
    if (!chooseBestFreeSplit(graph, webs, numRegisters, best, choice))
      break;

    webs[static_cast<size_t>(choice.sourceIndex)] = std::move(choice.left);
    webs.push_back(std::move(choice.right));
    splitRecords.push_back(choice.record);

    graph = InterferenceGraph::buildGraph(webs);
    best = std::move(choice.result);
    best.splitRecords = splitRecords;

    if (!colorsRespectInterference(graph, best.webToRegister))
    {
      best = freeColoringNoSplitting(graph, webs, numRegisters);
      best.splitRecords = splitRecords;
    }

    if (best.spilledWebs == 0)
      break;
  }

  best.splitRecords = splitRecords;
  return best;
}

AllocationResult GraphColoring::freeColoring(const Graph<int> &graph,
                                             const std::vector<Web> &webs,
                                             int numRegisters)
{
  return freeColoringNoSplitting(graph, webs, numRegisters);
}
