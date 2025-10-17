// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <axoncache/Constants.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include <cstdint>
#include <map>
#include <ostream>
#include <set>
#include <stdexcept>
#include <vector>
#include "CacheTestUtils.h"
#include "axoncache/common/StringUtils.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheValue.h"
#include "axoncache/transformer/TypeToString.h"

using namespace axoncache;

TEST_CASE( "LinearProbeCacheBaseLoadFactorTest" )
{
    const uint16_t offsetBits = 35U;
    const auto maxLoadFactor = Constants::ConfDefault::kLinearProbeMaxLoadFactor + 0.01;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( 100 * sizeof( uint64_t ) );
    CHECK_THROWS_AS( LinearProbeCache cache( offsetBits, 100, maxLoadFactor, std::move( memoryHandler ) ), std::runtime_error );
}

TEST_CASE( "LinearProbeCacheBaseKeySpaceisFull" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = 30; offsetBits <= 32; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
        LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
        const auto numberOfKeysToTest = cache.maxNumberEntries();
        const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );

        for ( const auto & [key, value] : strMap )
        {
            cache.put( key, value );
        }

        CHECK( strMap.size() == cache.numberOfEntries() );
        for ( const auto & p : strMap )
        {
            const auto key = p.first;
            const auto value = p.second;
            CHECK( cache.contains( std::string_view{ key } ) );
            CHECK( cache.get( std::string_view{ key } ) == std::string_view{ value } );
        }

        CHECK_THROWS_WITH( cache.put( "expect", "exception" ), "keySpace is full" ); // cache is full. can't add any more key
    }
}

TEST_CASE( "LinearProbeCacheBaseTest" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = Constants::kMinLinearProbeOffsetBits; offsetBits <= Constants::kMaxLinearProbeOffsetBits; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
        LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
        const auto numberOfKeysToTest = cache.maxNumberEntries();
        const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );

        for ( const auto & [key, value] : strMap )
        {
            cache.put( key, value );
        }

        CHECK( strMap.size() == cache.numberOfEntries() );
        for ( const auto & p : strMap )
        {
            const auto key = p.first;
            const auto value = p.second;
            CHECK( cache.contains( std::string_view{ key } ) );
            CHECK( cache.get( std::string_view{ key } ) == std::string_view{ value } );
        }
    }
}

TEST_CASE( "LinearProbeCacheBaseTestGetVectorKeyspaceFull" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = 20; offsetBits <= 22; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
        LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
        const auto numberOfKeysToTest = cache.maxNumberEntries();
        const auto strMap = axoncache::test_utils::gen_random_str_vec_map( numberOfKeysToTest );

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

        std::vector<std::string_view> vec;
        CHECK_THROWS_WITH( cache.put( "expect", vec ), "keySpace is full" ); // cache is full. can't add any more key
    }
}

TEST_CASE( "LinearProbeCacheBaseTestGetVector" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = Constants::kMinLinearProbeOffsetBits; offsetBits <= Constants::kMaxLinearProbeOffsetBits; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
        LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
        const auto numberOfKeysToTest = cache.maxNumberEntries();
        const auto strMap = axoncache::test_utils::gen_random_str_vec_map( numberOfKeysToTest );

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
}

TEST_CASE( "LinearProbeTestOffsetBitsTooShort" )
{
    const uint16_t offsetBits = 16U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto bigStrMap = axoncache::test_utils::gen_random_str_vec_map( 1, Constants::Limit::kVectorLength + 1 );
    const auto maxLoadFactor = 0.5;

    LinearProbeCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    auto key = axoncache::test_utils::gen_random( Constants::Limit::kKeyLength );
    auto val = axoncache::test_utils::gen_random( Constants::Limit::kValueLength - 1 ); // -1 for the null character
    cache.put( key, val );
    CHECK( cache.get( key ) == val );

    key = axoncache::test_utils::gen_random( Constants::Limit::kKeyLength );
    val = axoncache::test_utils::gen_random( Constants::Limit::kValueLength - 1 ); // -1 for the null character
    CHECK_THROWS_WITH( cache.put( key, val ), "offset bits 16 too short" );
}

TEST_CASE( "LinearProbeTestKeyAndValueTooLarge" )
{
    const uint16_t offsetBits = 30U;
    const auto numberOfKeysToTest = 1000UL;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto bigStrMap = axoncache::test_utils::gen_random_str_vec_map( 1, Constants::Limit::kVectorLength + 1 );
    const auto maxLoadFactor = 0.5;

    LinearProbeCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    auto key = axoncache::test_utils::gen_random( Constants::Limit::kKeyLength );
    auto val = axoncache::test_utils::gen_random( 100 );
    cache.put( key, val );
    CHECK( cache.get( key ) == val );

    key = axoncache::test_utils::gen_random( Constants::Limit::kKeyLength + 1 );
    CHECK_THROWS_WITH( cache.put( key, val ), "key size 65536 too large. max=65535" );

    key = axoncache::test_utils::gen_random( 60 );
    val = axoncache::test_utils::gen_random( Constants::Limit::kValueLength - 1 ); // -1 for the null character
    cache.put( key, val );
    CHECK( cache.get( key ) == val );

    key = axoncache::test_utils::gen_random( 10 );
    val = axoncache::test_utils::gen_random( Constants::Limit::kValueLength );
    CHECK_THROWS_WITH( cache.put( key, val ), "value size 16777216 too large. max=16777215" );
}

static auto GetLongVectorTester( uint16_t offsetBits, uint64_t numberOfKeysToTest )
{
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto bigStrMap = axoncache::test_utils::gen_random_str_vec_map( 1, Constants::Limit::kVectorLength + 1 );
    const auto maxLoadFactor = 0.5;

    LinearProbeCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    for ( const auto & p : bigStrMap )
    {
        std::vector<std::string_view> vec;
        for ( const auto & v : p.second )
        {
            vec.push_back( v );
        }
        CHECK_THROWS_AS( cache.put( p.first, vec ), std::runtime_error );
    }

    const auto strMap = axoncache::test_utils::gen_random_str_vec_map( 500, Constants::Limit::kVectorLength );

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

TEST_CASE( "LinearProbeTestGetLongVector28Bits" )
{
    const uint16_t offsetBits = 28U;
    const auto numberOfKeysToTest = 10000UL;
    GetLongVectorTester( offsetBits, numberOfKeysToTest );
}

TEST_CASE( "LinearProbeTestGetLongVector35Bits" )
{
    const uint16_t offsetBits = 35U;
    const auto numberOfKeysToTest = 10000UL;
    GetLongVectorTester( offsetBits, numberOfKeysToTest );
}

TEST_CASE( "LinearProbeCacheBaseTestGetEmpty" )
{
    const auto numberOfKeysToTest = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = Constants::kMinLinearProbeOffsetBits; offsetBits <= Constants::kMaxLinearProbeOffsetBits; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );

        LinearProbeCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

        CHECK( 0UL == cache.numberOfEntries() );
        CHECK( cache.get( std::string_view{ "helloworld" } ) == std::string_view{} );
    }
}

TEST_CASE( "LinearProbeCacheBaseTestPutDuplicateKeys" )
{
    const auto numberOfKeysToTest = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = Constants::kMinLinearProbeOffsetBits; offsetBits <= Constants::kMaxLinearProbeOffsetBits; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );

        LinearProbeCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

        auto putStatus = cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
        CHECK( putStatus.first == true );
        CHECK( putStatus.second == 0 );
        putStatus = cache.put( std::string_view{ "hello" }, std::string_view{ "world2" } );
        CHECK( putStatus.first == false );
        CHECK( putStatus.second == 1 );

        // Should always return the first value inserted for a key
        CHECK( cache.get( std::string_view{ "hello" } ) == std::string_view{ "world" } );
        CHECK( cache.numberOfEntries() == 1 ); // we don't insert duplicate keys
    }
}

TEST_CASE( "LinearProbeCacheBaseTestGetDoesNotExist" )
{
    const auto numberOfKeysToTest = 1000UL;
    const auto maxLoadFactor = 0.5f;

    for ( auto offsetBits = Constants::kMinLinearProbeOffsetBits; offsetBits <= Constants::kMaxLinearProbeOffsetBits; offsetBits++ )
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );

        LinearProbeCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

        cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
        cache.put( std::string_view{ "hello" }, std::string_view{ "world2" } );

        CHECK( cache.get( std::string_view{ "abc" } ) == std::string_view{} );

        const auto vec = cache.getVector( std::string_view{ "abc" } );
        CHECK( vec.empty() );
    }
}

TEST_CASE( "LinearProbeCacheBaseTestType" )
{
    const uint16_t offsetBits = 35U;
    const auto maxLoadFactor = 0.5f;
    LinearProbeCache cacheZero( offsetBits, 0UL, maxLoadFactor, std::make_unique<MallocMemoryHandler>( 0UL ) );
    CHECK( cacheZero.type() == CacheType::LINEAR_PROBE );
}

TEST_CASE( "LinearProbeCacheBaseTestOutput" )
{
    const uint16_t offsetBits = 35U;
    const auto maxLoadFactor = 0.5f;
    LinearProbeCache cache( offsetBits, 2UL, maxLoadFactor, std::make_unique<MallocMemoryHandler>( 2UL ) );
    cache.put( std::string_view{ "hello" }, std::string_view{ "world" } );
    std::stringstream ss;
    cache.output( ss );
    std::string expected = "hello=world";
    const auto result = ss.str();

    // hello: 68 65 6c 6c 6f
    // world: 77 6f 72 6c 64
    std::stringstream hexSs;
    for ( auto c : result )
    {
        hexSs << std::hex << static_cast<int>( c );
    }

    CHECK( hexSs.str() == "00000000800050ffffffe855ffffff9550060068656c6c6f776f726c640" );
}

TEST_CASE( "LinearProbeCacheAllTypeTest" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;
    const uint16_t offsetBits = 35U;

    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
    LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
    const auto numberOfKeysToTest = cache.maxNumberEntries() / 4;
    const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );
    std::set<std::string> keys;
    for ( const auto & [key, value] : strMap )
    {
        cache.put( key, value );
        keys.insert( key );
    }

    auto boolMap = axoncache::test_utils::gen_random_bool_map( numberOfKeysToTest, keys );
    for ( auto & [key, value] : boolMap )
    {
        cache.put( key, value );
    }

    auto doubleMap = axoncache::test_utils::gen_random_double_map( numberOfKeysToTest, keys );
    for ( auto & [key, value] : doubleMap )
    {
        cache.put( key, value );
    }

    auto int64Map = axoncache::test_utils::gen_random_int64_map( numberOfKeysToTest, keys );
    for ( auto & [key, value] : int64Map )
    {
        cache.put( key, value );
    }

    for ( const auto & p : strMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.get( std::string_view{ key } ) == std::string_view{ value } );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::String );
        CHECK( retVal.first == value );
    }

    for ( const auto & p : boolMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.getBool( std::string_view{ key } ).first == value );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::Bool );
        CHECK( transform<bool>( retVal.first ) == value );
    }

    for ( const auto & p : doubleMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.getDouble( std::string_view{ key } ).first == value );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::Double );
        CHECK( transform<double>( retVal.first ) == value );
    }

    for ( const auto & p : int64Map )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.getInt64( std::string_view{ key } ).first == value );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::Int64 );
        CHECK( transform<int64_t>( retVal.first ) == value );
    }
}

TEST_CASE( "LinearProbeCacheAllTypeToStringTest" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;
    const uint16_t offsetBits = 35U;

    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
    LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
    const auto numberOfKeysToTest = cache.maxNumberEntries() / 4;
    const auto strMap = axoncache::test_utils::gen_random_str_map( numberOfKeysToTest );
    std::set<std::string> keys;
    for ( const auto & [key, value] : strMap )
    {
        cache.put( key, value );
        keys.insert( key );
    }

    auto boolMap = axoncache::test_utils::gen_random_bool_map( numberOfKeysToTest, keys );
    for ( auto & [key, value] : boolMap )
    {
        cache.put( key, value ? "1" : "0" );
    }

    auto doubleMap = axoncache::test_utils::gen_random_double_map( numberOfKeysToTest, keys );
    for ( auto & [key, value] : doubleMap )
    {
        cache.put( key, std::to_string( value ) );
    }

    auto int64Map = axoncache::test_utils::gen_random_int64_map( numberOfKeysToTest, keys );
    for ( auto & [key, value] : int64Map )
    {
        cache.put( key, std::to_string( value ) );
    }

    for ( const auto & p : strMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.get( std::string_view{ key } ) == std::string_view{ value } );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::String );
        CHECK( retVal.first == value );
    }

    for ( const auto & p : boolMap )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.getBool( std::string_view{ key } ).first == value );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::String );
    }

    for ( const auto & p : doubleMap )
    {
        const auto key = p.first;
        const auto value = StringUtils::toDouble( std::to_string( p.second ) );
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.getDouble( std::string_view{ key } ).first == value );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::String );
    }

    for ( const auto & p : int64Map )
    {
        const auto key = p.first;
        const auto value = p.second;
        CHECK( cache.contains( std::string_view{ key } ) );
        CHECK( cache.getInt64( std::string_view{ key } ).first == value );
        auto retVal = cache.getWithType( std::string_view{ key } );
        CHECK( retVal.second == CacheValueType::String );
    }
}

TEST_CASE( "LinearProbeCacheDefaultValueReturnTest" )
{
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;
    const uint16_t offsetBits = 35U;

    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
    LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
    auto key = std::string_view( "key" );
    cache.put( key, std::string_view{} );
    {
        auto defaultValue = std::string_view( "value" );
        CHECK( cache.get( key, defaultValue ) == defaultValue );
    }
    {
        auto defaultValue = bool{ true };
        CHECK( cache.getBool( key, defaultValue ).first == defaultValue );
    }
    {
        auto defaultValue = int64_t{ 123456789 };
        CHECK( cache.getInt64( key, defaultValue ).first == defaultValue );
    }
    {
        auto defaultValue = double{ 3.1415 };
        CHECK( cache.getDouble( key, defaultValue ).first == defaultValue );
    }
    {
        auto keyString = std::string_view( "keyString" );
        cache.put( keyString, std::vector<std::string_view>{} );
        auto defaultValue = std::vector<std::string_view>{ "a|b|c" };
        auto retValue = cache.getVector( keyString, defaultValue );
        CHECK( retValue.size() == defaultValue.size() );
        for ( auto index = 0U; index < retValue.size(); ++index )
        {
            CHECK( retValue[index] == defaultValue[index] );
        }
    }
}

TEST_CASE( "LinearProbeCachePutAPItest" )
{
    const auto numberOfKeysSlots = 100UL;
    const auto maxLoadFactor = 0.5f;
    const uint16_t offsetBits = 35U;

    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
    LinearProbeCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ) );
    std::string_view value1{ "string_view" };
    cache.put( std::string_view{ "key1" }, std::string_view{ "string_view" } );
    bool value2{ true };
    cache.put( std::string_view{ "key2.1" }, value2 );
    value2 = false;
    cache.put( std::string_view{ "key2.2" }, value2 );
    int64_t value3{ 1 };
    cache.put( std::string_view{ "key3.1" }, value3 );
    value3 = 0;
    cache.put( std::string_view{ "key3.2" }, value3 );
    double value4{ 3.14 };
    cache.put( std::string_view{ "key4" }, value4 );
    std::vector<float> value5( { 1.0, 2.0, 2.5 } );
    cache.put( std::string_view{ "key5.1" }, value5 );
    cache.put( std::string_view{ "key5.2" }, std::string_view{ "1.0:2.0:2.5" } );
    std::vector<std::string_view> value6{ "value6" };
    cache.put( std::string_view{ "key6.1" }, value6 );

    std::vector<std::string_view> value6_2{ "value6A", "value6B" };
    cache.put( std::string_view{ "key6.2" }, value6_2 );

    CHECK( cache.get( std::string_view{ "key1" } ) == value1 );
    CHECK( cache.getBool( std::string_view{ "key2.1" } ).first == true );
    CHECK( cache.getBool( std::string_view{ "key2.2" } ).first == false );
    CHECK( cache.getInt64( std::string_view{ "key3.1" } ).first == 1LL );
    CHECK( cache.getInt64( std::string_view{ "key3.2" } ).first == 0LL );
    CHECK( cache.getBool( std::string_view{ "key3.1" } ).first == true );
    CHECK( cache.getBool( std::string_view{ "key3.2" } ).first == false );
    CHECK( cache.getDouble( std::string_view{ "key4" } ).first == 3.14 );

    CHECK( cache.getFloatVector( std::string_view{ "key5.1" } ) == value5 );
    CHECK( cache.getFloatVector( std::string_view{ "key5.2" } ) == value5 );
    CHECK( cache.getFloatVector( std::string_view{ "key5.3" } ).empty() );

    std::vector<int32_t> indices( { 1, 0, 3 } );
    std::vector<float> expectedValues( { 2.0, 1.0, 0.0 } );
    CHECK( cache.getFloatAtIndices( std::string_view{ "key5.1" }, indices ) == expectedValues );
    CHECK( cache.getFloatAtIndices( std::string_view{ "key5.2" }, indices ) == expectedValues );

    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.1" }, -1 ) == 0.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.1" }, 0 ) == 1.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.1" }, 1 ) == 2.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.1" }, 2 ) == 2.5f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.1" }, 3 ) == 0.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.2" }, -1 ) == 0.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.2" }, 0 ) == 1.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.2" }, 1 ) == 2.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.2" }, 2 ) == 2.5f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.2" }, 3 ) == 0.0f );
    CHECK( cache.getFloatAtIndex( std::string_view{ "key5.3" }, 1 ) == 0.0f );

    CHECK( cache.getVector( std::string_view{ "key6.1" } ) == value6 );
    CHECK( cache.getVector( std::string_view{ "key6.2" } ) == value6_2 );

    CHECK( cache.readKey( std::string_view{ "key1" } ) == value1 );
    CHECK( cache.readKey( std::string_view{ "key6.1" } ) == value6[0] );
    CHECK( cache.readKeys( std::string_view{ "key1" } ) == std::vector<std::string_view>{ 1, value1 } );
    CHECK( cache.readKeys( std::string_view{ "key6.1" } ) == value6 );
    CHECK( cache.readKeys( std::string_view{ "key6.2" } ) == value6_2 );
    CHECK( cache.readKey( std::string_view{ "key6.2" } ) == std::string_view{} );
    CHECK( cache.readKey( std::string_view{ "key3.1" } ) == std::string_view{} );
    CHECK( cache.readKeys( std::string_view{ "key3.1" } ) == std::vector<std::string_view>{} );
}
