#include "CommandLine.hpp"
#include "pch.hpp"

void CommandLineParser::usage()
{
  std::cout << getProgramName() << " [Options]..." << std::endl << "Options:" << std::endl << "\t-r <file>\t\tLoad <file> as cartridge rom" << std::endl << "\t-h  | --help\t\tPrint this help" << std::endl;
}

CommandLineParser::CommandLineParser( const int argc, char **argv ) : mOptions{}
{
  for ( int i = 0; i < argc; ++i )
  {
    tokens.push_back( std::string( argv[i] ) );
  }
}

const std::optional<std::string> CommandLineParser::getCmdOption( const std::string &option ) const
{
  auto itr = std::find( tokens.begin(), tokens.end(), option );
  if ( itr != tokens.end() && ++itr != tokens.end() )
  {
    return std::optional( *( itr ) );
  }
  return std::nullopt;
}

template <typename... Options> const auto CommandLineParser::getAllOptions( const Options... ops ) const
{
  std::vector<std::optional<std::string>> v;
  ( v.push_back( getCmdOption( ops ) ), ... );
  return v;
}

bool CommandLineParser::cmdOptionExists( const std::string &option ) const
{
  return std::find( tokens.begin(), tokens.end(), option ) != tokens.end();
}

template <typename... Options> bool CommandLineParser::allOptionsExists( const Options... opts ) const
{
  return ( ... && cmdOptionExists( opts ) );
}

template <typename... Options> bool CommandLineParser::anyOptionsExists( const Options... opts ) const
{
  return ( ... || cmdOptionExists( opts ) );
}

const std::string &CommandLineParser::getProgramName() const
{
  return tokens[0];
}

bool CommandLineParser::processArgs()
{
  if ( anyOptionsExists( "-h", "--help" ) )
  {
    usage();
    return false;
  }

  auto cart = getCmdOption( "-r" );
  if ( cart.has_value() )
  {
    mOptions.cartridgeROM = cart.value();
  }

  return true;
}