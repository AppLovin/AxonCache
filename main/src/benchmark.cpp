#include <axoncache/logger/Logger.h>
#include "axoncache/capi/CacheWriterCApi.h"
#include "axoncache/capi/CacheReaderCApi.h"

#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <locale>
#include <sstream>
#include <random>

namespace
{

// custom facet to add thousands separators
struct comma_numpunct : std::numpunct<char>
{
  protected:
    char do_thousands_sep() const override
    {
        return ',';
    } // NOLINT
    std::string do_grouping() const override
    {
        return "\3";
    } // NOLINT
};

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

auto benchModeUnorderedMap(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void
{
    AL_LOG_INFO( "Bench mode unordered map" );
    using clock = std::chrono::steady_clock;

    std::unordered_map<std::string, std::string> cache;
    cache.reserve( numKeys );

    {
        auto start = clock::now();

        for ( int idx = 0; idx < numKeys; ++idx )
        {
            cache[keys[idx]] = vals[idx];
        }

        // Write/Flush the cache
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
        auto start = clock::now();

        for ( int idx = 0; idx < numKeys; ++idx )
        {
            auto val = cache[keys[idx]];
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

auto benchModeAxonCache(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void
{
    AL_LOG_INFO( "Bench mode axoncache" );
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
                throw std::runtime_error( "Error looking up valuereader" );
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
