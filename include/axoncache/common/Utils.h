// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/common/StringUtils.h"
#include <ctime>
#include <sstream>
#include <fstream>

namespace axoncache
{

class Utils : public StringUtils
{
  public:
    static auto currentTimeMillis() -> int64_t
    {
        struct timespec curTime;
        clock_gettime( CLOCK_REALTIME, &curTime ); // signal-safe func, VS std::chrono::system_clock::now()

        return ( curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000 );
    }

    static auto readFile( const std::string & filename ) -> std::string
    {
        std::ifstream file( filename, std::ios::in | std::ios::binary );
        if ( !file )
        {
            return {};
        }

        std::ostringstream oss;
        oss << file.rdbuf(); // Read entire file into the stream
        return oss.str();
    }

    static void writeFile( const std::string & filename, const std::string & content )
    {
        std::ofstream out( filename, std::ios::out | std::ios::trunc );
        if ( !out )
        {
            throw std::runtime_error( "Failed to open file for writing: " + filename );
        }
        out << content;
        if ( !out )
        {
            throw std::runtime_error( "Failed to write to file: " + filename );
        }
    }
};

}
