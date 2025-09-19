// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once
#include <functional>
#include <string>

namespace axoncache
{
enum class LogLevel
{
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger
{
  public:
    using LogFunc = std::function<void( const char *, const LogLevel & level )>;

    static void log( const char * msg, LogLevel level = LogLevel::INFO );

    static void info( const std::string & msg );
    static void warn( const std::string & msg );
    static void error( const std::string & msg );
    static void critical( const std::string & msg );

    static void setLogFunction( LogFunc && func )
    {
        mCurrentLogger = std::move( func );
    }

  private:
    static LogFunc mCurrentLogger;
};

#define AL_LOG_INFO( msg ) axoncache::Logger::info( msg );

#define AL_LOG_WARN( msg ) axoncache::Logger::warn( msg );

#define AL_LOG_ERROR( msg ) axoncache::Logger::error( msg );

}
