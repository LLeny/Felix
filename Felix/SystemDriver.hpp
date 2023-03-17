#pragma once
#include "ISystemDriver.hpp"

class IRenderer;
class UserInput;
class VulkanRenderer;

class SystemDriver : public ISystemDriver
{
public:
  SystemDriver();

  ~SystemDriver() override = default;

  int eventLoop(Manager& m) override;

  std::shared_ptr<IRenderer> renderer() const override;
  void quit() override;
  void update() override;
  void initialize() override;
  std::shared_ptr<IUserInput> userInput() const override;
  void updateRotation( ImageProperties::Rotation rotation ) override;
  void setImageName( std::string name ) override;
  void setPaused( bool paused ) override;
  void setTitle( std::string title ) override;
  void registerDropFiles( std::function<void( std::filesystem::path )> dropFilesHandler ) override;
  void registerUpdate( std::function<void()> updataHandler ) override;

  int64_t mRenderingTime;

private:
  void handleFileDrop( std::filesystem::path path );
  void handleKeyEvent( int key, bool pressed  );

  friend std::shared_ptr<ISystemDriver> createSystemDriver( Manager& manager, CommandLineParser::CommandLineOptions &options, int nCmdShow );

  std::shared_ptr<VulkanRenderer> mRenderer;
  std::shared_ptr<UserInput> mIntputSource;

  std::function<void( std::filesystem::path )> mDropFilesHandler;
  std::function<void()> mUpdateHandler;
  
  std::string mImageFileName{};
  bool mPaused{};
  bool mNewPaused{};
  int64_t mLastRenderingTime;
};
