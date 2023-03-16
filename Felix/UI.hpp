#pragma once

#include "IInputSource.hpp"
#include <GLFW/glfw3.h>
#include <optional>
#include <memory>

class Manager;
struct ImGuiIO;
struct DebugWindow;

namespace ImGui
{
class FileBrowser;
}

struct BoardRendering
{
  bool enabled;
  void* window;
  float width;
  float height;
};

class UI
{
public:
  UI( Manager& manager );
  ~UI();

  void initialize();
  void drawGui( int left, int top, int right, int bottom );

private:
  bool mainMenu( ImGuiIO& io );
  void drawDebugWindows( ImGuiIO& io );
  void drawMainScreenView();
  void configureKeyWindow( std::optional<KeyInput::Key>& keyToConfigure );
  void imagePropertiesWindow( bool init );

private:
  Manager& mManager;
  bool mOpenMenu;
  int mMainScreenViewId{};
  std::unique_ptr<ImGui::FileBrowser> mFileBrowser;
};
