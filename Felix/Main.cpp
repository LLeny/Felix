#include "AudioOut.hpp"
#include "ComLynxWire.hpp"
#include "CommandLine.hpp"
#include "ConfigProvider.hpp"
#include "Core.hpp"
#include "IInputSource.hpp"
#include "ISystemDriver.hpp"
#include "InputFile.hpp"
#include "Log.hpp"
#include "Manager.hpp"
#include "SysConfig.hpp"
#include "pch.hpp"

int main( int argc, char *argv[] )
{
  try
  {
    L_SET_LOGLEVEL( Log::LL_DEBUG );

    CommandLineParser p( argc, argv );
    if ( !p.processArgs() )
    {
      return 0;
    }

    Manager manager;

    auto systemDriver = createSystemDriver( manager, p.mOptions, 0 );

    systemDriver->eventLoop( manager );

    return 0;
  }
  catch ( sol::error const &err )
  {
    L_ERROR << err.what();
    return -1;
  }
  catch ( std::exception const &ex )
  {
    L_ERROR << ex.what();
    return -1;
  }
}
