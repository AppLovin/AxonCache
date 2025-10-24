// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.
//
#pragma once

#include <axoncache/logger/Logger.h>

#include <string>
#include <chrono>
#include <iomanip>
#include <locale>
#include <sstream>
#include <random>

auto benchModeAxonCacheCppApi(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void;

auto benchModeAxonCacheCApi(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void;

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

void writeFile( const std::string & filename, const std::string & content );

template<class T>
auto benchModeHashTable(
    const std::string & label,
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void
{
    AL_LOG_INFO( "Using " + label );
    using clock = std::chrono::steady_clock;

    T cache;
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
            << "s (" << ssQps.str() << " keys/sec)";
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
