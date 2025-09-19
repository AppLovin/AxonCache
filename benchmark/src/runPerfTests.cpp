// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <benchmark/benchmark.h>
#include <axoncache/Constants.h>
#include "axoncache/logger/Logger.h"
#include <iostream>

auto main( int argc, char ** argv ) -> int
{
    ::benchmark::Initialize( &argc, argv );
    if ( ::benchmark::ReportUnrecognizedArguments( argc, argv ) )
    {
        return 1;
    }

    auto alcacheLogger = []( const char * msg, const axoncache::LogLevel & level )
    {
        switch ( level )
        {
            case axoncache::LogLevel::INFO:
                std::cout << msg << "\n";
                break;
            case axoncache::LogLevel::WARNING:
                std::cerr << msg << "\n";
                break;
            case axoncache::LogLevel::ERROR:
                std::cerr << msg << "\n";
                break;
        }
    };
    axoncache::Logger::setLogFunction( alcacheLogger );

    ::benchmark::RunSpecifiedBenchmarks();
    return 0;
}
