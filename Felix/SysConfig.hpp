#pragma once

#include "imgui.h"

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
    int pause = ImGuiKey_2;
    int down = ImGuiKey_DownArrow;
    int up = ImGuiKey_UpArrow;
    int right = ImGuiKey_RightArrow;
    int left = ImGuiKey_LeftArrow;
    int option1 = ImGuiKey_1;
    int option2 = ImGuiKey_3;
    int inner = ImGuiKey_Z;
    int outer = ImGuiKey_X;
  } keyMapping;
  std::filesystem::path lastOpenDirectory{};
  bool debugMode;
  bool visualizeCPU;
  bool visualizeDisasm;
  struct DisasmyOptions
  {
    bool  FollowPC = true;
    bool  ShowLabelsInAddrCol = true;
  } disasmOptions;
  bool visualizeMemory;
  struct MemoryOptions
  {
    bool  OptShowOptions = true;
    bool  OptShowDataPreview;
    bool  OptShowHexII;
    bool  OptShowAscii;
    bool  OptGreyOutZeroes;
    bool  OptUpperCaseHex;
    int   OptMidColsCount;
    int   OptAddrDigitsCount;
    float OptFooterExtraHeight;
  } memoryOptions;
  bool visualizeWatch;
  bool visualizeBreakpoint;
  bool visualizeHistory;
  bool debugModeOnBreak;
  bool normalModeOnRun;
  bool breakOnBrk;
  struct ScreenView
  {
    int id;
    int type;
    int customAddress;
    int safePalette;
  };
  std::vector<ScreenView> screenViews;
  struct Audio
  {
    bool mute;
  } audio;

  static std::shared_ptr<SysConfig> load( std::filesystem::path path );
  void serialize( std::filesystem::path path );

private:
  void load( sol::state const& lua );
};
