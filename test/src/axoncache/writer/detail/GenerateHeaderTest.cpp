// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <chrono>
#include <string_view>
#include <memory>
#include <utility>
#include <axoncache/Constants.h>
#include <axoncache/cache/MapCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <axoncache/version.h>
#include <axoncache/writer/detail/GenerateHeader.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheHeader.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

TEST_CASE( "GenerateHeaderTest" )
{
    MapCache cache( std::make_unique<MallocMemoryHandler>() );
    cache.put( "hello", "world" );
    cache.put( "this", "that" );
    cache.put( "a", "b" );

    GenerateHeader generator;
    std::stringstream output{};

    std::string_view cacheName = "test_cache";
    generator.write( &cache, cacheName, output );
    const auto result = output.str();

    std::stringstream input( result );
    const auto val = generator.read( input );
    const auto name = val.first;
    const auto header = val.second;

    CHECK( header.magicNumber == Constants::kCacheHeaderMagicNumber );
    CHECK( header.headerSize == sizeof( CacheHeader ) );
    CHECK( header.version == ( AXONCACHE_VERSION_MAJOR * 1000 ) + ( AXONCACHE_VERSION_MINOR * 10 ) + AXONCACHE_VERSION_PATCH );
    CHECK( header.cacheType == static_cast<uint16_t>( CacheType::MAP ) );
    CHECK( ( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() - header.creationTimeMs ) < 1000UL ); // Allow for 1 second of difference
    CHECK( header.numberOfKeySlots == 3 );
    CHECK( header.numberOfEntries == 3 );
    CHECK( name == cacheName );
}

TEST_CASE( "GenerateHeaderLongNameTest" )
{
    MapCache cache( std::make_unique<MallocMemoryHandler>() );
    cache.put( "hello", "world" );
    cache.put( "this", "that" );
    cache.put( "a", "b" );

    GenerateHeader generator;
    std::stringstream output{};

    std::string_view cacheName = "test_cache_long_name_that_exceeds_32_characters_and_should_be_truncated";
    generator.write( &cache, cacheName, output );
    const auto result = output.str();

    std::stringstream input( result );
    const auto val = generator.read( input );
    const auto name = val.first;
    const auto header = val.second;

    CHECK( header.magicNumber == Constants::kCacheHeaderMagicNumber );
    CHECK( header.headerSize == sizeof( CacheHeader ) );
    CHECK( header.version == ( AXONCACHE_VERSION_MAJOR * 1000 ) + ( AXONCACHE_VERSION_MINOR * 10 ) + AXONCACHE_VERSION_PATCH );
    CHECK( header.cacheType == static_cast<uint16_t>( CacheType::MAP ) );
    CHECK( ( std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count() - header.creationTimeMs ) < 1000UL ); // Allow for 1 second of difference
    CHECK( header.numberOfKeySlots == 3 );
    CHECK( header.numberOfEntries == 3 );
    CHECK( name == cacheName.substr( 0, Constants::kMaxCacheNameSize - 1 ) );
}

TEST_CASE( "GenerateHeaderTestReadShort" )
{
    MapCache cache( std::make_unique<MallocMemoryHandler>() );
    cache.put( "hello", "world" );
    cache.put( "this", "that" );
    cache.put( "a", "b" );

    GenerateHeader generator;
    std::stringstream input( "hello world" );
    CHECK_THROWS_WITH( generator.read( input ), "malformed cache" );
}

TEST_CASE( "GenerateHeaderTestReadMalformed" )
{
    MapCache cache( std::make_unique<MallocMemoryHandler>() );
    cache.put( "hello", "world" );
    cache.put( "this", "that" );
    cache.put( "a", "b" );

    GenerateHeader generator;
    std::stringstream input( "hello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello worldhello world" );
    CHECK_THROWS_AS( generator.read( input ), std::runtime_error );
}
