// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

// define has to happen before the doctest include
#define DOCTEST_CONFIG_IMPLEMENT

#include <doctest/doctest.h>

#include <spdlog/spdlog.h>
#include <axoncache/Constants.h>
#include "axoncache/logger/Logger.h"

int main( int argc, char ** argv )
{
    doctest::Context context;

    auto alcacheLogger = []( const char * msg, const axoncache::LogLevel & level )
    {
        switch ( level )
        {
            case axoncache::LogLevel::INFO:
                SPDLOG_INFO( msg );
                break;
            case axoncache::LogLevel::WARNING:
                SPDLOG_WARN( msg );
                break;
            case axoncache::LogLevel::ERROR:
                SPDLOG_ERROR( msg );
                break;
        }
    };
    axoncache::Logger::setLogFunction( alcacheLogger );
    spdlog::set_level( spdlog::level::info );

    context.setOption( "abort-after", 10 );  // stop test execution after 5 failed assertions
    context.setOption( "order-by", "name" ); // sort the test cases by their name

    context.applyCommandLine( argc, argv );

    // overrides
    context.setOption( "no-breaks", false ); // don't break in the debugger when assertions fail

    int res = context.run(); // run

    if ( context.shouldExit() ) // important - query flags (and --exit) rely on the user doing this
    {
        return res; // propagate the result of the tests
    }

    return res; // the result from doctest is propagated here as well
}
