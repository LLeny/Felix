#include "pch.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"

ConfigProvider::ConfigProvider() : mAppDataFolder{ obtainAppDataFolder() }, mSysConfig{}
{
  L_INFO << "ConfigProvider '" << mAppDataFolder << "'";
  std::filesystem::create_directories( mAppDataFolder );
  mSysConfig = SysConfig::load( mAppDataFolder );
  lock();
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
  get_user_config_folder( cfgdir, sizeof( cfgdir ), FELIX_NAME );
  std::filesystem::path result = cfgdir;
  return result;
}

void ConfigProvider::lock()
{
  errno = 0;
  auto lockFilePath = ConfigProvider::obtainAppDataFolder() / "lock.tmp";
  int fd = open( lockFilePath.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR );

  if ( fd == -1 )
  {
    L_INFO << "Couldn't  acquire lock on '" << lockFilePath << "'";
    mIsLocked = false;
    return;
  }

  if ( lockf( fd, F_TLOCK, 0 ) )
  {
    mIsLocked = false;
    return;
  }

  mIsLocked = true;
}

bool ConfigProvider::isLocked()
{
  return mIsLocked;
}

ConfigProvider gConfigProvider;
