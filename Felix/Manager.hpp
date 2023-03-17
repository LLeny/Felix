#pragma once

#include "ScriptDebugger.hpp"
#include "Utility.hpp"
#include "UI.hpp"
#include "Debugger.hpp"
#include "CPUEditor.hpp"
#include "MemEditor.hpp"
#include "WatchEditor.hpp"
#include "DisasmEditor.h"
#include "BreakpointEditor.hpp"

class AudioOut;
class ComLynxWire;
class Core;
class SymbolSource;
class InputFile;
class IEncoder;
class WinImgui;
class ScriptDebuggerEscapes;
class IUserInput;
class KeyNames;
class ImageProperties;
class ImageROM;
struct ImGuiIO;
class IRenderer;
class ISystemDriver;
class VulkanRenderer;

class Manager
{
public:
  Manager();
  ~Manager();

  void update();
  void reset();
  void updateRotation();
  void initialize( std::shared_ptr<ISystemDriver> systemDriver );
  IUserInput& userInput() const;
  void quit();
  UI mUI;

private:
  void processLua( std::filesystem::path const& path );
  std::optional<InputFile> computeInputFile();
  void stopThreads();
  void handleFileDrop( std::filesystem::path path );

  void updateDebugWindows();
  BoardRendering renderHistoryWindow();
  
  static std::shared_ptr<ImageROM const> getOptionalBootROM();
private:

  friend struct RamProxy;
  friend struct RomProxy;
  friend struct MikeyProxy;
  friend struct SuzyProxy;
  friend struct CPUProxy;
  friend class UI;
  friend class CPUEditor;
  friend class MemEditor;
  friend class WatchEditor;
  friend class DisasmEditor;
  friend class BreakpointEditor;
  friend class VulkanRenderer;

  bool mDoReset;

  Debugger mDebugger;

  struct DebugWindows
  {
    CPUEditor cpuEditor;
    MemEditor memoryEditor;
    WatchEditor watchEditor;
    DisasmEditor disasmEditor;
    BreakpointEditor breakpointEditor;
    BoardRendering historyBoard;
  } mDebugWindows;

  sol::state mLua;
  std::atomic_bool mProcessThreads;
  std::atomic_bool mJoinThreads;
  std::thread mRenderThread;
  std::thread mAudioThread;
  std::shared_ptr<ISystemDriver> mSystemDriver;
  std::shared_ptr<AudioOut> mAudioOut;
  std::shared_ptr<ComLynxWire> mComLynxWire;
  std::shared_ptr<IEncoder> mEncoder;
  std::unique_ptr<SymbolSource> mSymbols;
  std::shared_ptr<Core> mInstance;
  std::shared_ptr<ScriptDebuggerEscapes> mScriptDebuggerEscapes;
  std::shared_ptr<ImageProperties> mImageProperties;
  std::filesystem::path mArg;
  std::filesystem::path mLogPath;
  std::mutex mMutex;
  int64_t mRenderingTime;
};
