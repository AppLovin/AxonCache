// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

// define has to happen before the doctest include
#define DOCTEST_CONFIG_IMPLEMENT

#include "doctest/doctest.h"

#include <iostream>
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
                std::cout << msg << '\n';
                break;
            case axoncache::LogLevel::WARNING:
                std::cerr << msg << '\n';
                break;
            case axoncache::LogLevel::ERROR:
                std::cerr << msg << '\n';
                break;
        }
    };
    axoncache::Logger::setLogFunction( alcacheLogger );

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
