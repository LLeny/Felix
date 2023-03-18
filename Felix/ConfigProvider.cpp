#include "pch.hpp"
#include "ConfigProvider.hpp"
#include "SysConfig.hpp"
#ifdef WIN32
#include <windows.h>
#endif

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

#ifdef WIN32
  HANDLE hFile = CreateFile( (LPCSTR)lockFilePath.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
  auto dwLastError = GetLastError();

  if( hFile != NULL && hFile != INVALID_HANDLE_VALUE )
  {
    mIsLocked = true;
    return;
  }
  else if(dwLastError == ERROR_FILE_NOT_FOUND )
  {
    hFile = CreateFile( (LPCSTR)lockFilePath.c_str(), GENERIC_READ, 0, NULL, CREATE_NEW, 0, NULL );
    auto dwLastError = GetLastError();
    mIsLocked = hFile != NULL && hFile != INVALID_HANDLE_VALUE;
    return;
  }

  mIsLocked = false;
  return;
#else
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
#endif

}

bool ConfigProvider::isLocked()
{
  return mIsLocked;
}

ConfigProvider gConfigProvider;
