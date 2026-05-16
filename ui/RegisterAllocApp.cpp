/**
 * @file RegisterAllocApp.cpp
 * @brief Implementation of the interactive CLI and batch mode.
 */

#include "ui/RegisterAllocApp.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cctype>
#include <string>
#include "io/FileParser.h"

namespace fs = std::filesystem;

namespace
{

  bool looksLikeConfigPath(const std::string &p)
  {
    std::string s = p;
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c)
                   { return (char)std::tolower(c); });
    return s.find("config") != std::string::npos ||
           s.find("register") != std::string::npos ||
           s.find("params") != std::string::npos;
  }

} // namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
RegisterAllocApp::RegisterAllocApp()
    : mDataLoaded(false), mConfigLoaded(false), mGraphBuilt(false) {}

// ---------------------------------------------------------------------------
// run  – entry point
// ---------------------------------------------------------------------------
int RegisterAllocApp::run(int argc, char *argv[])
{
  // Batch mode: ./register_alloc -b ranges.txt registers.txt allocation.txt
  if (argc >= 2 && std::string(argv[1]) == "-b")
  {
    if (argc < 5)
    {
      std::cerr << "Usage: " << argv[0]
                << " -b <ranges.txt> <registers.txt> <allocation.txt>\n";
      return 1;
    }
    return runBatchMode(argv[2], argv[3], argv[4]);
  }

  if (argc > 1)
  {
    std::cerr << "Usage: " << argv[0]
              << " [-b <ranges.txt> <registers.txt> <allocation.txt>]\n";
    return 1;
  }

  runInteractiveMode();
  return 0;
}

// ---------------------------------------------------------------------------
// Helper utilities
// ---------------------------------------------------------------------------
void RegisterAllocApp::printSep(char c, int n) const
{
  std::cout << "   " << std::string(n, c) << "\n";
}

void RegisterAllocApp::waitEnter() const
{
  std::cout << "\nPress Enter to continue...";
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

int RegisterAllocApp::readInt(const std::string &prompt) const
{
  int val;
  while (true)
  {
    std::cout << "   " << prompt;
    if (std::cin >> val)
    {
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      return val;
    }
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cout << "   Invalid input. Please enter a number.\n";
  }
}

std::string RegisterAllocApp::readLine(const std::string &prompt) const
{
  std::string s;
  std::cout << "   " << prompt;
  std::getline(std::cin, s);
  // Strip surrounding spaces / quotes
  while (!s.empty() && (s.front() == ' ' || s.front() == '"'))
    s.erase(s.begin());
  while (!s.empty() && (s.back() == ' ' || s.back() == '"'))
    s.pop_back();
  return s;
}

// ---------------------------------------------------------------------------
// Core logic
// ---------------------------------------------------------------------------
bool RegisterAllocApp::loadRangesFile(const std::string &path)
{
  mRanges.clear();
  mWebs.clear();
  mGraph = Graph<int>();
  mGraphBuilt = false;
  mDataLoaded = false;

  if (!FileParser::parseRanges(path, mRanges))
  {
    std::cout << "   Failed to load ranges file. Check error messages above.\n";
    return false;
  }
  mDataLoaded = true;
  std::cout << "   Loaded " << mRanges.size() << " variable(s) from '" << path << "'.\n";
  return true;
}

bool RegisterAllocApp::loadConfigFile(const std::string &path)
{
  mConfigLoaded = false;
  if (!FileParser::parseConfig(path, mParams))
  {
    std::cout << "   Failed to load config file. Check error messages above.\n";
    return false;
  }
  mConfigLoaded = true;
  std::cout << "   Config loaded: " << mParams.numRegisters << " register(s), algorithm: ";
  switch (mParams.algorithm)
  {
  case AlgorithmType::Basic:
    std::cout << "basic";
    break;
  case AlgorithmType::Spilling:
    std::cout << "spilling (K=" << mParams.algorithmParam << ")";
    break;
  case AlgorithmType::Splitting:
    std::cout << "splitting (K=" << mParams.algorithmParam << ")";
    break;
  case AlgorithmType::Free:
    std::cout << "free";
    break;
  }
  std::cout << ".\n";
  return true;
}

void RegisterAllocApp::buildGraphFromData()
{
  if (!mDataLoaded)
  {
    std::cout << "   No ranges data loaded. Load the ranges file first.\n";
    return;
  }
  mWebs = InterferenceGraph::buildWebs(mRanges);
  mGraph = InterferenceGraph::buildGraph(mWebs);
  mGraphBuilt = true;
  std::cout << "   Built " << mWebs.size() << " web(s) and interference graph with "
            << mGraph.getNumVertex() << " node(s).\n";

  int lb = AllocationLogic::estimateChromatic(mGraph, mWebs);
  std::cout << "   Estimated minimum registers required (clique lower bound): " << lb << ".\n";
}

void RegisterAllocApp::runAllocation()
{
  if (!mGraphBuilt)
  {
    buildGraphFromData();
    if (!mGraphBuilt)
      return;
  }
  if (!mConfigLoaded)
  {
    std::cout << "   Config not loaded. Please load a config file first.\n";
    return;
  }
  mLastResult = AllocationLogic::runAllocation(mWebs, mGraph, mParams);
  if (!mParams.outputFile.empty())
  {
    fs::path dotPath = fs::path(mParams.outputFile).replace_extension(".dot");
    AllocationLogic::exportAllocationDOT(mGraph, mWebs, mLastResult, dotPath.string());
  }
}

// ---------------------------------------------------------------------------
// Batch mode
// ---------------------------------------------------------------------------
int RegisterAllocApp::runBatchMode(const std::string &rangesFile,
                                   const std::string &configFile,
                                   const std::string &outputFile)
{
  std::cerr << "Batch mode\n";

  if (!FileParser::parseRanges(rangesFile, mRanges))
  {
    std::cerr << "Error: failed to parse ranges file '" << rangesFile << "'.\n";
    return 1;
  }
  std::cerr << "Loaded " << mRanges.size() << " variable(s).\n";

  if (!FileParser::parseConfig(configFile, mParams))
  {
    std::cerr << "Error: failed to parse config file '" << configFile << "'.\n";
    return 1;
  }
  mParams.outputFile = outputFile;

  mWebs = InterferenceGraph::buildWebs(mRanges);
  mGraph = InterferenceGraph::buildGraph(mWebs);
  std::cerr << "Built " << mWebs.size() << " web(s), "
            << mGraph.getNumVertex() << " node(s) in interference graph.\n";

  mLastResult = AllocationLogic::runAllocation(mWebs, mGraph, mParams);
  if (!mParams.outputFile.empty())
  {
    fs::path dotPath = fs::path(mParams.outputFile).replace_extension(".dot");
    AllocationLogic::exportAllocationDOT(mGraph, mWebs, mLastResult, dotPath.string());
  }
  AllocationLogic::printResult(mWebs, mLastResult);
  return 0;
}

// ---------------------------------------------------------------------------
// Interactive menus
// ---------------------------------------------------------------------------
void RegisterAllocApp::printMainMenu() const
{
  printSep('=', 57);
  std::cout << "   Compiler Register Allocation Project\n"
            << "   DA Programming Project II  –  Spring 2026\n";
  printSep('=', 57);
  if (mDataLoaded)
    std::cout << "   Ranges : " << mRanges.size() << " variable(s) loaded.\n";
  else
    std::cout << "   Ranges : No data loaded.\n";

  if (mConfigLoaded)
  {
    std::cout << "   Config : " << mParams.numRegisters << " register(s), algorithm: ";
    switch (mParams.algorithm)
    {
    case AlgorithmType::Basic:
      std::cout << "basic";
      break;
    case AlgorithmType::Spilling:
      std::cout << "spilling (K=" << mParams.algorithmParam << ")";
      break;
    case AlgorithmType::Splitting:
      std::cout << "splitting (K=" << mParams.algorithmParam << ")";
      break;
    case AlgorithmType::Free:
      std::cout << "free";
      break;
    }
    std::cout << ".\n";
  }
  else
  {
    std::cout << "   Config : Not loaded.\n";
  }

  if (mGraphBuilt)
    std::cout << "   Graph  : " << mWebs.size() << " webs, "
              << mGraph.getNumVertex() << " nodes.\n";

  std::cout << "   Ready  : " << ((mDataLoaded && mConfigLoaded) ? "Yes (both files loaded)" : "No (load ranges + config)") << "\n";

  printSep('-', 57);
  std::cout << "   [1] Data Management\n"
            << "   [2] Configuration\n"
            << "   [3] Algorithms\n"
            << "   [4] Visualization\n"
            << "   [0] Exit\n";
  printSep('=', 57);
}

void RegisterAllocApp::runInteractiveMode()
{
  while (true)
  {
    printMainMenu();
    int choice = readInt("Option: ");
    std::cout << "\n";
    switch (choice)
    {
    case 1:
      menuDataManagement();
      break;
    case 2:
      menuConfiguration();
      break;
    case 3:
      menuAlgorithms();
      break;
    case 4:
      menuVisualization();
      break;
    case 0:
      std::cout << "See you soon! If you ever need to figure out how to allocate some more registers, don't hesitate to ask!\n";
      return;
    default:
      std::cout << "   Unknown option. Try again.\n";
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Data management menu
// ---------------------------------------------------------------------------
void RegisterAllocApp::menuDataManagement()
{
  while (true)
  {
    printSep('=', 57);
    std::cout << "   [ Data Management ]\n";
    printSep('-', 57);
    std::cout << "   [1] Quick load BOTH files from inputs/\n"
              << "   [2] Load BOTH files manually (ranges + config)\n"
              << "   [3] Load ranges file only (advanced)\n"
              << "   [4] Load config file only (advanced)\n"
              << "   [5] Build interference graph\n"
              << "   [6] View loaded webs\n"
              << "   [7] View current parameters\n"
              << "   [0] Back\n";
    printSep('=', 57);
    int choice = readInt("Option: ");
    switch (choice)
    {
    case 1:
      doLoadInputPairFromFolder();
      break;
    case 2:
      doLoadInputPairManual();
      break;
    case 3:
      doLoadRangesFile();
      break;
    case 4:
      doLoadConfigFile();
      break;
    case 5:
      buildGraphFromData();
      waitEnter();
      break;
    case 6:
      doViewWebs();
      waitEnter();
      break;
    case 7:
      doViewParameters();
      waitEnter();
      break;
    case 0:
      return;
    default:
      std::cout << "   Unknown option.\n";
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Configuration menu
// ---------------------------------------------------------------------------
void RegisterAllocApp::menuConfiguration()
{
  while (true)
  {
    printSep('=', 57);
    std::cout << "   [ Configuration ]\n";
    printSep('-', 57);
    std::cout << "   [1] Set number of registers  : " << mParams.numRegisters << "\n"
              << "   [2] Set algorithm            : ";
    switch (mParams.algorithm)
    {
    case AlgorithmType::Basic:
      std::cout << "basic\n";
      break;
    case AlgorithmType::Spilling:
      std::cout << "spilling (K=" << mParams.algorithmParam << ")\n";
      break;
    case AlgorithmType::Splitting:
      std::cout << "splitting (K=" << mParams.algorithmParam << ")\n";
      break;
    case AlgorithmType::Free:
      std::cout << "free\n";
      break;
    }
    std::cout << "   [3] Set output file          : " << mParams.outputFile << "\n"
              << "   [0] Back\n";
    printSep('=', 57);
    int choice = readInt("Option: ");
    switch (choice)
    {
    case 1:
    {
      int value = readInt("Number of registers: ");
      if (value < 0)
      {
        std::cout << "   Invalid value. The number of registers must be non-negative.\n";
        break;
      }
      mParams.numRegisters = value;
      mConfigLoaded = true;
      break;
    }
    case 2:
    {
      std::cout << "   Algorithms: [1] basic  [2] spilling  [3] splitting  [4] free\n";
      int a = readInt("Select: ");
      if (a == 1)
      {
        mParams.algorithm = AlgorithmType::Basic;
      }
      else if (a == 2)
      {
        mParams.algorithm = AlgorithmType::Spilling;
        int value = readInt("Max spills (K): ");
        if (value < 0)
        {
          std::cout << "   Invalid value. Max spills must be non-negative.\n";
          break;
        }
        mParams.algorithmParam = value;
      }
      else if (a == 3)
      {
        mParams.algorithm = AlgorithmType::Splitting;
        int value = readInt("Max splits (K): ");
        if (value < 0)
        {
          std::cout << "   Invalid value. Max splits must be non-negative.\n";
          break;
        }
        mParams.algorithmParam = value;
      }
      else if (a == 4)
      {
        mParams.algorithm = AlgorithmType::Free;
      }
      else
      {
        std::cout << "   Invalid.\n";
        break;
      }
      mConfigLoaded = true;
      break;
    }
    case 3:
      mParams.outputFile = readLine("Output file path: ");
      break;
    case 0:
      return;
    default:
      std::cout << "   Unknown option.\n";
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Algorithms menu
// ---------------------------------------------------------------------------
void RegisterAllocApp::menuAlgorithms()
{
  while (true)
  {
    printSep('=', 57);
    std::cout << "   [ Algorithms ]\n";
    printSep('-', 57);
    std::cout << "   [1] Run register allocation\n"
              << "   [2] Show last result\n"
              << "   [0] Back\n";
    printSep('=', 57);
    int choice = readInt("Option: ");
    switch (choice)
    {
    case 1:
      doRunAllocation();
      waitEnter();
      break;
    case 2:
      doShowAllocationResult();
      waitEnter();
      break;
    case 0:
      return;
    default:
      std::cout << "   Unknown option.\n";
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Visualization menu
// ---------------------------------------------------------------------------
void RegisterAllocApp::menuVisualization()
{
  while (true)
  {
    printSep('=', 57);
    std::cout << "   [ Visualization ]\n";
    printSep('-', 57);
    std::cout << "   [1] Print interference graph (terminal)\n"
              << "   [2] Export interference graph (DOT)\n"
              << "   [3] Export colored allocation graph (DOT)\n"
              << "   [0] Back\n";
    printSep('=', 57);
    int choice = readInt("Option: ");
    switch (choice)
    {
    case 1:
      doViewGraphTerminal();
      waitEnter();
      break;
    case 2:
      doExportGraphDOT();
      waitEnter();
      break;
    case 3:
      doExportAllocationDOT();
      waitEnter();
      break;
    case 0:
      return;
    default:
      std::cout << "   Unknown option.\n";
      break;
    }
  }
}

// ---------------------------------------------------------------------------
// Data management actions
// ---------------------------------------------------------------------------
void RegisterAllocApp::doLoadInputPairFromFolder()
{
  std::string dirPath = "inputs";
  if (!fs::exists(dirPath))
    dirPath = "tests/input";

  std::vector<std::string> files;
  if (fs::exists(dirPath))
  {
    for (const auto &entry : fs::recursive_directory_iterator(dirPath))
    {
      if (!entry.is_regular_file())
        continue;
      auto ext = entry.path().extension().string();
      if (ext == ".txt" || ext == ".ranges" || ext == ".cfg" || ext == ".conf")
      {
        files.push_back(entry.path().string());
      }
    }
  }

  if (files.empty())
  {
    std::cout << "   No candidate input files found in '" << dirPath << "'.\n";
    return;
  }

  std::sort(files.begin(), files.end());

  std::vector<int> cfgIdx;
  std::vector<int> rngIdx;

  std::cout << "   Files in '" << dirPath << "':\n";
  for (size_t i = 0; i < files.size(); ++i)
  {
    bool isCfg = looksLikeConfigPath(files[i]);
    if (isCfg)
      cfgIdx.push_back((int)i + 1);
    else
      rngIdx.push_back((int)i + 1);
    std::cout << "   [" << i + 1 << "] " << files[i]
              << (isCfg ? "   (config?)" : "   (ranges?)") << "\n";
  }
  std::cout << "   [0] Cancel\n";

  int defaultRange = rngIdx.empty() ? 0 : rngIdx.front();
  int defaultCfg = cfgIdx.empty() ? 0 : cfgIdx.front();

  if (defaultRange > 0 && defaultCfg > 0)
  {
    std::cout << "   Suggested pair:\n"
              << "     ranges -> [" << defaultRange << "] " << files[defaultRange - 1] << "\n"
              << "     config -> [" << defaultCfg << "] " << files[defaultCfg - 1] << "\n";
  }

  int rangesChoice = readInt("Select ranges file index: ");
  if (rangesChoice <= 0 || rangesChoice > (int)files.size())
    return;

  int configChoice = readInt("Select config file index: ");
  if (configChoice <= 0 || configChoice > (int)files.size())
    return;

  bool okRanges = loadRangesFile(files[rangesChoice - 1]);
  bool okConfig = loadConfigFile(files[configChoice - 1]);

  if (okRanges && okConfig)
  {
    std::cout << "   Both files loaded successfully.\n";
    buildGraphFromData();
  }
}

void RegisterAllocApp::doLoadInputPairManual()
{
  std::cout << "   This tool needs TWO input files:\n"
            << "     1) ranges file (live ranges per variable)\n"
            << "     2) config file (registers + algorithm)\n";

  std::string rangesPath = readLine("Enter path to ranges file: ");
  if (rangesPath.empty())
    return;

  std::string configPath = readLine("Enter path to config file: ");
  if (configPath.empty())
    return;

  bool okRanges = loadRangesFile(rangesPath);
  bool okConfig = loadConfigFile(configPath);
  if (okRanges && okConfig)
  {
    std::cout << "   Both files loaded successfully.\n";
    buildGraphFromData();
  }
}

void RegisterAllocApp::doLoadRangesFile()
{
  std::string path = readLine("Enter path to ranges file: ");
  if (!path.empty())
    loadRangesFile(path);
}

void RegisterAllocApp::doLoadConfigFile()
{
  std::string path = readLine("Enter path to config file: ");
  if (!path.empty())
    loadConfigFile(path);
}

void RegisterAllocApp::doViewWebs() const
{
  if (!mDataLoaded)
  {
    std::cout << "   No data loaded.\n";
    return;
  }
  if (!mGraphBuilt)
  {
    std::cout << "   Graph not built yet. Run 'Build interference graph' first.\n";
    return;
  }

  std::cout << "   Webs (" << mWebs.size() << "):\n";
  for (const auto &web : mWebs)
  {
    std::cout << "   web" << web.id << " [" << web.variable << "]:\n";
    for (const auto &range : web.ranges)
    {
      std::cout << "     ";
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
}

void RegisterAllocApp::doViewParameters() const
{
  std::cout << "   Current parameters:\n"
            << "     numRegisters   : " << mParams.numRegisters << "\n"
            << "     algorithm      : ";
  switch (mParams.algorithm)
  {
  case AlgorithmType::Basic:
    std::cout << "basic\n";
    break;
  case AlgorithmType::Spilling:
    std::cout << "spilling (K=" << mParams.algorithmParam << ")\n";
    break;
  case AlgorithmType::Splitting:
    std::cout << "splitting (K=" << mParams.algorithmParam << ")\n";
    break;
  case AlgorithmType::Free:
    std::cout << "free\n";
    break;
  }
  std::cout << "     outputFile     : " << mParams.outputFile << "\n";
}

// ---------------------------------------------------------------------------
// Algorithm actions
// ---------------------------------------------------------------------------
void RegisterAllocApp::doRunAllocation()
{
  if (!mDataLoaded)
  {
    std::cout << "   No ranges loaded.\n";
    return;
  }
  if (!mConfigLoaded)
  {
    std::cout << "   No config loaded.\n";
    return;
  }
  runAllocation();
  AllocationLogic::printResult(mWebs, mLastResult);
}

// ---------------------------------------------------------------------------
// Visualization actions
// ---------------------------------------------------------------------------
void RegisterAllocApp::doViewGraphTerminal() const
{
  if (!mGraphBuilt)
  {
    std::cout << "   Graph not built.\n";
    return;
  }
  InterferenceGraph::printGraph(mGraph, mWebs);
}

void RegisterAllocApp::doExportGraphDOT() const
{
  if (!mGraphBuilt)
  {
    std::cout << "   Graph not built.\n";
    return;
  }
  std::string path = readLine("Output DOT file path [interference.dot]: ");
  if (path.empty())
    path = "interference.dot";
  InterferenceGraph::exportDOT(mGraph, mWebs, path);
}

void RegisterAllocApp::doExportAllocationDOT() const
{
  if (!mGraphBuilt)
  {
    std::cout << "   Graph not built.\n";
    return;
  }
  if (mLastResult.webToRegister.empty())
  {
    std::cout << "   No allocation result available. Run register allocation first.\n";
    return;
  }

  std::string path = readLine("Output colored DOT file path [allocation_colored.dot]: ");
  if (path.empty())
    path = "allocation_colored.dot";
  AllocationLogic::exportAllocationDOT(mGraph, mWebs, mLastResult, path);
}

void RegisterAllocApp::doShowAllocationResult() const
{
  AllocationLogic::printResult(mWebs, mLastResult);
}
