#pragma once
#include "CommandLine.hpp"
#include "IRenderer.hpp"
#include "ImageProperties.hpp"
#include "pch.hpp"


class IUserInput;
class Manager;

class ISystemDriver
{
public:
  virtual ~ISystemDriver() = default;

  virtual std::shared_ptr<IRenderer>  renderer() const                                     = 0;
  virtual void                        quit()                                               = 0;
  virtual void                        update()                                             = 0;
  virtual void                        initialize()                                         = 0;
  virtual std::shared_ptr<IUserInput> userInput() const                                    = 0;
  virtual void                        updateRotation( ImageProperties::Rotation rotation ) = 0;
  virtual void                        setImageName( std::string name )                     = 0;
  virtual void                        setPaused( bool paused )                             = 0;

  virtual int eventLoop( Manager &m ) = 0;

  virtual void registerDropFiles( std::function<void( std::filesystem::path )> ) = 0;
  virtual void registerUpdate( std::function<void()> )                           = 0;

  int64_t mRenderingTime;
};

std::shared_ptr<ISystemDriver> createSystemDriver( Manager &manager, CommandLineParser::CommandLineOptions &options, int nCmdShow );
