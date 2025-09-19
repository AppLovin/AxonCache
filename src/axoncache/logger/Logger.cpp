// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/logger/Logger.h"

using namespace axoncache;

// Default do a no-op logger
Logger::LogFunc Logger::mCurrentLogger = []( const char *, LogLevel ) {};

void Logger::log( const char * msg, LogLevel level )
{
    mCurrentLogger( msg, level );
}

void Logger::info( const std::string & msg )
{
    mCurrentLogger( msg.c_str(), LogLevel::INFO );
}

void Logger::warn( const std::string & msg )
{
    mCurrentLogger( msg.c_str(), LogLevel::WARNING );
}

void Logger::error( const std::string & msg )
{
    mCurrentLogger( msg.c_str(), LogLevel::ERROR );
}
