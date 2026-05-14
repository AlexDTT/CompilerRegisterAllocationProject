/**
 * @file FileParser.cpp
 * @brief Implementation of FileParser.
 */

#include "io/FileParser.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <set>

// ---------------------------------------------------------------------------
// Helpers (anonymous namespace)
// ---------------------------------------------------------------------------
namespace
{

  std::string trimStr(const std::string &s)
  {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
      return "";
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
  }

  std::string stripComment(const std::string &line)
  {
    auto pos = line.find('#');
    return (pos == std::string::npos) ? line : line.substr(0, pos);
  }

  std::vector<std::string> splitByComma(const std::string &s)
  {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string token;
    while (std::getline(ss, token, ','))
      parts.push_back(trimStr(token));
    return parts;
  }

} // anonymous namespace

// ---------------------------------------------------------------------------
// FileParser::cleanLine
// ---------------------------------------------------------------------------
std::string FileParser::cleanLine(const std::string &s)
{
  return trimStr(stripComment(s));
}

// ---------------------------------------------------------------------------
// FileParser::parsePoints
// ---------------------------------------------------------------------------
bool FileParser::parsePoints(const std::string &token, std::vector<ProgramPoint> &points)
{
  auto parts = splitByComma(token);
  std::set<int> seenLines;
  for (const auto &part : parts)
  {
    if (part.empty())
    {
      std::cerr << "Error: empty program-point entry in '" << token << "'.\n";
      return false;
    }
    char marker = '\0';
    std::string numStr = part;

    if (!numStr.empty() && (numStr.back() == '+' || numStr.back() == '-'))
    {
      marker = numStr.back();
      numStr.pop_back();
    }
    if (numStr.empty())
    {
      std::cerr << "Error: empty line number in token '" << part << "'.\n";
      return false;
    }
    try
    {
      size_t parsedChars = 0;
      int line = std::stoi(numStr, &parsedChars);
      if (parsedChars != numStr.size())
      {
        std::cerr << "Error: invalid trailing characters in line number '" << numStr << "'.\n";
        return false;
      }
      if (line <= 0)
      {
        std::cerr << "Error: line number must be positive, got " << line << ".\n";
        return false;
      }
      if (!seenLines.insert(line).second)
      {
        std::cerr << "Error: duplicate program point " << line
                  << " in one live range.\n";
        return false;
      }
      points.emplace_back(line, marker);
    }
    catch (const std::exception &)
    {
      std::cerr << "Error: invalid line number '" << numStr << "'.\n";
      return false;
    }
  }
  return !points.empty();
}

// ---------------------------------------------------------------------------
// FileParser::parseRanges
// ---------------------------------------------------------------------------
bool FileParser::parseRanges(const std::string &filename,
                             std::map<std::string, std::vector<LiveRange>> &ranges)
{
  std::ifstream file(filename);
  if (!file.is_open())
  {
    std::cerr << "Error: cannot open ranges file '" << filename << "'.\n";
    return false;
  }

  std::string line;
  int lineNum = 0;
  bool hadError = false;
  std::map<std::string, bool> hasPlusByVar;
  std::map<std::string, bool> hasMinusByVar;

  while (std::getline(file, line))
  {
    ++lineNum;
    std::string clean = cleanLine(line);
    if (clean.empty())
      continue;

    // Expected format: "varname: 7+,8,9,10-"
    auto colonPos = clean.find(':');
    if (colonPos == std::string::npos)
    {
      std::cerr << "Warning: line " << lineNum << ": no colon found, skipping.\n";
      continue;
    }

    std::string varName = trimStr(clean.substr(0, colonPos));
    std::string pointStr = trimStr(clean.substr(colonPos + 1));

    if (varName.empty())
    {
      std::cerr << "Error: line " << lineNum << ": empty variable name.\n";
      hadError = true;
      continue;
    }
    if (pointStr.empty())
    {
      std::cerr << "Error: line " << lineNum << ": no program points after ':'.\n";
      hadError = true;
      continue;
    }

    LiveRange lr;
    lr.variable = varName;
    if (!parsePoints(pointStr, lr.points))
    {
      std::cerr << "Error: line " << lineNum << ": failed to parse program points.\n";
      hadError = true;
      continue;
    }

    // Per updated specification: a variable can have intersection-only ranges with
    // no '+'/'-' markers on that particular line, as long as at least one '+' and
    // one '-' exist somewhere across all ranges of that variable.
    for (const auto &p : lr.points)
    {
      if (p.marker == '+')
        hasPlusByVar[varName] = true;
      else if (p.marker == '-')
        hasMinusByVar[varName] = true;
    }

    ranges[varName].push_back(std::move(lr));
  }

  for (const auto &[varName, varRanges] : ranges)
  {
    (void)varRanges;
    bool hasPlus = hasPlusByVar[varName];
    bool hasMinus = hasMinusByVar[varName];
    if (!hasPlus || !hasMinus)
    {
      std::cerr << "Error: variable '" << varName
                << "' must have at least one '+' and one '-' across its live ranges.\n";
      hadError = true;
    }
  }

  if (ranges.empty() && !hadError)
  {
    std::cerr << "Warning: no live ranges found in '" << filename << "'.\n";
  }
  return !hadError;
}

// ---------------------------------------------------------------------------
// FileParser::parseConfig
// ---------------------------------------------------------------------------
bool FileParser::parseConfig(const std::string &filename, Parameters &params)
{
  std::ifstream file(filename);
  if (!file.is_open())
  {
    std::cerr << "Error: cannot open config file '" << filename << "'.\n";
    return false;
  }

  std::string line;
  int lineNum = 0;
  bool foundRegisters = false;
  bool foundAlgorithm = false;

  while (std::getline(file, line))
  {
    ++lineNum;
    std::string clean = cleanLine(line);
    if (clean.empty())
      continue;

    auto colonPos = clean.find(':');
    if (colonPos == std::string::npos)
    {
      std::cerr << "Warning: line " << lineNum << ": unexpected format, skipping.\n";
      continue;
    }

    std::string key = trimStr(clean.substr(0, colonPos));
    std::string value = trimStr(clean.substr(colonPos + 1));

    if (key == "registers")
    {
      try
      {
        params.numRegisters = std::stoi(value);
        if (params.numRegisters < 0)
        {
          std::cerr << "Error: registers must be non-negative.\n";
          return false;
        }
        foundRegisters = true;
      }
      catch (...)
      {
        std::cerr << "Error: line " << lineNum << ": invalid register count '" << value << "'.\n";
        return false;
      }
    }
    else if (key == "algorithm")
    {
      // value may be "basic", "spilling, 2", "splitting, 2", "free"
      auto parts = splitByComma(value);
      if (parts.empty())
      {
        std::cerr << "Error: line " << lineNum << ": empty algorithm value.\n";
        return false;
      }
      std::string algoName = trimStr(parts[0]);
      // lowercase
      std::transform(algoName.begin(), algoName.end(), algoName.begin(), ::tolower);

      if (algoName == "basic")
      {
        if (parts.size() != 1)
        {
          std::cerr << "Error: 'basic' must not have an extra numeric parameter.\n";
          return false;
        }
        params.algorithm = AlgorithmType::Basic;
        params.algorithmParam = 0;
      }
      else if (algoName == "spilling")
      {
        if (parts.size() != 2)
        {
          std::cerr << "Error: 'spilling' requires a numeric parameter: algorithm: spilling, K\n";
          return false;
        }
        params.algorithm = AlgorithmType::Spilling;
        try
        {
          params.algorithmParam = std::stoi(parts[1]);
          if (params.algorithmParam < 0)
          {
            std::cerr << "Error: spilling parameter must be non-negative.\n";
            return false;
          }
        }
        catch (...)
        {
          std::cerr << "Error: invalid spilling parameter '" << parts[1] << "'.\n";
          return false;
        }
      }
      else if (algoName == "splitting")
      {
        if (parts.size() != 2)
        {
          std::cerr << "Error: 'splitting' requires a numeric parameter: algorithm: splitting, K\n";
          return false;
        }
        params.algorithm = AlgorithmType::Splitting;
        try
        {
          params.algorithmParam = std::stoi(parts[1]);
          if (params.algorithmParam < 0)
          {
            std::cerr << "Error: splitting parameter must be non-negative.\n";
            return false;
          }
        }
        catch (...)
        {
          std::cerr << "Error: invalid splitting parameter '" << parts[1] << "'.\n";
          return false;
        }
      }
      else if (algoName == "free")
      {
        if (parts.size() != 1)
        {
          std::cerr << "Error: 'free' must not have an extra numeric parameter.\n";
          return false;
        }
        params.algorithm = AlgorithmType::Free;
        params.algorithmParam = 0;
      }
      else
      {
        std::cerr << "Error: line " << lineNum << ": unknown algorithm '" << algoName
                  << "'. Expected: basic, spilling, splitting, free.\n";
        return false;
      }
      foundAlgorithm = true;
    }
    else
    {
      std::cerr << "Warning: line " << lineNum << ": unknown key '" << key << "', skipping.\n";
    }
  }

  if (!foundRegisters)
  {
    std::cerr << "Error: 'registers' field not found in config file.\n";
    return false;
  }
  if (!foundAlgorithm)
  {
    std::cerr << "Warning: 'algorithm' field not found; defaulting to 'basic'.\n";
  }
  return true;
}
