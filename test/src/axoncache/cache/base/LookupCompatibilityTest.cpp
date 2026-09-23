// SPDX-License-Identifier: MIT
// Copyright (c) 2026 AppLovin. All rights reserved.

// Keep this test limited to lookup APIs available in v1.0.11. Besides guarding
// current behavior, the same source can be compiled at that tag to compare the
// results across the lookup optimizations introduced through v1.0.22.

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <axoncache/cache/CacheType.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/cache/hasher/Xxh3Hasher.h>
#include <axoncache/domain/CacheValue.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include "doctest/doctest.h"

using namespace axoncache;

namespace
{
auto keysForSlot( uint64_t slot, uint64_t numberOfKeySlots, size_t count ) -> std::vector<std::string>
{
    std::vector<std::string> keys;
    for ( uint64_t candidate = 0; keys.size() < count; ++candidate )
    {
        auto key = "wrap-key-" + std::to_string( candidate );
        if ( Xxh3Hasher::hash( key ) % numberOfKeySlots == slot )
        {
            keys.emplace_back( std::move( key ) );
        }
    }
    return keys;
}
}

TEST_CASE( "LinearProbeDedupLookupCompatibilityAcrossWraparound" )
{
    // A non-power-of-two slot count exercises the explicit wraparound without
    // accidentally relying on a bit mask. All keys start in the final slot, so
    // every lookup after the first must cross the end of the key space.
    constexpr uint64_t numberOfKeySlots = 17;
    constexpr uint16_t offsetBits = 35;
    const auto keys = keysForSlot( numberOfKeySlots - 1, numberOfKeySlots, 7 );

    auto memory = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeDedupCache cache( offsetBits,
                                 numberOfKeySlots,
                                 0.5,
                                 std::move( memory ),
                                 CacheType::LINEAR_PROBE_DEDUP_TYPED );
    cache.setDuplicatedValues( { "shared-value" } );

    bool boolValue = true;
    int64_t intValue = -123456789;
    double doubleValue = 6.25;
    const std::vector<std::string_view> listValue{ "first", "second", "third" };

    CHECK( cache.put( keys[0], "shared-value" ).second == 0 );
    CHECK( cache.put( keys[1], "direct-value" ).second == 1 );
    CHECK( cache.put( keys[2], intValue ).second == 2 );
    CHECK( cache.put( keys[3], doubleValue ).second == 3 );
    CHECK( cache.put( keys[4], boolValue ).second == 4 );
    CHECK( cache.put( keys[5], listValue ).second == 5 );
    CHECK( cache.put( keys[6], " +12.5" ).second == 6 );

    const auto [dedupString, dedupFound] = cache.getString( keys[0] );
    CHECK( dedupFound );
    CHECK( dedupString == "shared-value" );

    const auto [directString, directFound] = cache.getString( keys[1] );
    CHECK( directFound );
    CHECK( directString == "direct-value" );

    CHECK( cache.getInt64( keys[2] ) == std::make_pair( intValue, true ) );
    CHECK( cache.getDouble( keys[3] ) == std::make_pair( doubleValue, true ) );
    CHECK( cache.getBool( keys[4] ) == std::make_pair( boolValue, true ) );
    CHECK( cache.getVector( keys[5] ) == listValue );
    CHECK( cache.getDouble( keys[6] ) == std::make_pair( 12.5, true ) );

    CHECK( cache.getWithType( keys[0] ).second == CacheValueType::String );
    CHECK( cache.getWithType( keys[2] ).second == CacheValueType::Int64 );
    CHECK( cache.getWithType( keys[3] ).second == CacheValueType::Double );
    CHECK( cache.getWithType( keys[4] ).second == CacheValueType::Bool );
    CHECK( cache.getWithType( keys[5] ).second == CacheValueType::StringList );
    CHECK( cache.getWithType( keys[6] ).second == CacheValueType::String );

    CHECK( cache.contains( keys[0] ) );
    CHECK( cache.contains( keys[5] ) );
    CHECK_FALSE( cache.contains( "missing-key" ) );
    CHECK( cache.getString( "missing-key", "fallback" ) == std::make_pair( std::string_view{ "fallback" }, false ) );
    CHECK( cache.getInt64( "missing-key", 42 ) == std::make_pair( int64_t{ 42 }, false ) );
    CHECK( cache.getDouble( "missing-key", 2.5 ) == std::make_pair( 2.5, false ) );
}
