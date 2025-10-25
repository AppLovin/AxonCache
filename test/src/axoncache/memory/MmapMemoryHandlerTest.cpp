// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/memory/MmapMemoryHandler.h>
#include "doctest/doctest.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheHeader.h"

using namespace axoncache;

TEST_CASE( "MmapMemoryHandlerTest" )
{
    CacheHeader header{};
    [[maybe_unused]] MmapMemoryHandler handler( header, "" );
}
