#pragma once

#include "imgui.h"
#include <GLFW/glfw3.h>

struct SysConfig
{
  struct
  {
    int x = 20;
    int y = 20;
    int width = 960;
    int height = 630;
  } mainWindow;
  bool singleInstance = false;
  struct BootROM
  {
    bool useExternal = false;
    std::filesystem::path path{};
  } bootROM;
  struct KeyMapping
  {
#ifdef WIN32
    int pause = '2';
    int down = 264;
    int up = 265;
    int right = 262;
    int left = 263;
    int option1 = '1';
    int option2 = '3';
    int inner = 'Z';
    int outer = 'X';
#else
    int pause = GLFW_KEY_2;
    int down = GLFW_KEY_DOWN;
    int up = GLFW_KEY_UP;
    int right = GLFW_KEY_RIGHT;
    int left = GLFW_KEY_LEFT;
    int option1 = GLFW_KEY_1;
    int option2 = GLFW_KEY_3;
    int inner = GLFW_KEY_Z;
    int outer = GLFW_KEY_X;
#endif
  } keyMapping;
  std::filesystem::path lastOpenDirectory{};
  bool debugMode{};
  bool visualizeCPU{};
  bool visualizeDisasm{};
  struct DisasmyOptions
  {
    bool  followPC = true;
    bool  ShowLabelsInAddrCol = true;
    int   tablePC = 0x200;
  } disasmOptions;
  bool visualizeMemory{};
  struct MemoryOptions
  {
    bool  OptShowOptions = true;
    bool  OptShowDataPreview{};
    bool  OptShowHexII{};
    bool  OptShowAscii{};
    bool  OptGreyOutZeroes{};
    bool  OptUpperCaseHex{};
    int   OptMidColsCount{};
    int   OptAddrDigitsCount{};
    float OptFooterExtraHeight{};
  } memoryOptions;
  bool visualizeWatch{};
  bool visualizeBreakpoint{};
  bool visualizeHistory{};
  bool debugModeOnBreak{};
  bool normalModeOnRun{};
  bool breakOnBrk{};
  struct ScreenView
  {
    int id{};
    int type{};
    int customAddress{};
    int safePalette{};
  };
  std::vector<ScreenView> screenViews;
  struct Audio
  {
    bool mute{};
  } audio;

  SysConfig();
  SysConfig( sol::state const& lua );

  void serialize( std::filesystem::path path );

  static std::shared_ptr<SysConfig> load( std::filesystem::path path );
};
