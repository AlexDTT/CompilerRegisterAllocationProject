/**
 * @file AllocationLogic.cpp
 * @brief Implementation of the AllocationLogic orchestration layer.
 */

#include "services/AllocationLogic.h"

#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "io/OutputWriter.h"

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

    // Specification compliance: infeasible allocation must output registers: 0
    // and assign all webs to memory.
    result.webToRegister.clear();
    for (const auto &web : webs)
      result.webToRegister[web.id] = -1;
    result.registersUsed = 0;
    result.spilledWebs = static_cast<int>(webs.size());
  }

  // Write output file.
  if (!params.outputFile.empty())
  {
    OutputWriter::write(params.outputFile, webs, result.webToRegister);
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
