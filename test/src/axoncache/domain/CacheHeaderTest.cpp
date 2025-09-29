// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/Constants.h>
#include <axoncache/domain/CacheHeader.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include <cstring>
#include <sstream>
#include <string>
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

TEST_CASE( "CacheHeaderTestToString" )
{
    CacheHeader header;
    header.headerSize = sizeof( CacheHeader );
    header.nameStart = sizeof( CacheHeader ) - Constants::kMaxCacheNameSize;
    header.cacheType = 1U;
    header.hashcodeBits = 29,
    header.offsetBits = 35,
    header.hashFuncId = Constants::HashFuncId::XXH3;
    header.maxLoadFactor = 0.5,
    header.maxCollisions = 53,
    header.creationTimeMs = 12345UL;
    header.magicNumber = 42;
    header.numberOfEntries = 10;
    header.numberOfKeySlots = 20;
    header.dataSize = 800;
    header.size = 1024,
    header.version = 1;
    strcpy( header.cacheName, "test_cache" );

    std::stringstream ss;
    ss << header;
    CHECK( ss.str() == R"({"magic_number":42,"header_size":104,"name_start":72,"version":1,"cache_type":1,"hashcode_bits":29,"offset_bits":35,"hash_func_id":2,"max_collisions":53,"max_load_factor":0.5,"creation_time_ms":12345,"number_of_key_slots":20,"number_of_entries":10,"data_size":800,"size":1024,"cache_name":"test_cache"})" );
}
