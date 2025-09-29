// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/cache/factory/CacheFactory.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

TEST_CASE( "CacheFactoryTest" )
{
    [[maybe_unused]] CacheFactory factory{};
}
