// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <memory>
#include <utility>
#include <filesystem>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <doctest/doctest.h>
#include <cstdint>
#include "axoncache/capi/CacheReaderCApi.h"
#include <axoncache/writer/CacheFileWriter.h>

#include "ResourceLoader.h"

using namespace axoncache;

//
// ninja -C build axoncacheTest && ./build/test/axoncacheTest --test-case=CacheReaderCApiBasicTest
//
TEST_CASE( "CacheReaderCApiBasicTest" ) // NOLINT
{
    //
    // Test file lives here "test/data/bench_cache.1758220992251.cache";
    //
    auto dataPath = axoncacheTest::resolveResourcePath( "../../../data", __FILE__ );

    auto * handle = NewCacheReaderHandle();
    int errorCode = CacheReader_Initialize( handle,
                                            "bench_cache", dataPath.c_str() /* "test/data" */, "1758220992251", 1 );

    CHECK( errorCode == 0 );

    int size = 0;
    int isExist = 0;
    char * key = ( char * )"key_666"; // NOLINT
    auto keySize = strlen( key );     // or this to be more exact ? auto strSize = env->GetStringUTFLength( key );
    auto * val = CacheReader_GetKey( handle, ( char * )key, keySize, &isExist, &size );

    CHECK( isExist == 1 );
    CHECK( std::string( val ) == "val_666" );
    free( val ); // NOLINT

    CacheReader_DeleteCppObject( handle );
}

TEST_CASE( "CacheReaderCApiTypedValueTest" ) // NOLINT
{
    const std::string dataPath = std::filesystem::temp_directory_path();
    const std::string cacheName = "test_cache";
    const std::string cacheTimestamp = "1";
    const auto numberOfKeysSlots = 1000UL;
    const auto maxLoadFactor = 0.5f;
    const uint16_t offsetBits = 35U;
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
    LinearProbeDedupCache cache( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ), CacheType::LINEAR_PROBE_DEDUP );
    {
        const std::string key = "value";
        cache.put( "1.a", key );
    }
    {
        int64_t key = 123LL;
        cache.put( "2.a", key );
    }
    {
        bool key = true;
        cache.put( "3.a", key );
    }
    {
        double key = 3.14f;
        cache.put( "4.a", key );
    }
    {
        std::vector<std::string_view> values{ "abc", "de", "f" };
        cache.put( "5.a", values );
    }
    {
        std::vector<float> values{ 1.f, 2.f, 3.f };
        cache.put( "6.a", values );
    }
    {
        const std::string key;
        cache.put( "7.a", key );
    }

    CacheFileWriter writer( dataPath, cacheName + "." + cacheTimestamp, &cache );
    writer.write();

    auto * handle = NewCacheReaderHandle();
    int errorCode = CacheReader_Initialize( handle, cacheName.c_str(), dataPath.c_str(), cacheTimestamp.c_str(), 1 );
    CHECK( errorCode == 0 );

    // string
    {
        std::string key = "1.a";
        int size = 0;
        int isExist = 0;
        char * val = CacheReader_GetKey( handle, key.data(), key.size(), &isExist, &size );
        CHECK( size == 5 );
        CHECK( std::string( val ) == "value" );
        CHECK( isExist == 1 );
        free( val ); // NOLINT

        size = 0;
        isExist = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( size == 6 );
        CHECK( std::string( keyType ) == "String" );
        free( keyType ); // NOLINT

        // Missing key
        key = "1.z";
        size = 0;
        isExist = 0;
        val = CacheReader_GetKey( handle, key.data(), key.size(), &isExist, &size );
        CHECK( size == 0 );
        CHECK( isExist == 0 );
        free( val ); // NOLINT

        // Key exist but no value
        key = "7.a";
        size = 0;
        isExist = 0;
        val = CacheReader_GetKey( handle, key.data(), key.size(), &isExist, &size );
        CHECK( size == 0 );
        CHECK( isExist == 1 );
        free( val ); // NOLINT
    }
    // int64
    {
        std::string key = "2.a";
        int isExist = 0;
        int64_t val = CacheReader_GetLong( handle, key.data(), key.size(), &isExist, 0 );
        CHECK( isExist == 1 );
        CHECK( val == 123LL );

        int size = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( std::string( keyType ) == "Int64" );
        CHECK( size == 5 );
        free( keyType ); // NOLINT

        key = "2.z";
        isExist = 1;
        val = CacheReader_GetLong( handle, key.data(), key.size(), &isExist, 987LL );
        CHECK( isExist == 0 );
        CHECK( val == 987LL );
    }
    // int32
    {
        std::string key = "2.a";
        int isExist = 0;
        int val = CacheReader_GetInteger( handle, key.data(), key.size(), &isExist, 0 );
        CHECK( isExist == 1 );
        CHECK( val == 123 );

        int size = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( std::string( keyType ) == "Int64" ); // Populate key was encoded as int64
        CHECK( size == 5 );
        free( keyType ); // NOLINT

        key = "2.z";
        isExist = 1;
        val = CacheReader_GetInteger( handle, key.data(), key.size(), &isExist, 987 );
        CHECK( isExist == 0 );
        CHECK( val == 987 );
    }
    // bool
    {
        std::string key = "3.a";
        int isExist = 0;
        int val = CacheReader_GetBool( handle, key.data(), key.size(), &isExist, 0 );
        CHECK( isExist == 1 );
        CHECK( val == 1 );

        int size = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( std::string( keyType ) == "Bool" ); // Populate key was encoded as int64
        CHECK( size == 4 );
        free( keyType ); // NOLINT

        key = "3.z";
        isExist = 1;
        val = CacheReader_GetBool( handle, key.data(), key.size(), &isExist, 0 );
        CHECK( isExist == 0 );
        CHECK( val == 0 );
    }
    // double
    {
        std::string key = "4.a";
        int isExist = 0;
        double val = CacheReader_GetDouble( handle, key.data(), key.size(), &isExist, 0. );
        CHECK( isExist == 1 );
        CHECK( val == 3.14f );

        int size = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( std::string( keyType ) == "Double" ); // Populate key was encoded as int64
        CHECK( size == 6 );
        free( keyType ); // NOLINT

        key = "4.z";
        isExist = 1;
        val = CacheReader_GetDouble( handle, key.data(), key.size(), &isExist, 2.47f );
        CHECK( isExist == 0 );
        CHECK( val == 2.47f );
    }
    // GetVector
    {
        std::string key = "5.a";
        int vectorSize = 0;
        int * valueSizes = nullptr;
        char ** values = CacheReader_GetVector( handle, key.data(), key.size(), &vectorSize, &valueSizes );
        std::vector<std::string_view> expectedValues{ "abc", "de", "f" };
        CHECK( valueSizes != nullptr );
        CHECK( vectorSize == expectedValues.size() );
        for ( size_t i = 0; i < expectedValues.size(); i++ )
        {
            CHECK( std::string{ values[i], static_cast<size_t>( valueSizes[i] ) } == expectedValues[i] );
            CHECK( valueSizes[i] == expectedValues[i].size() );
            free( values[i] );
        }
        free( values );
        free( valueSizes );

        int size = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( std::string( keyType ) == "StringList" ); // was encoded as a string list
        CHECK( size == 10 );
        free( keyType ); // NOLINT

        key = "5.z";
        vectorSize = 1;
        values = CacheReader_GetVector( handle, key.data(), key.size(), &vectorSize, &valueSizes );
        CHECK( values == nullptr );
        CHECK( vectorSize == 0 );
        CHECK( valueSizes == nullptr );
    }
    // GetVectorKey
    {
        std::string key = "5.a";
        int valueSize;
        char * value;
        std::vector<std::string> expectedValues{ "abc", "de", "f" };
        for ( size_t i = 0; i < expectedValues.size(); i++ )
        {
            valueSize = 0;
            value = CacheReader_GetVectorKey( handle, key.data(), key.size(), i, &valueSize );
            CHECK( value != nullptr );
            CHECK( valueSize == expectedValues[i].size() );
            CHECK( std::string{ value, static_cast<size_t>( valueSize ) } == expectedValues[i] );
            free( value );
        }
        valueSize = 0;
        value = CacheReader_GetVectorKey( handle, key.data(), key.size(), expectedValues.size() + 1, &valueSize );
        CHECK( value == nullptr );
        CHECK( valueSize == 0 );

        key = "5.z";
        valueSize = 1;
        value = CacheReader_GetVectorKey( handle, key.data(), key.size(), 0, &valueSize );
        CHECK( value == nullptr );
        CHECK( valueSize == 0 );
    }
    // GetFloatVector
    {
        std::string key = "6.a";
        int vectorSize = 0;
        std::vector<float> expectedValues{ 1.f, 2.f, 3.f };
        float * values = CacheReader_GetFloatVector( handle, key.data(), key.size(), &vectorSize );
        CHECK( values != nullptr );
        CHECK( vectorSize == expectedValues.size() );
        for ( size_t i = 0; i < expectedValues.size(); i++ )
        {
            CHECK( values[i] == expectedValues[i] );
        }
        free( values );

        int size = 0;
        char * keyType = CacheReader_GetKeyType( handle, key.data(), key.size(), &size );
        CHECK( std::string( keyType ) == "FloatList" ); // was encoded as a string list
        CHECK( size == 9 );
        free( keyType ); // NOLINT

        key = "6.z";
        vectorSize = 1;
        values = CacheReader_GetFloatVector( handle, key.data(), key.size(), &vectorSize );
        CHECK( values == nullptr );
        CHECK( vectorSize == 0 );
    }
    CacheReader_DeleteCppObject( handle );
    std::filesystem::remove( dataPath + "/" + cacheName + "." + cacheTimestamp + ".cache" );
}
