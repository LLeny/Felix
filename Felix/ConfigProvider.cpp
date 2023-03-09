#include "pch.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

ConfigProvider::ConfigProvider() : mAppDataFolder{ obtainAppDataFolder() }, mSysConfig{}
{
  L_INFO << "ConfigProvider '" << mAppDataFolder << "'";
  std::filesystem::create_directories( mAppDataFolder );
  mSysConfig = SysConfig::load( mAppDataFolder );
}

ConfigProvider::~ConfigProvider()
{
  serialize();
}

std::shared_ptr<SysConfig> ConfigProvider::sysConfig() const
{
  return mSysConfig;
}

std::filesystem::path ConfigProvider::appDataFolder() const
{
  return mAppDataFolder;
}

void ConfigProvider::serialize()
{
  mSysConfig->serialize( mAppDataFolder );
}

std::filesystem::path ConfigProvider::obtainAppDataFolder()
{
  char cfgdir[MAX_PATH];
  get_user_config_folder(cfgdir, sizeof(cfgdir), FELIX_NAME);
  std::filesystem::path result = cfgdir;
  return result;
}

ConfigProvider gConfigProvider;
