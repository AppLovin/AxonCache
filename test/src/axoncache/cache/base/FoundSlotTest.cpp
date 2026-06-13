// SPDX-License-Identifier: MIT
// Copyright (c) 2026 AppLovin. All rights reserved.

// Tests for the `foundSlot` out-parameter exposed on the HashedCacheBase read
// path. A lookup optionally reports which key slot the key resolved to (or -1
// when the key is absent), so callers can record cache usage per slot without a
// second hash+probe. See HashedCacheBase::setFoundSlot.

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <vector>
#include <set>
#include <cstdint>
#include <axoncache/Constants.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/cache/CacheType.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include "doctest/doctest.h"
#include "CacheTestUtils.h"

using namespace axoncache;

namespace
{
constexpr uint16_t kOffsetBits = 35U;
// A sentinel distinct from both a valid slot (>= 0) and the not-found value (-1),
// so we can assert the getter actually wrote the out-parameter rather than left it.
constexpr int64_t kUnset = -999;
}

TEST_CASE( "FoundSlotLinearProbeExistingAndMissing" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    const std::vector<std::pair<std::string, std::string>> kv = {
        { "alpha", "1" }, { "bravo", "2" }, { "charlie", "3" }, { "delta", "4" }, { "echo", "5" } };
    for ( const auto & [key, value] : kv )
    {
        cache.put( key, value );
    }

    for ( const auto & [key, value] : kv )
    {
        int64_t slot = kUnset;
        const auto retValue = cache.get( key, {}, &slot );
        CHECK( retValue == std::string_view{ value } );
        CHECK( slot >= 0 );
        CHECK( slot < static_cast<int64_t>( cache.numberOfKeySlots() ) );
    }

    // A missing key writes -1 (not left as the sentinel).
    int64_t slot = kUnset;
    const auto retValue = cache.get( std::string_view{ "does-not-exist" }, {}, &slot );
    CHECK( retValue.empty() );
    CHECK( slot == -1 );
}

TEST_CASE( "FoundSlotLinearProbeDistinctKeysDistinctSlots" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    const auto numberOfKeysToTest = cache.maxNumberEntries();
    const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );
    for ( const auto & [key, value] : strMap )
    {
        cache.put( key, value );
    }

    // In a linear-probe table every key occupies its own slot, so the reported
    // slots must all be distinct and in range.
    std::set<int64_t> slots;
    for ( const auto & [key, value] : strMap )
    {
        int64_t slot = kUnset;
        const auto retValue = cache.get( key, {}, &slot );
        CHECK( retValue == std::string_view{ value } );
        CHECK( slot >= 0 );
        CHECK( slot < static_cast<int64_t>( cache.numberOfKeySlots() ) );
        slots.insert( slot );
    }
    CHECK( slots.size() == strMap.size() );
}

TEST_CASE( "FoundSlotLinearProbeConsistentAcrossGetters" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    // Every read method that can locate "skey" must agree on the slot.
    int64_t fromGet = kUnset;
    ( void )cache.get( std::string_view{ "skey" }, {}, &fromGet );

    int64_t fromGetString = kUnset;
    ( void )cache.getString( std::string_view{ "skey" }, {}, &fromGetString );

    int64_t fromGetWithType = kUnset;
    ( void )cache.getWithType( std::string_view{ "skey" }, &fromGetWithType );

    int64_t fromContains = kUnset;
    ( void )cache.contains( std::string_view{ "skey" }, &fromContains );

    int64_t fromReadKey = kUnset;
    ( void )cache.readKey( std::string_view{ "skey" }, &fromReadKey );

    int64_t fromReadKeys = kUnset;
    ( void )cache.readKeys( std::string_view{ "skey" }, &fromReadKeys );

    int64_t fromGetKeyType = kUnset;
    ( void )cache.getKeyType( std::string_view{ "skey" }, &fromGetKeyType );

    CHECK( fromGet >= 0 );
    CHECK( fromGetString == fromGet );
    CHECK( fromGetWithType == fromGet );
    CHECK( fromContains == fromGet );
    CHECK( fromReadKey == fromGet );
    CHECK( fromReadKeys == fromGet );
    CHECK( fromGetKeyType == fromGet );
}

TEST_CASE( "FoundSlotLinearProbeGetStringAndGetKeyType" )
{
    // getString and getKeyType were extended to report the slot specifically so
    // that marking can stay liberal (mark on any read, prune conservatively).
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    int64_t getStringSlot = kUnset;
    const auto [str, found] = cache.getString( std::string_view{ "skey" }, {}, &getStringSlot );
    CHECK( found );
    CHECK( str == std::string_view{ "hello" } );
    CHECK( getStringSlot >= 0 );

    int64_t keyTypeSlot = kUnset;
    const auto keyType = cache.getKeyType( std::string_view{ "skey" }, &keyTypeSlot );
    CHECK( keyType == "String" );
    CHECK( keyTypeSlot == getStringSlot );

    // Missing key: both report -1.
    int64_t missGetStringSlot = kUnset;
    const auto [missStr, missFound] = cache.getString( std::string_view{ "nope" }, {}, &missGetStringSlot );
    CHECK( !missFound );
    CHECK( missStr.empty() );
    CHECK( missGetStringSlot == -1 );

    int64_t missKeyTypeSlot = kUnset;
    const auto missKeyType = cache.getKeyType( std::string_view{ "nope" }, &missKeyTypeSlot );
    CHECK( missKeyType.empty() );
    CHECK( missKeyTypeSlot == -1 );
}

TEST_CASE( "FoundSlotLinearProbeTypedGetters" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    bool boolValue = true;
    int64_t int64Value = 42;
    double doubleValue = 3.14;
    std::vector<float> floatValue{ 1.0F, 2.0F, 2.5F };
    std::vector<std::string_view> vectorValue{ "a", "b" };
    cache.put( std::string_view{ "bkey" }, boolValue );
    cache.put( std::string_view{ "nkey" }, int64Value );
    cache.put( std::string_view{ "dkey" }, doubleValue );
    cache.put( std::string_view{ "fkey" }, floatValue );
    cache.put( std::string_view{ "vkey" }, vectorValue );

    const auto inRange = [&]( int64_t slot ) { return slot >= 0 && slot < static_cast<int64_t>( cache.numberOfKeySlots() ); };

    {
        int64_t slot = kUnset;
        CHECK( cache.getBool( std::string_view{ "bkey" }, false, &slot ).first == true );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        CHECK( cache.getInt64( std::string_view{ "nkey" }, 0, &slot ).first == 42 );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        CHECK( cache.getDouble( std::string_view{ "dkey" }, 0, &slot ).first == 3.14 );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        CHECK( cache.getFloatVector( std::string_view{ "fkey" }, &slot ) == floatValue );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        CHECK( cache.getFloatAtIndex( std::string_view{ "fkey" }, 1, &slot ) == 2.0F );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        const std::vector<int32_t> indices{ 0, 2 };
        const std::vector<float> expected{ 1.0F, 2.5F };
        CHECK( cache.getFloatAtIndices( std::string_view{ "fkey" }, indices, &slot ) == expected );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        const auto span = cache.getFloatSpan( std::string_view{ "fkey" }, &slot );
        CHECK( span.size() == floatValue.size() );
        CHECK( inRange( slot ) );
    }
    {
        int64_t slot = kUnset;
        CHECK( cache.getVector( std::string_view{ "vkey" }, {}, &slot ) == vectorValue );
        CHECK( inRange( slot ) );
    }

    // Each typed getter writes -1 for a missing key.
    int64_t slot = kUnset;
    CHECK( cache.getBool( std::string_view{ "nope" }, false, &slot ).second == false );
    CHECK( slot == -1 );
    slot = kUnset;
    CHECK( cache.getInt64( std::string_view{ "nope" }, 0, &slot ).second == false );
    CHECK( slot == -1 );
    slot = kUnset;
    CHECK( cache.getDouble( std::string_view{ "nope" }, 0, &slot ).second == false );
    CHECK( slot == -1 );
    slot = kUnset;
    CHECK( cache.getFloatVector( std::string_view{ "nope" }, &slot ).empty() );
    CHECK( slot == -1 );
    slot = kUnset;
    CHECK( cache.getVector( std::string_view{ "nope" }, {}, &slot ).empty() );
    CHECK( slot == -1 );
}

TEST_CASE( "FoundSlotLinearProbeLiberalMarkingOnTypeMismatch" )
{
    // The slot is reported because the KEY is present, independent of whether
    // the requested value type matches what is stored. This is what lets the
    // usage tracker mark liberally (and prune conservatively).
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    int64_t int64Value = 42;
    cache.put( std::string_view{ "num" }, int64Value );

    // Read an Int64-typed key through the String getter: the value does not
    // match, but the slot is still reported.
    int64_t mismatchSlot = kUnset;
    ( void )cache.get( std::string_view{ "num" }, {}, &mismatchSlot );
    CHECK( mismatchSlot >= 0 );
    CHECK( mismatchSlot < static_cast<int64_t>( cache.numberOfKeySlots() ) );

    // And it matches the slot reported by the correctly-typed getter.
    int64_t typedSlot = kUnset;
    CHECK( cache.getInt64( std::string_view{ "num" }, 0, &typedSlot ).first == 42 );
    CHECK( typedSlot == mismatchSlot );
}

TEST_CASE( "FoundSlotLinearProbeContains" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    int64_t slot = kUnset;
    CHECK( cache.contains( std::string_view{ "skey" }, &slot ) );
    CHECK( slot >= 0 );
    CHECK( slot < static_cast<int64_t>( cache.numberOfKeySlots() ) );

    slot = kUnset;
    CHECK( !cache.contains( std::string_view{ "nope" }, &slot ) );
    CHECK( slot == -1 );
}

TEST_CASE( "FoundSlotLinearProbeNullptrIsSafe" )
{
    // Backward compatibility: omitting the slot argument (the default nullptr)
    // leaves the existing behavior unchanged.
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    CHECK( cache.get( std::string_view{ "skey" } ) == std::string_view{ "hello" } );
    CHECK( cache.getString( std::string_view{ "skey" } ).first == std::string_view{ "hello" } );
    CHECK( cache.contains( std::string_view{ "skey" } ) );
    CHECK( cache.getKeyType( std::string_view{ "skey" } ) == "String" );
    CHECK( cache.get( std::string_view{ "nope" } ).empty() );
}

TEST_CASE( "FoundSlotDedupCacheExistingAndMissing" )
{
    // Exercises the LinearProbeDedupCache overrides of getInternal /
    // getWithTypeInternal, which also call setFoundSlot.
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeDedupCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ), CacheType::LINEAR_PROBE );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    int64_t getSlot = kUnset;
    CHECK( cache.get( std::string_view{ "skey" }, {}, &getSlot ) == std::string_view{ "hello" } );
    CHECK( getSlot >= 0 );
    CHECK( getSlot < static_cast<int64_t>( cache.numberOfKeySlots() ) );

    int64_t withTypeSlot = kUnset;
    ( void )cache.getWithType( std::string_view{ "skey" }, &withTypeSlot );
    CHECK( withTypeSlot == getSlot );

    int64_t missSlot = kUnset;
    CHECK( cache.get( std::string_view{ "nope" }, {}, &missSlot ).empty() );
    CHECK( missSlot == -1 );
}

TEST_CASE( "FoundSlotBucketChainExistingAndMissing" )
{
    // The slot is also reported for SimpleProbe-backed caches, but the
    // semantics differ from LinearProbe. SimpleProbe::findKeySlotOffset returns
    // the hash bucket directly (it never returns AXONCACHE_KEY_NOT_FOUND);
    // presence is decided later by walking the chain. So the reported slot is
    // the candidate bucket the key hashes to, which is always in range and is
    // NOT necessarily unique across keys. The -1 sentinel is a LinearProbe-only
    // guarantee. We therefore assert range and cross-getter consistency, not -1.
    const uint16_t offsetBits = 64U;
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    BucketChainCache cache( offsetBits, numberOfKeySlots, 1.0, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    int64_t getSlot = kUnset;
    CHECK( cache.get( std::string_view{ "skey" }, {}, &getSlot ) == std::string_view{ "hello" } );
    CHECK( getSlot >= 0 );
    CHECK( getSlot < static_cast<int64_t>( cache.numberOfKeySlots() ) );

    int64_t containsSlot = kUnset;
    CHECK( cache.contains( std::string_view{ "skey" }, &containsSlot ) );
    CHECK( containsSlot == getSlot );

    // A missing key is still a genuine miss (empty value), but SimpleProbe
    // reports the candidate hash bucket rather than -1.
    int64_t missSlot = kUnset;
    CHECK( cache.get( std::string_view{ "nope" }, {}, &missSlot ).empty() );
    CHECK( missSlot != kUnset ); // the out-parameter was written
    CHECK( missSlot >= 0 );      // candidate bucket, not the LinearProbe -1 sentinel
    CHECK( missSlot < static_cast<int64_t>( cache.numberOfKeySlots() ) );
}
