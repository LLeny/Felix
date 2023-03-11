#include "pch.hpp"
#include "SystemDriver.hpp"
#include "CommandLine.hpp"
#include "ConfigProvider.hpp"
#include "Manager.hpp"
#include "SysConfig.hpp"
#include "UserInput.hpp"
#include "version.hpp"

#ifndef _WIN32
#include <time.h>
#endif

bool runOtherInstanceIfPresent()
{
  auto sysConfig = gConfigProvider.sysConfig();

  if ( sysConfig->singleInstance && !gConfigProvider.isLocked() )
  {
    L_INFO << "Felix if configured as single instance and another instance seems to be running.";
    return false;
  }

  return true;
}

std::shared_ptr<ISystemDriver> createSystemDriver( Manager &manager, CommandLineParser::CommandLineOptions &options, int nCmdShow )
{
  if ( !runOtherInstanceIfPresent() )
  {
    return {};
  }

  std::string name = appname_string + " " + version_string;

  auto pSystemDriver = std::make_shared<SystemDriver>();

  pSystemDriver->initialize();

  manager.initialize( pSystemDriver );

  if ( pSystemDriver->mDropFilesHandler )
  {
    std::filesystem::path path( options.cartridgeROM );

    if ( std::filesystem::exists( path ) )
    {
      pSystemDriver->mDropFilesHandler( std::move( path ) );
    }
  }

  return pSystemDriver;
}

SystemDriver::SystemDriver() : mIntputSource{}
{
}

void SystemDriver::initialize()
{
  auto iniPath = gConfigProvider.appDataFolder();

  mRenderer     = std::make_shared<VulkanRenderer>();
  mIntputSource = std::make_shared<UserInput>();

  mRenderer->initialize();
  mRenderer->registerFileDropCallback( [&]( std::filesystem::path path ) { this->handleFileDrop( path ); } );
  mRenderer->registerKeyEventCallback( [&]( int key, bool pressed ) { this->handleKeyEvent( key, pressed ); } );
}

int SystemDriver::eventLoop( Manager &m )
{
#ifdef _WIN32
  LARGE_INTEGER l;
#else
  timespec ts;
#endif
  int64_t now;

  while ( !mRenderer->shouldClose() )
  {
#ifdef _WIN32
    QueryPerformanceCounter( &l );
    now = l.QuadPart;
#else
    timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    now = ( ts.tv_sec * 1000000000UL ) + ts.tv_nsec;
#endif

    update();

    mRenderingTime     = now - mLastRenderingTime;
    mLastRenderingTime = now;

    std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
  }

  return 0;
}

void SystemDriver::quit()
{
  mRenderer->terminate();
}

void SystemDriver::setTitle( std::string title )
{
  mRenderer->setTitle( title );
}

void SystemDriver::update()
{
  mIntputSource->updateGamepad();
  if ( mUpdateHandler )
  {
    mUpdateHandler();
  }

  if ( mPaused != mNewPaused )
  {
    mPaused = mNewPaused;

    std::string n = appname_string + " " + version_string + " " + mImageFileName;

    if ( mPaused )
    {
      n += " paused";
    }

    setTitle( n );
  }
}

std::shared_ptr<IUserInput> SystemDriver::userInput() const
{
  return mIntputSource;
}

std::shared_ptr<IRenderer> SystemDriver::renderer() const
{
  return mRenderer;
}

void SystemDriver::updateRotation( ImageProperties::Rotation rotation )
{
  mIntputSource->setRotation( rotation );
  mRenderer->setRotation( rotation );
}

void SystemDriver::setImageName( std::string name )
{
  mImageFileName = std::move( name );
  std::string n  = appname_string + " " + version_string + " " + mImageFileName;
  setTitle( n );
}

void SystemDriver::setPaused( bool paused )
{
  mNewPaused = paused;
}

void SystemDriver::registerDropFiles( std::function<void( std::filesystem::path )> dropFilesHandler )
{
  mDropFilesHandler = std::move( dropFilesHandler );
}

void SystemDriver::registerUpdate( std::function<void()> updateHandler )
{
  mUpdateHandler = std::move( updateHandler );
}

void SystemDriver::handleFileDrop( std::filesystem::path file )
{
  if ( mDropFilesHandler )
  {
    mDropFilesHandler( std::move( file ) );
  }
}

void SystemDriver::handleKeyEvent( int key, bool pressed )
{
  if( pressed )
  {
    mIntputSource->keyDown(key);
  }
  else
  {
    mIntputSource->keyUp(key);
  }  
}
