// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/memory/MallocMemoryHandler.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

TEST_CASE( "MallocMemoryHandlerTest" )
{
    [[maybe_unused]] MallocMemoryHandler handler;
}
