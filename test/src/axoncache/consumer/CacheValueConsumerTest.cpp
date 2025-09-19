// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string_view>
#include <memory>
#include <utility>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/MapCache.h>
#include <axoncache/consumer/CacheValueConsumer.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include <vector>
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheValue.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

TEST_CASE( "CacheValueConsumerTest" )
{
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( 1024 * sizeof( uint64_t ) );
    MapCache cache( std::move( memoryHandler ) );

    auto value1 = CacheValue{ "world" };
    auto value2 = CacheValue{ std::vector<std::string_view>{ "is", "a", "test" } };
    auto keyValue1 = std::make_pair( "hello", value1 );
    auto keyValue2 = std::make_pair( "this", value2 );

    CacheValueConsumer consumer( &cache );
    consumer.consumeValue( keyValue1 );
    consumer.consumeValue( keyValue2 );

    CHECK( cache.numberOfEntries() == 2 );
    CHECK( cache.get( "hello" ) == "world" );
    auto vec = cache.getVector( "this" );
    CHECK( vec.size() == 3 );
    CHECK( vec[0] == "is" );
    CHECK( vec[1] == "a" );
    CHECK( vec[2] == "test" );
}

TEST_CASE( "CacheValueConsumerNoneTest" )
{
    const uint16_t offsetBits = 21U;
    const auto maxLoadFactor = 0.5f;
    LinearProbeCache cache( offsetBits, 100UL, maxLoadFactor, std::make_unique<MallocMemoryHandler>( 100UL ) );

    auto value1 = CacheValue{ "123" };
    auto keyValue1 = std::make_pair( "hello", value1 );

    CacheValueConsumer consumer( &cache );
    consumer.consumeValue( keyValue1 );
    CHECK( cache.get( "hello" ) == "123" );
}
