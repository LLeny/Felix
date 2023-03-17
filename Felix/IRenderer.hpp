#pragma once
#include "pch.hpp"
#include "UI.hpp"
#include "Log.hpp"
#include "IVideoSink.hpp"
#include "ImageProperties.hpp"
#include "imgui.h"

class Manager;

class IRenderer
{
public:
  IRenderer() {};
  virtual ~IRenderer() {};

  virtual void initialize() = 0;
  virtual void terminate() = 0;

  virtual bool shouldClose() = 0;
  virtual void setTitle( std::string title ) = 0;
  
  virtual int64_t render( Manager& manager, UI& ui ) = 0;

  virtual std::shared_ptr<IVideoSink> getVideoSink() = 0;
  virtual ImVec2 getDimensions() = 0;
  virtual void setRotation( ImageProperties::Rotation rotation ) = 0;  

  virtual int addScreenView( uint16_t baseAddress ) = 0;
  virtual void setScreenViewBaseAddress( int id, uint16_t baseAddress ) = 0;

  virtual int addBoard( int width, int height, const char* content ) = 0;

  virtual bool deleteView( int screenId ) = 0;

  virtual ImTextureID getTextureID( int screenId ) = 0;

  virtual void registerFileDropCallback( std::function<void( std::filesystem::path )> callback ) = 0;
  virtual void registerKeyEventCallback( std::function<void( int, bool )> callback ) = 0;
};
