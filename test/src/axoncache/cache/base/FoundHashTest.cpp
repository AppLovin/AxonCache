// SPDX-License-Identifier: MIT
// Copyright (c) 2026 AppLovin. All rights reserved.

// Tests for the `foundHash` out-parameter exposed on the HashedCacheBase read
// path. A lookup optionally reports the version-stable entry hash the read
// already computed (HashAlgo::hash(key)) so callers can track cross-version
// cache usage without a second hash+probe. For LinearProbe the getter writes 0
// when the key is absent (the "no key, do not mark" sentinel); for SimpleProbe/
// chained caches the probe returns a candidate bucket even on a miss, so the
// hash is reported regardless. See HashedCacheBase::setFoundHash.

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
#include <axoncache/cache/hasher/Xxh3Hasher.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include "doctest/doctest.h"
#include "CacheTestUtils.h"

using namespace axoncache;

namespace
{
constexpr uint16_t kOffsetBits = 35U;
// A sentinel distinct from both a valid hash and the not-found value (0), so we
// can assert the getter actually wrote the out-parameter rather than left it.
constexpr uint64_t kUnset = 0xDEADBEEFDEADBEEFULL;

auto expectedHash( std::string_view key ) -> uint64_t
{
    return Xxh3Hasher::hash( key );
}
}

TEST_CASE( "FoundHashLinearProbeExistingAndMissing" )
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
        uint64_t hash = kUnset;
        const auto retValue = cache.get( key, {}, &hash );
        CHECK( retValue == std::string_view{ value } );
        CHECK( hash == expectedHash( key ) );
        CHECK( hash != 0U );
    }

    // A missing key writes 0 (not left as the sentinel).
    uint64_t hash = kUnset;
    const auto retValue = cache.get( std::string_view{ "does-not-exist" }, {}, &hash );
    CHECK( retValue.empty() );
    CHECK( hash == 0U );
}

TEST_CASE( "FoundHashLinearProbeDistinctKeysDistinctHashes" )
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

    // Distinct keys hash to distinct 64-bit values (no collisions expected at this scale),
    // and each reported hash equals the key's XXH3.
    std::set<uint64_t> hashes;
    for ( const auto & [key, value] : strMap )
    {
        uint64_t hash = kUnset;
        const auto retValue = cache.get( key, {}, &hash );
        CHECK( retValue == std::string_view{ value } );
        CHECK( hash == expectedHash( key ) );
        hashes.insert( hash );
    }
    CHECK( hashes.size() == strMap.size() );
}

TEST_CASE( "FoundHashLinearProbeConsistentAcrossGetters" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    // Every read method that can locate "skey" must agree on the hash.
    uint64_t fromGet = kUnset;
    ( void )cache.get( std::string_view{ "skey" }, {}, &fromGet );

    uint64_t fromGetString = kUnset;
    ( void )cache.getString( std::string_view{ "skey" }, {}, &fromGetString );

    uint64_t fromGetWithType = kUnset;
    ( void )cache.getWithType( std::string_view{ "skey" }, &fromGetWithType );

    uint64_t fromContains = kUnset;
    ( void )cache.contains( std::string_view{ "skey" }, &fromContains );

    uint64_t fromReadKey = kUnset;
    ( void )cache.readKey( std::string_view{ "skey" }, &fromReadKey );

    uint64_t fromReadKeys = kUnset;
    ( void )cache.readKeys( std::string_view{ "skey" }, &fromReadKeys );

    uint64_t fromGetKeyType = kUnset;
    ( void )cache.getKeyType( std::string_view{ "skey" }, &fromGetKeyType );

    CHECK( fromGet == expectedHash( "skey" ) );
    CHECK( fromGetString == fromGet );
    CHECK( fromGetWithType == fromGet );
    CHECK( fromContains == fromGet );
    CHECK( fromReadKey == fromGet );
    CHECK( fromReadKeys == fromGet );
    CHECK( fromGetKeyType == fromGet );
}

TEST_CASE( "FoundHashLinearProbeGetStringAndGetKeyType" )
{
    // getString and getKeyType report the hash so that marking can stay liberal
    // (mark on any read that locates the key).
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    uint64_t getStringHash = kUnset;
    const auto [str, found] = cache.getString( std::string_view{ "skey" }, {}, &getStringHash );
    CHECK( found );
    CHECK( str == std::string_view{ "hello" } );
    CHECK( getStringHash == expectedHash( "skey" ) );

    uint64_t keyTypeHash = kUnset;
    const auto keyType = cache.getKeyType( std::string_view{ "skey" }, &keyTypeHash );
    CHECK( keyType == "String" );
    CHECK( keyTypeHash == getStringHash );

    // Missing key: both report 0.
    uint64_t missGetStringHash = kUnset;
    const auto [missStr, missFound] = cache.getString( std::string_view{ "nope" }, {}, &missGetStringHash );
    CHECK( !missFound );
    CHECK( missStr.empty() );
    CHECK( missGetStringHash == 0U );

    uint64_t missKeyTypeHash = kUnset;
    const auto missKeyType = cache.getKeyType( std::string_view{ "nope" }, &missKeyTypeHash );
    CHECK( missKeyType.empty() );
    CHECK( missKeyTypeHash == 0U );
}

TEST_CASE( "FoundHashLinearProbeTypedGetters" )
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

    {
        uint64_t hash = kUnset;
        CHECK( cache.getBool( std::string_view{ "bkey" }, false, &hash ).first == true );
        CHECK( hash == expectedHash( "bkey" ) );
    }
    {
        uint64_t hash = kUnset;
        CHECK( cache.getInt64( std::string_view{ "nkey" }, 0, &hash ).first == 42 );
        CHECK( hash == expectedHash( "nkey" ) );
    }
    {
        uint64_t hash = kUnset;
        CHECK( cache.getDouble( std::string_view{ "dkey" }, 0, &hash ).first == 3.14 );
        CHECK( hash == expectedHash( "dkey" ) );
    }
    {
        uint64_t hash = kUnset;
        CHECK( cache.getFloatVector( std::string_view{ "fkey" }, &hash ) == floatValue );
        CHECK( hash == expectedHash( "fkey" ) );
    }
    {
        uint64_t hash = kUnset;
        CHECK( cache.getFloatAtIndex( std::string_view{ "fkey" }, 1, &hash ) == 2.0F );
        CHECK( hash == expectedHash( "fkey" ) );
    }
    {
        uint64_t hash = kUnset;
        const std::vector<int32_t> indices{ 0, 2 };
        const std::vector<float> expected{ 1.0F, 2.5F };
        CHECK( cache.getFloatAtIndices( std::string_view{ "fkey" }, indices, &hash ) == expected );
        CHECK( hash == expectedHash( "fkey" ) );
    }
    {
        uint64_t hash = kUnset;
        const auto span = cache.getFloatSpan( std::string_view{ "fkey" }, &hash );
        CHECK( span.size() == floatValue.size() );
        CHECK( hash == expectedHash( "fkey" ) );
    }
    {
        uint64_t hash = kUnset;
        CHECK( cache.getVector( std::string_view{ "vkey" }, {}, &hash ) == vectorValue );
        CHECK( hash == expectedHash( "vkey" ) );
    }

    // Each typed getter writes 0 for a missing key.
    uint64_t hash = kUnset;
    CHECK( cache.getBool( std::string_view{ "nope" }, false, &hash ).second == false );
    CHECK( hash == 0U );
    hash = kUnset;
    CHECK( cache.getInt64( std::string_view{ "nope" }, 0, &hash ).second == false );
    CHECK( hash == 0U );
    hash = kUnset;
    CHECK( cache.getDouble( std::string_view{ "nope" }, 0, &hash ).second == false );
    CHECK( hash == 0U );
    hash = kUnset;
    CHECK( cache.getFloatVector( std::string_view{ "nope" }, &hash ).empty() );
    CHECK( hash == 0U );
    hash = kUnset;
    CHECK( cache.getVector( std::string_view{ "nope" }, {}, &hash ).empty() );
    CHECK( hash == 0U );
}

TEST_CASE( "FoundHashLinearProbeLiberalMarkingOnTypeMismatch" )
{
    // The hash is reported because the KEY is present, independent of whether
    // the requested value type matches what is stored. This is what lets the
    // usage tracker mark liberally.
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    int64_t int64Value = 42;
    cache.put( std::string_view{ "num" }, int64Value );

    // Read an Int64-typed key through the String getter: the value does not
    // match, but the hash is still reported.
    uint64_t mismatchHash = kUnset;
    ( void )cache.get( std::string_view{ "num" }, {}, &mismatchHash );
    CHECK( mismatchHash == expectedHash( "num" ) );

    // And it matches the hash reported by the correctly-typed getter.
    uint64_t typedHash = kUnset;
    CHECK( cache.getInt64( std::string_view{ "num" }, 0, &typedHash ).first == 42 );
    CHECK( typedHash == mismatchHash );
}

TEST_CASE( "FoundHashLinearProbeContains" )
{
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    uint64_t hash = kUnset;
    CHECK( cache.contains( std::string_view{ "skey" }, &hash ) );
    CHECK( hash == expectedHash( "skey" ) );

    hash = kUnset;
    CHECK( !cache.contains( std::string_view{ "nope" }, &hash ) );
    CHECK( hash == 0U );
}

TEST_CASE( "FoundHashLinearProbeNullptrIsSafe" )
{
    // Backward compatibility: omitting the hash argument (the default nullptr)
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

TEST_CASE( "FoundHashDedupCacheExistingAndMissing" )
{
    // Exercises the LinearProbeDedupCache overrides of getInternal /
    // getWithTypeInternal, which also call setFoundHash.
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    LinearProbeDedupCache cache( kOffsetBits, numberOfKeySlots, 0.5, std::move( memoryHandler ), CacheType::LINEAR_PROBE );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    uint64_t getHash = kUnset;
    CHECK( cache.get( std::string_view{ "skey" }, {}, &getHash ) == std::string_view{ "hello" } );
    CHECK( getHash == expectedHash( "skey" ) );

    uint64_t withTypeHash = kUnset;
    ( void )cache.getWithType( std::string_view{ "skey" }, &withTypeHash );
    CHECK( withTypeHash == getHash );

    uint64_t missHash = kUnset;
    CHECK( cache.get( std::string_view{ "nope" }, {}, &missHash ).empty() );
    CHECK( missHash == 0U );
}

TEST_CASE( "FoundHashBucketChainExistingAndMissing" )
{
    // The hash is also reported for SimpleProbe-backed caches, but the miss
    // semantics differ from LinearProbe. SimpleProbe::findKeySlotOffset returns
    // the candidate bucket directly (it never returns AXONCACHE_KEY_NOT_FOUND);
    // presence is decided later by walking the chain. So setFoundHash always
    // writes the key's hash -- the 0-on-miss sentinel is a LinearProbe-only
    // guarantee. This is the same caveat the old foundSlot had. The multicache
    // usage path only uses LinearProbe, so it never relies on this.
    const uint16_t offsetBits = 64U;
    const auto numberOfKeySlots = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeySlots * sizeof( uint64_t ) );
    BucketChainCache cache( offsetBits, numberOfKeySlots, 1.0, std::move( memoryHandler ) );

    cache.put( std::string_view{ "skey" }, std::string_view{ "hello" } );

    uint64_t getHash = kUnset;
    CHECK( cache.get( std::string_view{ "skey" }, {}, &getHash ) == std::string_view{ "hello" } );
    CHECK( getHash == expectedHash( "skey" ) );

    uint64_t containsHash = kUnset;
    CHECK( cache.contains( std::string_view{ "skey" }, &containsHash ) );
    CHECK( containsHash == getHash );

    // A missing key is still a genuine miss (empty value), but SimpleProbe
    // reports the key's hash rather than the 0 sentinel.
    uint64_t missHash = kUnset;
    CHECK( cache.get( std::string_view{ "nope" }, {}, &missHash ).empty() );
    CHECK( missHash == expectedHash( "nope" ) );
}
