// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <axoncache/Constants.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include <sstream>
#include <map>
#include <ostream>
#include <stdexcept>
#include <vector>
#include "CacheTestUtils.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

TEST_CASE( "HashedCacheBaseTest" )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );
    const auto loadFactor = 1.0;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, loadFactor, std::move( memoryHandler ) );

    for ( const auto & [key, value] : strMap )
    {
        cache.put( key, value );
    }

    CHECK( strMap.size() == cache.numberOfEntries() );
    for ( const auto & p : strMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.get( std::string_view{ key } ) == std::string_view{ value } );
    }
}

TEST_CASE( "HashedCacheBaseTestGetVector" )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = axoncache::test_utils::gen_random_str_vec_map( numberOfKeysToTest );
    const auto loadFactor = 1.0;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, loadFactor, std::move( memoryHandler ) );

    for ( const auto & [key, value] : strMap )
    {
        std::vector<std::string_view> vec;
        for ( const auto & v : value )
        {
            vec.push_back( v );
        }
        cache.put( key, vec );
    }

    for ( const auto & p : strMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        const auto vec = cache.getVector( std::string_view{ key } );
        CHECK( vec.size() == value.size() );
        for ( auto ix = 0U; ix < vec.size(); ++ix )
        {
            CHECK( vec[ix] == value[ix] );
        }
    }
}

TEST_CASE( "HashedCacheBaseTestGetLongVector" )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto bigStrMap = axoncache::test_utils::gen_random_str_vec_map( 1, Constants::Limit::kVectorLength + 1 );
    const auto loadFactor = 1.0;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, loadFactor, std::move( memoryHandler ) );

    for ( const auto & p : bigStrMap )
    {
        std::vector<std::string_view> vec;
        for ( const auto & v : p.second )
        {
            vec.push_back( v );
        }
        CHECK_THROWS_AS( cache.put( p.first, vec ), std::runtime_error );
    }

    const auto strMap = axoncache::test_utils::gen_random_str_vec_map( 1000, Constants::Limit::kVectorLength );

    for ( const auto & p : strMap )
    {
        std::vector<std::string_view> vec;
        for ( const auto & v : p.second )
        {
            vec.push_back( v );
        }
        cache.put( p.first, vec );
    }

    for ( const auto & p : strMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        const auto vec = cache.getVector( std::string_view{ key } );
        CHECK( vec.size() == value.size() );
        for ( auto ix = 0U; ix < vec.size(); ++ix )
        {
            CHECK( vec[ix] == value[ix] );
        }
    }
}

TEST_CASE( "HashedCacheBaseTestGetEmpty" )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto loadFactor = 1.0;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, loadFactor, std::move( memoryHandler ) );

    CHECK( 0UL == cache.numberOfEntries() );
    CHECK( cache.get( std::string_view{ "helloworld" } ) == std::string_view{} );
}

TEST_CASE( "HashedCacheBaseTestKeyAndValueTooLarge" )
{
    const uint16_t offsetBits = 30U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto bigStrMap = axoncache::test_utils::gen_random_str_vec_map( 1, Constants::Limit::kVectorLength + 1 );
    const auto maxLoadFactor = 0.5;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    auto key = axoncache::test_utils::gen_random( Constants::Limit::kKeyLength );
    auto val = axoncache::test_utils::gen_random( 70 );
    cache.put( key, val );
    CHECK( cache.get( key ) == val );

    key = axoncache::test_utils::gen_random( Constants::Limit::kKeyLength + 1 );
    CHECK_THROWS_WITH( cache.put( key, val ), "key size 65536 too large. max=65535" );

    key = axoncache::test_utils::gen_random( 60 );
    val = axoncache::test_utils::gen_random( Constants::Limit::kValueLength - 1 ); // -1 for the null terminator
    cache.put( key, val );
    CHECK( cache.get( key ) == val );

    key = axoncache::test_utils::gen_random( 10 );
    val = axoncache::test_utils::gen_random( Constants::Limit::kValueLength );
    CHECK_THROWS_WITH( cache.put( key, val ), "value size 16777216 too large. max=16777215" );
}

TEST_CASE( "HashedCacheBaseTestPutDuplicateKeys" )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto loadFactor = 1.0;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, loadFactor, std::move( memoryHandler ) );

    auto ret = cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    CHECK( ret.first == true );
    CHECK( ret.second == 0 );
    ret = cache.put( std::string_view{ "hello" }, std::string_view{ "world2" } );
    CHECK( ret.first == true );
    CHECK( ret.second == 1 );

    // Should always return the first value inserted for a key
    CHECK( cache.get( std::string_view{ "hello" } ) == std::string_view{ "world" } );
}

TEST_CASE( "HashedCacheBaseTestGetDoesNotExist" )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto loadFactor = 1.0;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, loadFactor, std::move( memoryHandler ) );

    cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    cache.put( std::string_view{ "hello" }, std::string_view{ "world2" } );

    CHECK( cache.get( std::string_view{ "abc" } ) == std::string_view{ "" } );

    const auto vec = cache.getVector( std::string_view{ "abc" } );
    CHECK( vec.empty() );
}

TEST_CASE( "HashedCacheBaseTestNumberOfKeySlots" )
{
    const uint16_t offsetBits = 64U;
    const auto loadFactor = 1.0;
    BucketChainCache cacheZero( offsetBits, 0UL, loadFactor, std::make_unique<MallocMemoryHandler>( 0UL ) );
    CHECK( cacheZero.numberOfKeySlots() == 1UL );

    BucketChainCache cacheOne( offsetBits, 1UL, loadFactor, std::make_unique<MallocMemoryHandler>( 1UL ) );
    CHECK( cacheOne.numberOfKeySlots() == 1UL );

    BucketChainCache cacheFour( offsetBits, 4UL, loadFactor, std::make_unique<MallocMemoryHandler>( 4UL ) );
    CHECK( cacheFour.numberOfKeySlots() == 4UL );

    BucketChainCache cacheFive( offsetBits, 5UL, loadFactor, std::make_unique<MallocMemoryHandler>( 5UL ) );
    CHECK( cacheFive.numberOfKeySlots() == 8UL );

    BucketChainCache cache13( offsetBits, 13UL, loadFactor, std::make_unique<MallocMemoryHandler>( 13UL ) );
    CHECK( cache13.numberOfKeySlots() == 16UL );
}

TEST_CASE( "HashedCacheBaseTestType" )
{
    const uint16_t offsetBits = 64U;
    const auto loadFactor = 1.0;
    BucketChainCache cacheZero( offsetBits, 0UL, loadFactor, std::make_unique<MallocMemoryHandler>( 0UL ) );
    CHECK( cacheZero.type() == CacheType::BUCKET_CHAIN );
}

TEST_CASE( "HashedCacheBaseTestOutput" )
{
    const uint16_t offsetBits = 64U;
    const auto loadFactor = 1.0;
    BucketChainCache cache( offsetBits, 1UL, loadFactor, std::make_unique<MallocMemoryHandler>( 1UL ) );
    cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    std::stringstream ss;
    cache.output( ss );
    std::string expected = "hello=world";
    const auto result = ss.str();

    std::stringstream hexSs;
    for ( auto c : result )
    {
        hexSs << std::hex << static_cast<int>( c );
    }

    CHECK( hexSs.str() == "800000000000000050600068656c6c6f776f726c640" );
}
