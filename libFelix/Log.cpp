#include "pch.hpp"
#include "Log.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

Log::Log() : mLogLevel{ LL_INFO }
{
}

void Log::setLogLevel( LogLevel ll )
{
  mLogLevel = ll;
}

void Log::log( LogLevel ll, std::string const & message )
{
#ifdef _WIN32
    OutputDebugStringA( message.c_str() );
#else
  if ( ll >= mLogLevel )
  {
    static bool err = false;
    if ( ll >= LL_ERROR )
    {
     if ( !err )
     {
       std::cout.flush();
     }
     std::cerr << message;
     err = true;
    }
    else
    {
     if ( err )
     {
       std::cerr.flush();
     }
     std::cout << message;
     err = false;
    }
  }
#endif
}

Log & Log::instance()
{
  static Log instance{};
  return instance;
}

Formatter::Formatter( Log::LogLevel ll ) : mLl{ ll }, mSS{}
{
}

Formatter::~Formatter()
{
  mSS << std::endl;
  Log::instance().log( mLl, mSS.str() );
}
