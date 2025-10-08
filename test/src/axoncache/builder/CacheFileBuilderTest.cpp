// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/builder/CacheFileBuilder.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include <stdexcept>
#include <string>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/cache/CacheBase.h"
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

TEST_CASE( "CacheFileBuilderTest" )
{
    axoncache::SharedSettingsProvider settings( "" );
    CHECK_THROWS_AS( CacheFileBuilder builder( &settings, "", "", {}, nullptr ), std::runtime_error ); // exception if no data file specified
}
