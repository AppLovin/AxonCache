// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <axoncache/cache/MapCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include <iosfwd>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include "CacheTestUtils.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

TEST_CASE( "MapCacheBaseTest" )
{
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );

    MapCache cache( std::move( memoryHandler ) );

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

TEST_CASE( "MapCacheBaseTestGetVector" )
{
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = axoncache::test_utils::gen_random_str_vec_map( numberOfKeysToTest );

    MapCache cache( std::move( memoryHandler ) );

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

TEST_CASE( "MapCacheBaseTestGetEmpty" )
{
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );

    MapCache cache( std::move( memoryHandler ) );

    CHECK( 0UL == cache.numberOfEntries() );
    CHECK( cache.get( std::string_view{ "helloworld" } ) == std::string_view{} );
}

TEST_CASE( "MapCacheBaseTestPutDuplicateKeys" )
{
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );

    MapCache cache( std::move( memoryHandler ) );

    cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    cache.put( std::string_view{ "hello" }, std::string_view{ "world2" } );

    // Should always return the first value inserted for a key
    CHECK( cache.get( std::string_view{ "hello" } ) == "world" );
}

TEST_CASE( "MapCacheBaseTestGetDoesNotExist" )
{
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );

    MapCache cache( std::move( memoryHandler ) );

    cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    cache.put( std::string_view{ "hello" }, std::string_view{ "world2" } );

    CHECK( cache.get( std::string_view{ "abc" } ) == std::string_view{ "" } );

    const auto vec = cache.getVector( std::string_view{ "abc" } );
    CHECK( vec.empty() );
}

TEST_CASE( "MapCacheBaseTestNumberOfKeySlots" )
{
    MapCache cacheZero( std::make_unique<MallocMemoryHandler>( 0UL ) );
    CHECK( cacheZero.numberOfKeySlots() == 0UL );
}

TEST_CASE( "MapCacheBaseTestType" )
{
    MapCache cacheZero( std::make_unique<MallocMemoryHandler>( 0UL ) );
    CHECK( cacheZero.type() == CacheType::MAP );
}

TEST_CASE( "MapCacheBaseTestOutput" )
{
    MapCache cache( std::make_unique<MallocMemoryHandler>( 2UL ) );
    cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    cache.put( std::string_view{ "key2" }, std::vector<std::string_view>{ "this", "isa", "vec" } );
    std::stringstream ss;
    cache.output( ss );
    CHECK( ss.str() == "hello=world\nkey2=this|isa|vec\n" );
}
