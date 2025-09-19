// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/cache/value/ChainedValue.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include <stdlib.h>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include "CacheTestUtils.h"
#include "axoncache/cache/CacheType.h"
using namespace axoncache;

TEST_CASE( "ChainedValueTestSimple" )
{
    const auto numberOfKeysToTest = 1UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    value.add( 0UL, "hello", unusedHashcode, 1, "world", &memoryHandler );

    bool isExist = false;
    CHECK( value.get( memoryHandler.data(), 0UL, "hello", 1, &isExist ) == "world" );
}

TEST_CASE( "ChainedValueTestTypeMismatch" )
{
    const auto numberOfKeysToTest = 1UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    value.add( 0UL, "hello", unusedHashcode, 1, "world", &memoryHandler );

    bool isExist = false;
    CHECK( value.get( memoryHandler.data(), 0UL, "hello", 2, &isExist ) == "" );
}

TEST_CASE( "ChainedValueTestEmptyKey" )
{
    const auto numberOfKeysToTest = 1UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    value.add( 0UL, "", unusedHashcode, 1, "world", &memoryHandler );

    bool isExist = false;
    CHECK( value.get( memoryHandler.data(), 0UL, "", 1, &isExist ) == "world" );
}

TEST_CASE( "ChainedValueTestEmptyValue" )
{
    const auto numberOfKeysToTest = 1UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    value.add( 0UL, "hello", unusedHashcode, 1, "", &memoryHandler );

    bool isExist = false;
    CHECK( value.get( memoryHandler.data(), 0UL, "hello", 1, &isExist ) == "" );
}

TEST_CASE( "ChainedValueTestEmptyKeyValue" )
{
    const auto numberOfKeysToTest = 1UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    value.add( 0UL, "", unusedHashcode, 1, "", &memoryHandler );

    bool isExist = false;
    CHECK( value.get( memoryHandler.data(), 0UL, "", 1, &isExist ) == "" );
}

TEST_CASE( "ChainedValueTestMany" )
{
    const auto numberOfKeysToTest = 10000UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );
    const auto strMap = test_utils::gen_random_str_map( numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    auto keyOffset = 0UL;
    for ( const auto & str : strMap )
    {
        value.add( keyOffset, str.first, unusedHashcode, str.first.size(), str.second, &memoryHandler );
        keyOffset += sizeof( uint64_t );
    }

    keyOffset = 0UL;
    for ( const auto & str : strMap )
    {
        bool isExist = false;
        CHECK( value.get( memoryHandler.data(), keyOffset, str.first, str.first.size(), &isExist ) == str.second );
        keyOffset += sizeof( uint64_t );
    }

    const auto strMapNegative = test_utils::gen_random_str_map( 1000 );
    for ( const auto & str : strMapNegative )
    {
        if ( !strMap.contains( str.first ) )
        {
            const auto ranOffset = ( rand() % numberOfKeysToTest ) * sizeof( uint64_t );
            CHECK( !value.contains( memoryHandler.data(), ranOffset, str.first ) );
        }
    }
}

TEST_CASE( "ChainedValueTestCollisions" )
{
    const auto numberOfKeysToTest = 1000UL;
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );
    const auto strMap = test_utils::gen_random_str_map( numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    for ( const auto & str : strMap )
    {
        value.add( 0UL, str.first, unusedHashcode, str.first.size(), str.second, &memoryHandler );
    }

    for ( const auto & str : strMap )
    {
        bool isExist = false;
        CHECK( value.get( memoryHandler.data(), 0UL, str.first, str.first.size(), &isExist ) == str.second );
    }

    const auto strMapNegative = test_utils::gen_random_str_map( 100 );
    for ( const auto & str : strMapNegative )
    {
        if ( !strMap.contains( str.first ) )
        {
            bool isExist = false;
            CHECK( value.get( memoryHandler.data(), 0UL, str.first, str.first.size(), &isExist ) == "" );
        }
    }
}
