#pragma once
#include "pch.hpp"
#include "UI.hpp"
#include "Log.hpp"
#include "IVideoSink.hpp"
#include "ImageProperties.hpp"
#include "imgui.h"

class IRenderer
{
public:
  IRenderer() {};
  virtual ~IRenderer() {};
  virtual void initialize() = 0;
  virtual int64_t render( UI& ui ) = 0;
  virtual void terminate() = 0;
  virtual bool shouldClose() = 0;
  virtual void setTitle( std::string title ) = 0;
  virtual std::shared_ptr<IVideoSink> getVideoSink() = 0;
  virtual ImTextureID getMainScreenTextureID() = 0;
  virtual void registerFileDropCallback( std::function<void( std::filesystem::path )> callback ) = 0;
  virtual void registerKeyEventCallback( std::function<void( int, bool )> callback ) = 0;
  virtual void setRotation( ImageProperties::Rotation rotation ) = 0;
  virtual ImVec2 getDimensions() = 0;
};
