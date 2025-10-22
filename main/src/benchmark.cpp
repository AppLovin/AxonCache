// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "benchmark.h"

#include <axoncache/logger/Logger.h>
#include "axoncache/capi/CacheWriterCApi.h"
#include "axoncache/capi/CacheReaderCApi.h"

#include <axoncache/CacheGenerator.h>
#include <axoncache/loader/CacheOneTimeLoader.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/cache/BucketChainCache.h>
#include "axoncache/common/SharedSettingsProvider.h"

#include <string>
#include <chrono>
#include <iomanip>
#include <locale>
#include <sstream>
#include <random>

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

auto benchModeAxonCacheCApi(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void
{
    AL_LOG_INFO( "Using AxonCacheCApi" );
    using clock = std::chrono::steady_clock;

    const std::string dataPath = ".";
    const std::string settingsPath = dataPath + "/test.settings";

    std::ostringstream oss;
    oss << "ccache.destination_folder=" << dataPath << "\n";
    oss << "ccache.type=5" << dataPath << "\n";
    oss << "ccache.offset.bits=28" << dataPath << "\n";
    writeFile( settingsPath, oss.str() );

    {
        auto start = clock::now();

        auto * handle = NewCacheWriterHandle();
        int errorCode = CacheWriter_Initialize( handle,
                                                "bench_cli_test",
                                                settingsPath.c_str(),
                                                2 * numKeys );
        if ( errorCode != 0 )
        {
            throw std::runtime_error( "Error initializing writer" );
        }

        for ( int idx = 0; idx < numKeys; ++idx )
        {
            if ( CacheWriter_InsertKey( handle,
                                        keys[idx].data(), keys[idx].size(),
                                        vals[idx].data(), vals[idx].size(), 0 ) != 0 )
            {
                throw std::runtime_error( "Error inserting key" );
            }
        }

        // Write/Flush the cache
        CacheWriter_FinishCacheCreation( handle );
        CacheWriter_Finalize( handle );
        CacheWriter_DeleteCppObject( handle );

        std::string filePath = dataPath + "/bench_cli_test.cache";
        std::string newFilePath = dataPath + "/bench_cli_test.1690484217134.cache";
        ::rename( filePath.c_str(), newFilePath.c_str() );

        auto end = clock::now();
        double elapsed = std::chrono::duration<double>( end - start ).count();
        double qps = static_cast<double>( numKeys ) / elapsed;

        std::stringstream ssKeys;
        std::stringstream ssQps;
        ssKeys.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );
        ssQps.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );

        ssKeys << numKeys;
        ssQps << static_cast<int64_t>( qps );

        std::ostringstream oss;
        oss << "Inserted " << ssKeys.str()
            << " keys in " << std::fixed << std::setprecision( 3 ) << elapsed
            << "s (" << ssQps.str() << " keys/sec)\n";
        AL_LOG_INFO( oss.str() );
    }

    // Create a random generator seeded with a non-deterministic value
    std::random_device randomDevice;
    std::mt19937 gen( randomDevice() );

    // Shuffle in place
    std::shuffle( keys.begin(), keys.end(), gen );

    // Now create a reader and do some lookups
    {
        auto * handle = NewCacheReaderHandle();
        int errorCode = CacheReader_Initialize( handle,
                                                "bench_cli_test",
                                                dataPath.c_str(),
                                                "1690484217134",
                                                1 );
        if ( errorCode != 0 )
        {
            throw std::runtime_error( "Error initializing reader" );
        }

        auto start = clock::now();

        for ( int idx = 0; idx < numKeys; ++idx )
        {
            int size = 0;
            int isExist = 0;
            CacheReader_GetKey( handle,
                                keys[idx].data(), keys[idx].size(),
                                &isExist, &size );
            if ( isExist != 1 )
            {
                throw std::runtime_error( "Error looking up value" );
            }
        }

        auto end = clock::now();
        double elapsed = std::chrono::duration<double>( end - start ).count();
        double qps = static_cast<double>( numKeys ) / elapsed;

        std::stringstream ssKeys;
        std::stringstream ssQps;
        ssKeys.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );
        ssQps.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );

        ssKeys << numKeys;
        ssQps << static_cast<int64_t>( qps );

        std::stringstream oss;
        oss << "Looked up " << ssKeys.str()
            << " keys in " << std::fixed << std::setprecision( 3 ) << elapsed
            << "s (" << ssQps.str() << " keys/sec)\n";
        AL_LOG_INFO( oss.str() );
    }
}

auto benchModeAxonCacheCppApi(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void
{
    AL_LOG_INFO( "Using AxonCacheCppApi" );
    using clock = std::chrono::steady_clock;

    const std::string dataPath = ".";
    const std::string settingsPath = dataPath + "/test.settings";

    std::ostringstream oss;
    oss << "ccache.destination_folder=" << dataPath << "\n";
    oss << "ccache.type=5" << "\n";
    oss << "ccache.offset.bits=28" << "\n";
    writeFile( settingsPath, oss.str() );

    {
        auto start = clock::now();

        auto * handle = NewCacheWriterHandle();
        int errorCode = CacheWriter_Initialize( handle,
                                                "bench_cli_test",
                                                settingsPath.c_str(),
                                                2 * numKeys );
        if ( errorCode != 0 )
        {
            std::ostringstream oss;
            oss << "Error initializing writer: " << errorCode;
            throw std::runtime_error( oss.str().c_str() );
        }

        for ( int idx = 0; idx < numKeys; ++idx )
        {
            if ( CacheWriter_InsertKey( handle,
                                        keys[idx].data(), keys[idx].size(),
                                        vals[idx].data(), vals[idx].size(), 0 ) != 0 )
            {
                throw std::runtime_error( "Error inserting key" );
            }
        }

        // Write/Flush the cache
        CacheWriter_FinishCacheCreation( handle );
        CacheWriter_Finalize( handle );
        CacheWriter_DeleteCppObject( handle );

        std::string filePath = dataPath + "/bench_cli_test.cache";
        std::string newFilePath = dataPath + "/bench_cli_test.1690484217134.cache";
        ::rename( filePath.c_str(), newFilePath.c_str() );

        auto end = clock::now();
        double elapsed = std::chrono::duration<double>( end - start ).count();
        double qps = static_cast<double>( numKeys ) / elapsed;

        std::stringstream ssKeys;
        std::stringstream ssQps;
        ssKeys.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );
        ssQps.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );

        ssKeys << numKeys;
        ssQps << static_cast<int64_t>( qps );

        std::ostringstream oss;
        oss << "Inserted " << ssKeys.str()
            << " keys in " << std::fixed << std::setprecision( 3 ) << elapsed
            << "s (" << ssQps.str() << " keys/sec)\n";
        AL_LOG_INFO( oss.str() );
    }

    // Create a random generator seeded with a non-deterministic value
    std::random_device randomDevice;
    std::mt19937 gen( randomDevice() );

    // Shuffle in place
    std::shuffle( keys.begin(), keys.end(), gen );

    // Now create a reader and do some lookups
    {
        const axoncache::SharedSettingsProvider settings( "" );
        axoncache::CacheOneTimeLoader loader( &settings );

        auto start = clock::now();

        std::string taskName = "bench_cli_test";
        std::string destinationFolder{ dataPath }; // NOLINT
        std::string timestamp( "1690484217134" );

        // The decompressed file name is flipped, the extension comes last
        const size_t lastindex = taskName.find_last_of( '.' );
        auto cacheNameNoExt = taskName.substr( 0, lastindex );

        std::string cacheName;
        cacheName += cacheNameNoExt;
        cacheName += ".";
        cacheName += timestamp;
        cacheName += ".cache";

        std::string cacheAbsolutePath( destinationFolder );
        cacheAbsolutePath += "/";
        cacheAbsolutePath += cacheName;

        auto cache =loader.loadAbsolutePath<axoncache::LinearProbeDedupCache>( cacheName, cacheAbsolutePath, true );

        for ( int idx = 0; idx < numKeys; ++idx )
        {
            const auto & key = keys[idx];
            auto result = cache->getString( std::string_view{ key.data(), key.size() } );
            if ( !result.second )
            {
                throw std::runtime_error( "Error looking up value" );
            }
        }

        auto end = clock::now();
        double elapsed = std::chrono::duration<double>( end - start ).count();
        double qps = static_cast<double>( numKeys ) / elapsed;

        std::stringstream ssKeys;
        std::stringstream ssQps;
        ssKeys.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );
        ssQps.imbue( std::locale( std::locale::classic(), new comma_numpunct ) );

        ssKeys << numKeys;
        ssQps << static_cast<int64_t>( qps );

        std::stringstream oss;
        oss << "Looked up " << ssKeys.str()
            << " keys in " << std::fixed << std::setprecision( 3 ) << elapsed
            << "s (" << ssQps.str() << " keys/sec)\n";
        AL_LOG_INFO( oss.str() );
    }
}
