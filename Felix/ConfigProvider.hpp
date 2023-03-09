#pragma once

#include "Log.hpp"
#include "cfgpath.hpp"
#include "version.hpp"
#include <fcntl.h>
#include <stdio.h>

struct WinConfig;
struct SysConfig;

class ConfigProvider
{
public:
  ConfigProvider();
  ~ConfigProvider();

  std::shared_ptr<SysConfig> sysConfig() const;
  std::filesystem::path appDataFolder() const;

  void serialize();
  bool isLocked();

private:
  static std::filesystem::path obtainAppDataFolder();
  void lock();
  std::shared_ptr<SysConfig> mSysConfig;
  std::filesystem::path mAppDataFolder;
  bool mIsLocked = false;
};

extern ConfigProvider gConfigProvider;
