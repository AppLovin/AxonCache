// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <axoncache/version.h>
#include "doctest/doctest.h"

TEST_CASE( "AxonCache version" )
{
    static_assert( std::string_view( AXONCACHE_VERSION ) == std::string_view( "2.5" ) );
    CHECK( std::string( AXONCACHE_VERSION ) == std::string( "2.5" ) );
}
