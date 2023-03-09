#pragma once
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

class CommandLineParser
{
public:
  CommandLineParser( const int argc, char **argv );

  bool processArgs();

  struct CommandLineOptions
  {
    std::string cartridgeROM;
  } mOptions;

private:
  const std::optional<std::string> getCmdOption( const std::string &option ) const;
  template <typename... Options> const auto getAllOptions( const Options... ops ) const;
  bool cmdOptionExists( const std::string &option ) const;
  template <typename... Options> bool allOptionsExists( const Options... opts ) const;
  template <typename... Options> bool anyOptionsExists( const Options... opts ) const;
  const std::string &getProgramName() const;
  void usage();

  std::vector<std::string> tokens;
};