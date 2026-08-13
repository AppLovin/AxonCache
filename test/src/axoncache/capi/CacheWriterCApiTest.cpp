// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <filesystem>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <array>
#include "doctest/doctest.h"
#include "axoncache/capi/CacheWriterCApi.h"
#include "axoncache/capi/CacheReaderCApi.h"
#include "axoncache/domain/CacheValue.h"

// NOLINTNEXTLINE(cppcoreguidelines-avoid-do-while)
// NOLINTBEGIN(cppcoreguidelines-avoid-do-while)

namespace
{

void writeFile( const std::string & filename, const std::string & content )
{
    std::ofstream out( filename, std::ios::out | std::ios::trunc );
    if ( !out )
    {
        throw std::runtime_error( "Failed to open file for writing: " + filename );
    }
    out << content;
    if ( !out )
    {
        throw std::runtime_error( "Failed to write to file: " + filename );
    }
}

}

//
// ninja -C build axoncacheTest && ./build/bin/axoncacheTest ---test-case=CacheWriterCApiBasicTest
//
TEST_CASE( "CacheWriterCApiBasicTest" ) // NOLINT
{
    const std::string dataPath = std::filesystem::temp_directory_path();
    const std::string settingsPath = dataPath + "/test.settings";

    std::ostringstream oss;
    oss << "ccache.destination_folder=" << dataPath << "\n";
    oss << "ccache.type=5" << dataPath << "\n";
    oss << "ccache.offset.bits=28" << dataPath << "\n";
    writeFile( settingsPath, oss.str() );

    {
        auto * handle = NewCacheWriterHandle();
        int errorCode = CacheWriter_Initialize( handle,
                                                "test_cache",
                                                settingsPath.c_str(),
                                                100 );
        CHECK( errorCode == 0 ); // success

        // Insert a few keys
        std::string key = "1.key";
        std::string val = "1.val";
        CacheWriter_AddDuplicateValue( handle, val.data(), 0 );
        CacheWriter_FinishAddDuplicateValues( handle );

        CHECK( CacheWriter_InsertKey( handle, key.data(), key.size(), val.data(), val.size(), 0 ) == 0 );

        key = "2.key";
        val = "2.val";

        CHECK( CacheWriter_InsertKey( handle, key.data(), key.size(), val.data(), val.size(), 0 ) == 0 );

        // Write/Flush the cache
        CHECK( CacheWriter_FinishCacheCreation( handle ) == 0 );
        CacheWriter_Finalize( handle );
        CacheWriter_DeleteCppObject( handle );

        std::string filePath = dataPath + "/test_cache.cache";
        std::string newFilePath = dataPath + "/test_cache.1690484217134.cache";
        CHECK( ::rename( filePath.c_str(), newFilePath.c_str() ) == 0 );
    }

    // Now create a reader and do some lookups
    {
        auto * handle = NewCacheReaderHandle();
        int errorCode = CacheReader_Initialize( handle,
                                                "test_cache",
                                                dataPath.c_str(),
                                                "1690484217134",
                                                true );
        CHECK( errorCode == 0 );

        {
            int size = 0;
            std::string key = "1.key";
            int isExist = 0;
            auto * val = CacheReader_GetKey( handle, key.data(), key.size(), &isExist, &size );

            CHECK( isExist == 1 );
            CHECK( size == 5 );
            CHECK( std::string( val ) == "1.val" );
        }

        {
            int size = 0;
            std::string key = "2.key";
            int isExist = 0;
            auto * val = CacheReader_GetKey( handle, key.data(), key.size(), &isExist, &size );

            CHECK( isExist == 1 );
            CHECK( size == 5 );
            CHECK( std::string( val ) == "2.val" );
        }

        {
            int size = 0;
            std::string key = "123456789.key";
            int isExist = 0;
            auto * val = CacheReader_GetKey( handle, key.data(), key.size(), &isExist, &size );

            CHECK( isExist == 0 );
            CHECK( size == 0 );
            CHECK( val == nullptr );
        }

        CacheReader_DeleteCppObject( handle );
    }
}

TEST_CASE( "CacheWriterCApiOffsetBitsTooSmallTest" ) // NOLINT
{
    const std::string dataPath = std::filesystem::temp_directory_path();
    const std::string settingsPath = dataPath + "/test.settings";

    std::ostringstream oss;
    oss << "ccache.destination_folder=" << dataPath << "\n";
    oss << "ccache.type=5" << dataPath << "\n";
    oss << "ccache.offset.bits=15" << dataPath << "\n";
    writeFile( settingsPath, oss.str() );

    {
        auto * handle = NewCacheWriterHandle();
        int errorCode = CacheWriter_Initialize( handle,
                                                "test_cache",
                                                settingsPath.c_str(),
                                                100 );
        CHECK( errorCode == 3 );
    }
}

TEST_CASE( "CacheWriterCApiDoubleHonorsValueSize" ) // NOLINT
{
    const std::string dataPath = std::filesystem::temp_directory_path();
    const std::string settingsPath = dataPath + "/axoncache_double_buffer.settings";
    const std::string cacheName = "axoncache_double_buffer";
    const std::string timestamp = "1690484217135";
    const std::string outputPath = dataPath + "/" + cacheName + ".cache";
    const std::string timestampedPath = dataPath + "/" + cacheName + "." + timestamp + ".cache";

    std::filesystem::remove( outputPath );
    std::filesystem::remove( timestampedPath );

    std::ostringstream settings;
    settings << "ccache.destination_folder=" << dataPath << "\n";
    settings << "ccache.type=5\n";
    settings << "ccache.offset.bits=28\n";
    writeFile( settingsPath, settings.str() );

    auto * writer = NewCacheWriterHandle();
    REQUIRE( CacheWriter_Initialize( writer, cacheName.c_str(), settingsPath.c_str(), 100 ) == 0 );

    std::string key = "double.key";
    std::array<char, 5> valueBuffer{ '1', '2', '.', '5', '9' };
    CHECK( CacheWriter_InsertKey( writer,
                                  key.data(),
                                  key.size(),
                                  valueBuffer.data(),
                                  4,
                                  static_cast<int8_t>( axoncache::CacheValueType::Double ) ) == 0 );
    CHECK( CacheWriter_FinishCacheCreation( writer ) == 0 );
    CacheWriter_Finalize( writer );
    CacheWriter_DeleteCppObject( writer );
    REQUIRE( ::rename( outputPath.c_str(), timestampedPath.c_str() ) == 0 );

    auto * reader = NewCacheReaderHandle();
    REQUIRE( CacheReader_Initialize( reader, cacheName.c_str(), dataPath.c_str(), timestamp.c_str(), true ) == 0 );
    int exists = 0;
    CHECK( CacheReader_GetDouble( reader, key.data(), key.size(), &exists, 0.0 ) == 12.5 );
    CHECK( exists == 1 );
    CacheReader_DeleteCppObject( reader );

    std::filesystem::remove( settingsPath );
    std::filesystem::remove( timestampedPath );
}

// NOLINTEND(cppcoreguidelines-avoid-do-while)
