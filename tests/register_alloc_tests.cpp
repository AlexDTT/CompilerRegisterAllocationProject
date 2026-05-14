/**
 * @file register_alloc_tests.cpp
 * @brief Unit tests for the compiler register allocation project.
 *
 * The suite intentionally focuses on specification-critical behaviour:
 * parsing rules, deterministic web construction, interference handling,
 * output formatting, and the four allocation modes.
 */

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "algorithms/GraphColoring.h"
#include "data_structures/InterferenceGraph.h"
#include "io/FileParser.h"
#include "io/OutputWriter.h"
#include "models/LiveRange.h"
#include "models/Parameters.h"
#include "models/Web.h"
#include "services/AllocationLogic.h"

namespace fs = std::filesystem;

static int g_passed = 0;
static int g_failed = 0;

#define EXPECT_TRUE(expr)                                                                    \
  do                                                                                         \
  {                                                                                          \
    if (!(expr))                                                                             \
    {                                                                                        \
      std::cerr << "FAIL [" << __FILE__ << ":" << __LINE__ << "] " #expr "\n";             \
      ++g_failed;                                                                            \
    }                                                                                        \
    else                                                                                     \
    {                                                                                        \
      ++g_passed;                                                                            \
    }                                                                                        \
  } while (false)

#define EXPECT_EQ(a, b) EXPECT_TRUE((a) == (b))

static LiveRange makeLR(const std::string &var,
                        std::initializer_list<std::pair<int, char>> pts)
{
  LiveRange lr;
  lr.variable = var;
  for (auto [line, marker] : pts)
    lr.points.emplace_back(line, marker);
  return lr;
}

static fs::path tempPath(const std::string &name)
{
  return fs::temp_directory_path() / name;
}

static std::string readFile(const fs::path &path)
{
  std::ifstream in(path);
  std::ostringstream oss;
  oss << in.rdbuf();
  return oss.str();
}

class ScopedStreamRedirect
{
public:
  explicit ScopedStreamRedirect(std::ostream &stream)
      : mStream(stream), mOriginal(stream.rdbuf(mCapture.rdbuf())) {}

  ~ScopedStreamRedirect()
  {
    mStream.rdbuf(mOriginal);
  }

  std::string str() const
  {
    return mCapture.str();
  }

private:
  std::ostringstream mCapture;
  std::ostream &mStream;
  std::streambuf *mOriginal;
};

static bool colorsRespectInterference(const Graph<int> &graph,
                                      const std::map<int, int> &webToRegister)
{
  for (const auto &[id, color] : webToRegister)
  {
    if (color < 0)
      continue;

    Vertex<int> *vertex = graph.findVertex(id);
    EXPECT_TRUE(vertex != nullptr);
    if (!vertex)
      return false;

    for (const auto *edge : vertex->getAdj())
    {
      int neighbor = edge->getDest()->getInfo();
      auto it = webToRegister.find(neighbor);
      if (it != webToRegister.end() && it->second >= 0 && it->second == color)
        return false;
    }
  }
  return true;
}

static std::map<std::string, std::vector<LiveRange>> makeRanges1LikeInput()
{
  std::map<std::string, std::vector<LiveRange>> raw;
  raw["sum"].push_back(makeLR("sum", {{7, '+'}, {8, '\0'}, {9, '\0'}, {10, '-'}}));
  raw["i"].push_back(makeLR("i", {{1, '+'}, {2, '\0'}, {3, '\0'}, {4, '\0'}, {5, '\0'}, {6, '-'}}));
  raw["i"].push_back(makeLR("i", {{9, '+'}, {10, '\0'}, {11, '\0'}, {12, '-'}}));
  raw["i"].push_back(makeLR("i", {{12, '+'}, {13, '\0'}, {14, '-'}}));
  raw["i"].push_back(makeLR("i", {{20, '+'}, {11, '\0'}, {12, '-'}}));
  return raw;
}

static void test_parseConfig_variants()
{
  {
    const fs::path path = tempPath("ra_cfg_basic.txt");
    std::ofstream out(path);
    out << "registers: 3\nalgorithm: basic\n";
    out.close();

    Parameters params;
    EXPECT_TRUE(FileParser::parseConfig(path.string(), params));
    EXPECT_EQ(params.numRegisters, 3);
    EXPECT_EQ(params.algorithm, AlgorithmType::Basic);
    EXPECT_EQ(params.algorithmParam, 0);
    std::remove(path.c_str());
  }

  {
    const fs::path path = tempPath("ra_cfg_spilling.txt");
    std::ofstream out(path);
    out << "registers: 2\nalgorithm: spilling, 1\n";
    out.close();

    Parameters params;
    EXPECT_TRUE(FileParser::parseConfig(path.string(), params));
    EXPECT_EQ(params.algorithm, AlgorithmType::Spilling);
    EXPECT_EQ(params.algorithmParam, 1);
    std::remove(path.c_str());
  }

  {
    const fs::path path = tempPath("ra_cfg_invalid.txt");
    std::ofstream out(path);
    out << "registers: 2\nalgorithm: basic, 1\n";
    out.close();

    Parameters params;
    ScopedStreamRedirect suppress(std::cerr);
    EXPECT_TRUE(!FileParser::parseConfig(path.string(), params));
    std::remove(path.c_str());
  }
}

static void test_parseRanges_supports_intersection_only_lines()
{
  const fs::path path = tempPath("ra_ranges_ok.txt");
  {
    std::ofstream out(path);
    out << "# comment\n";
    out << "sum: 7+,8,9,10-\n";
    out << "i: 1+,2,3,4,7\n";
    out << "i: 7,8\n";
    out << "i: 8,9-\n";
  }

  std::map<std::string, std::vector<LiveRange>> ranges;
  EXPECT_TRUE(FileParser::parseRanges(path.string(), ranges));
  EXPECT_TRUE(ranges.count("i") == 1);
  EXPECT_EQ((int)ranges["i"].size(), 3);
  std::remove(path.c_str());
}

static void test_parseRanges_requires_plus_and_minus()
{
  const fs::path path = tempPath("ra_ranges_invalid.txt");
  {
    std::ofstream out(path);
    out << "a: 1+,2,3\n";
    out << "b: 4,5-\n";
  }

  std::map<std::string, std::vector<LiveRange>> ranges;
  ScopedStreamRedirect suppress(std::cerr);
  EXPECT_TRUE(!FileParser::parseRanges(path.string(), ranges));
  std::remove(path.c_str());
}

static void test_parseRanges_rejects_malformed_points()
{
  const fs::path path = tempPath("ra_ranges_malformed.txt");
  {
    std::ofstream out(path);
    out << "a: 1+,2x,3-\n";
    out << "b: 1+,2,2,3-\n";
  }

  std::map<std::string, std::vector<LiveRange>> ranges;
  ScopedStreamRedirect suppress(std::cerr);
  EXPECT_TRUE(!FileParser::parseRanges(path.string(), ranges));
  std::remove(path.c_str());
}

static void test_buildWebs_is_deterministic_and_merges_transitively()
{
  auto raw = makeRanges1LikeInput();
  const std::vector<Web> webs = InterferenceGraph::buildWebs(raw);

  EXPECT_EQ((int)webs.size(), 3);
  EXPECT_EQ(webs[0].id, 0);
  EXPECT_EQ(webs[0].variable, "i");
  EXPECT_EQ((int)webs[0].ranges.size(), 1);

  EXPECT_EQ(webs[1].id, 1);
  EXPECT_EQ(webs[1].variable, "i");
  EXPECT_EQ((int)webs[1].ranges.size(), 3);

  EXPECT_EQ(webs[2].id, 2);
  EXPECT_EQ(webs[2].variable, "sum");
}

static void test_interference_definition_meets_last_use_is_not_edge()
{
  Web a{0, "x", {makeLR("x", {{3, '+'}, {4, '\0'}, {5, '-'}})}};
  Web b{1, "y", {makeLR("y", {{5, '+'}, {6, '\0'}, {7, '-'}})}};
  EXPECT_TRUE(!InterferenceGraph::interferes(a, b));
}

static void test_outputWriter_aggregates_ranges_per_web()
{
  Web web;
  web.id = 0;
  web.variable = "i";
  web.ranges.push_back(makeLR("i", {{9, '+'}, {10, '\0'}, {11, '\0'}, {12, '-'}}));
  web.ranges.push_back(makeLR("i", {{20, '+'}, {11, '\0'}, {12, '-'}}));
  web.ranges.push_back(makeLR("i", {{12, '+'}, {13, '\0'}, {14, '-'}}));

  std::map<int, int> assignment = {{0, 1}};
  const fs::path outFile = tempPath("ra_output_writer.txt");
  EXPECT_TRUE(OutputWriter::write(outFile.string(), {web}, assignment,
                                  {"spills: 1", "spill: web3"}));

  const std::string content = readFile(outFile);
  EXPECT_TRUE(content.find("web0: 9+,10,11,12,13,14-,20+") != std::string::npos);
  EXPECT_TRUE(content.find("r1: web0") != std::string::npos);
  EXPECT_TRUE(content.find("spills: 1\nspill: web3") != std::string::npos);
  std::remove(outFile.c_str());
}

static void test_basicColoring_detects_infeasible_triangle()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web c{2, "c", {makeLR("c", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  std::vector<Web> webs = {a, b, c};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  AllocationResult result = GraphColoring::basicColoring(graph, webs, 2);
  EXPECT_TRUE(!result.feasible);
  EXPECT_TRUE(result.spilledWebs >= 1);
}

static void test_spillingColoring_succeeds_with_bounded_spill()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web c{2, "c", {makeLR("c", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  std::vector<Web> webs = {a, b, c};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  AllocationResult result = GraphColoring::spillingColoring(graph, webs, 2, 1);
  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.spilledWebs, 1);
  EXPECT_EQ(result.registersUsed, 2);
  EXPECT_TRUE(colorsRespectInterference(graph, result.webToRegister));
}

static void test_splittingColoring_succeeds_after_one_split()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {5, '\0'}, {6, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '\0'}, {4, '-'}})}};
  Web c{2, "c", {makeLR("c", {{3, '+'}, {4, '\0'}, {5, '\0'}, {6, '-'}})}};
  std::vector<Web> webs = {a, b, c};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  AllocationResult result = GraphColoring::splittingColoring(graph, webs, 2, 1);
  EXPECT_TRUE(result.feasible);
  EXPECT_EQ((int)webs.size(), 4);
  EXPECT_EQ(result.spilledWebs, 0);
  EXPECT_EQ(webs[0].ranges[0].points.back().marker, '\0');
  EXPECT_EQ(webs[3].ranges[0].points.front().marker, '\0');

  Graph<int> rebuilt = InterferenceGraph::buildGraph(webs);
  EXPECT_TRUE(colorsRespectInterference(rebuilt, result.webToRegister));
}

static void test_freeColoring_returns_valid_allocation_with_spill()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web c{2, "c", {makeLR("c", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  std::vector<Web> webs = {a, b, c};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  AllocationResult result = GraphColoring::freeColoring(graph, webs, 2);
  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.spilledWebs, 1);
  EXPECT_TRUE(colorsRespectInterference(graph, result.webToRegister));
}

static void test_freeColoring_recognizes_bipartite_graph()
{
  std::vector<Web> webs;
  for (int i = 0; i < 6; ++i)
  {
    std::string name = "v" + std::to_string(i);
    webs.push_back(Web{i, name, {makeLR(name, {{1, '+'}, {2, '-'}})}});
  }

  Graph<int> graph;
  for (int i = 0; i < 6; ++i)
    graph.addVertex(i);
  for (int left = 0; left < 3; ++left)
    for (int right = 3; right < 6; ++right)
      graph.addBidirectionalEdge(left, right, 1.0);

  AllocationResult result = GraphColoring::freeColoring(graph, webs, 2);
  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.spilledWebs, 0);
  EXPECT_EQ(result.registersUsed, 2);
  EXPECT_TRUE(colorsRespectInterference(graph, result.webToRegister));
}

static void test_allocationLogic_basic_infeasible_forces_all_memory()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web c{2, "c", {makeLR("c", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  std::vector<Web> webs = {a, b, c};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  Parameters params;
  params.numRegisters = 2;
  params.algorithm = AlgorithmType::Basic;
  params.outputFile.clear();

  ScopedStreamRedirect suppress(std::cerr);
  AllocationResult result = AllocationLogic::runAllocation(webs, graph, params);
  EXPECT_TRUE(!result.feasible);
  EXPECT_EQ(result.registersUsed, 0);
  EXPECT_EQ(result.spilledWebs, 3);
  EXPECT_EQ(result.webToRegister.at(0), -1);
  EXPECT_EQ(result.webToRegister.at(1), -1);
  EXPECT_EQ(result.webToRegister.at(2), -1);
}

static void test_allocationLogic_spilling_preserves_partial_memory_assignment()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web c{2, "c", {makeLR("c", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  std::vector<Web> webs = {a, b, c};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  Parameters params;
  params.numRegisters = 2;
  params.algorithm = AlgorithmType::Spilling;
  params.algorithmParam = 1;
  params.outputFile.clear();

  AllocationResult result = AllocationLogic::runAllocation(webs, graph, params);
  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.spilledWebs, 1);
  EXPECT_EQ(result.registersUsed, 2);
  EXPECT_TRUE(colorsRespectInterference(graph, result.webToRegister));
}

static void test_allocationLogic_exports_colored_dot()
{
  Web a{0, "a", {makeLR("a", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  Web b{1, "b", {makeLR("b", {{1, '+'}, {2, '\0'}, {3, '-'}})}};
  std::vector<Web> webs = {a, b};
  Graph<int> graph = InterferenceGraph::buildGraph(webs);

  AllocationResult result = GraphColoring::freeColoring(graph, webs, 1);
  const fs::path outFile = tempPath("ra_colored_allocation.dot");
  EXPECT_TRUE(AllocationLogic::exportAllocationDOT(graph, webs, result, outFile.string()));

  const std::string content = readFile(outFile);
  EXPECT_TRUE(content.find("graph allocation") != std::string::npos);
  EXPECT_TRUE(content.find("fillcolor=") != std::string::npos);
  EXPECT_TRUE(content.find("M = memory") != std::string::npos);
  std::remove(outFile.c_str());
}

int main()
{
  test_parseConfig_variants();
  test_parseRanges_supports_intersection_only_lines();
  test_parseRanges_requires_plus_and_minus();
  test_parseRanges_rejects_malformed_points();
  test_buildWebs_is_deterministic_and_merges_transitively();
  test_interference_definition_meets_last_use_is_not_edge();
  test_outputWriter_aggregates_ranges_per_web();
  test_basicColoring_detects_infeasible_triangle();
  test_spillingColoring_succeeds_with_bounded_spill();
  test_splittingColoring_succeeds_after_one_split();
  test_freeColoring_returns_valid_allocation_with_spill();
  test_freeColoring_recognizes_bipartite_graph();
  test_allocationLogic_basic_infeasible_forces_all_memory();
  test_allocationLogic_spilling_preserves_partial_memory_assignment();
  test_allocationLogic_exports_colored_dot();

  std::cout << "\nTest results: " << g_passed << " passed, " << g_failed << " failed.\n";
  return g_failed == 0 ? 0 : 1;
}
