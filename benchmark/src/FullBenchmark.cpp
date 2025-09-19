// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/CacheGenerator.h>
#include <axoncache/cache/CacheType.h>
#include <axoncache/version.h>
#include <axoncache/Constants.h>
#include <axoncache/loader/CacheOneTimeLoader.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <axoncache/cache/MapCache.h>

#include <axoncache/common/SharedSettingsProvider.h>
#include <benchmark/benchmark.h>
#include "CacheBenchmarkUtils.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>

namespace axoncache
{
template<typename Cache, CacheType CacheT>
class FullBenchmark : public benchmark::Fixture
{
  public:
    void SetUp( [[maybe_unused]] const ::benchmark::State & state )
    {
        srand( 0 );
        // TODO: maybe use real world data herer
        std::string cacheName = "alcache_benchmark";

        // write data file
        auto dataFilename = "alcache_benchmark_input.dta";
        auto dataFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + dataFilename;
        auto cacheFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + cacheName + std::string{ Constants::kCacheFileNameSuffix };

        SharedSettingsProvider settings( "" );

        const auto numberOfKeySlots = 1U << 20;
        const uint16_t offsetBits = 30U;
        const double maxLoadFactor = 0.5f;

        settings.setSetting( axoncache::Constants::ConfKey::kMaxLoadFactor, std::to_string( maxLoadFactor ) );
        settings.setSetting( axoncache::Constants::ConfKey::kCacheNames, cacheName );                                             // command line only supports one cache
        settings.setSetting( axoncache::Constants::ConfKey::kCacheType + "." + cacheName, std::to_string( ( uint32_t )CacheT ) ); // use BucketChainCache
        settings.setSetting( axoncache::Constants::ConfKey::kInputDir + "." + cacheName, std::filesystem::temp_directory_path().string() );
        settings.setSetting( axoncache::Constants::ConfKey::kLoadDir + "." + cacheName, std::filesystem::temp_directory_path().string() );
        settings.setSetting( axoncache::Constants::ConfKey::kInputFiles + "." + cacheName, dataFilename );
        settings.setSetting( axoncache::Constants::ConfKey::kKeySlots + "." + cacheName, std::to_string( numberOfKeySlots ) );
        settings.setSetting( axoncache::Constants::ConfKey::kOffsetBits + "." + cacheName, std::to_string( offsetBits ) );
        settings.setSetting( axoncache::Constants::ConfKey::kKeySlots + "." + cacheName, std::to_string( numberOfKeySlots ) );
        settings.setSetting( axoncache::Constants::ConfKey::kOutputDir + "." + cacheName, std::filesystem::temp_directory_path().string() );
        settings.setSetting( axoncache::Constants::ConfKey::kControlCharLine, "\n" );
        settings.setSetting( axoncache::Constants::ConfKey::kControlCharVectorType, "\t" );

        const auto strKeyRatio = 0.8;                                     // 80% string keys, 20% vector keys
        const auto numberOfStringKeys = numberOfKeySlots * maxLoadFactor; // target 85% load factor
        std::vector<std::string> topValues;
        std::tie( mStringKeys, mStringListKeys, topValues ) = fullCacheTestWriteData( dataFile, numberOfStringKeys * strKeyRatio, numberOfStringKeys * ( 1.0 - strKeyRatio ) );

        // Generate cache
        axoncache::CacheGenerator alCache( &settings );
        if constexpr ( std::is_same_v<Cache, axoncache::LinearProbeDedupCache> )
        {
            alCache.start( topValues );
        }
        else
        {
            alCache.start();
        }

        // Load cache
        axoncache::CacheOneTimeLoader loader( &settings );
        mCache = loader.load<Cache>( cacheName );
    }

    void TearDown( [[maybe_unused]] const ::benchmark::State & state )
    {
        std::filesystem::remove( mDataFile );
        std::filesystem::remove( mCacheFile );
        mCache.reset();
    }

    static auto getTopValues( const std::pair<std::map<std::string, std::string>, std::map<std::string, std::vector<std::string>>> & keyPairs, size_t pickCount = 256 ) -> std::vector<std::string>
    {
        std::unordered_map<std::string, int> valueCounter;
        for ( const auto & pair : keyPairs.first )
        {
            valueCounter[pair.second]++;
        }
        for ( const auto & pair : keyPairs.second )
        {
            std::vector<std::string_view> valueViews;
            valueViews.reserve( pair.second.size() );
            for ( const auto & str : pair.second )
            {
                valueViews.emplace_back( str.data(), str.size() );
            }
            auto value = StringListToString::transform( valueViews );
            if ( !value.empty() )
            {
                value.pop_back(); // remove null terminator
            }
            valueCounter[value]++;
        }

        auto comp = []( const std::pair<int, std::string_view> & pair1, const std::pair<int, std::string_view> & pair2 ) -> bool
        {
            return pair1.first * pair1.second.size() > pair2.first * pair2.second.size();
        };

        std::priority_queue<std::pair<int, std::string_view>, std::vector<std::pair<int, std::string_view>>, decltype( comp )> topValues( comp );
        for ( auto & valueOccurrence : valueCounter )
        {
            if ( valueOccurrence.second > 1 )
            {
                topValues.emplace( valueOccurrence.second, std::string_view{ valueOccurrence.first.data(), valueOccurrence.first.size() } );
            }
            if ( topValues.size() > pickCount )
            {
                topValues.pop();
            }
        }
        std::vector<std::string> values;
        values.reserve( pickCount );
        while ( !topValues.empty() )
        {
            values.emplace_back( topValues.top().second );
            topValues.pop();
        }
        return values;
    }

    static auto fullCacheTestWriteData( const std::string & dataFile, uint64_t numberOfStringKeys, uint64_t numberOfStringListKeys )
    {
        std::fstream dataFileOut( dataFile, std::ios::out | std::ios::binary );

        auto stringPairs = benchmark_utils::gen_random_str_map_alpha_numeric( numberOfStringKeys );
        auto stringListPairs = benchmark_utils::gen_random_str_vec_map_alpha_numeric( numberOfStringListKeys );

        std::vector<std::string> stringKeys;
        std::vector<std::string> stringListKeys;

        for ( const auto & pair : stringPairs )
        {
            dataFileOut << pair.first << "=" << pair.second << "\n";
            stringKeys.push_back( pair.first );
        }

        for ( const auto & pair : stringListPairs )
        {
            dataFileOut << "\t" << pair.first << "=";
            auto prefix = "";
            for ( const auto & str : pair.second )
            {
                dataFileOut << prefix << str;
                prefix = "|";
            }
            dataFileOut << "\n";
            stringListKeys.push_back( pair.first );
        }

        dataFileOut.flush();
        dataFileOut.close();
        return std::make_tuple( stringKeys, stringListKeys, getTopValues( std::make_pair( std::move( stringPairs ), std::move( stringListPairs ) ) ) );
    }

    void lookup( benchmark::State & state )
    {
        auto ix = 0UL;
        for ( auto _ : state )
        {
            ix = ix >= mStringKeys.size() ? 0UL : ix;
            const auto & key = mStringKeys[ix];
            benchmark::DoNotOptimize( mCache->get( key ) );
            ++ix;
        }
    }

    void negativeLookup( benchmark::State & state )
    {
        // This is *not* guranteed to be 100% negative lookups but should mostly be so
        std::vector<std::string> keys;
        keys.reserve( 10000 );
        for ( auto ix = 0; ix < 10000; ++ix )
        {
            keys.push_back( benchmark_utils::gen_random( ( rand() % 10 ) + 10 ) );
        }

        auto ix = 0UL;
        for ( auto _ : state )
        {
            ix = ix >= keys.size() ? 0UL : ix;
            const auto & key = keys[ix];
            benchmark::DoNotOptimize( mCache->getVector( key ) );
            ++ix;
        }
    }
    std::string mDataFile;
    std::string mCacheFile;
    std::vector<std::string> mStringKeys;
    std::vector<std::string> mStringListKeys;
    std::shared_ptr<Cache> mCache;
};

}

using namespace axoncache;

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, BucketChainLookup, BucketChainCache, CacheType::BUCKET_CHAIN )
( benchmark::State & state )
{
    lookup( state );
}

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, LinearProbeLookup, LinearProbeCache, CacheType::LINEAR_PROBE )
( benchmark::State & state )
{
    lookup( state );
}

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, LinearProbeNegativeLookup, LinearProbeCache, CacheType::LINEAR_PROBE )
( benchmark::State & state )
{
    negativeLookup( state );
}

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, BucketChainNegativeLookup, BucketChainCache, CacheType::BUCKET_CHAIN )
( benchmark::State & state )
{
    negativeLookup( state );
}

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, BucketChainListLookup, BucketChainCache, CacheType::BUCKET_CHAIN )
( benchmark::State & state )
{
    auto ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= mStringListKeys.size() ? 0UL : ix;
        const auto & key = mStringListKeys[ix];
        benchmark::DoNotOptimize( mCache->getVector( key ) );
        ++ix;
    }
}

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, LinearProbeDedupLookup, LinearProbeDedupCache, CacheType::LINEAR_PROBE_DEDUP )
( benchmark::State & state )
{
    lookup( state );
}

BENCHMARK_TEMPLATE_DEFINE_F( FullBenchmark, LinearProbeDedupNegativeLookup, LinearProbeDedupCache, CacheType::LINEAR_PROBE_DEDUP )
( benchmark::State & state )
{
    negativeLookup( state );
}

uint64_t start = ( 1U << 20U ); // 1U << 20U: 1M
uint64_t end = ( 1U << 20U );

BENCHMARK_REGISTER_F( FullBenchmark, BucketChainLookup )->Range( start, end );
BENCHMARK_REGISTER_F( FullBenchmark, BucketChainNegativeLookup )->Range( start, end );
//BENCHMARK_REGISTER_F( FullBenchmark, BucketChainListLookup )->Range( start, end );

BENCHMARK_REGISTER_F( FullBenchmark, LinearProbeLookup )->Range( start, end );
BENCHMARK_REGISTER_F( FullBenchmark, LinearProbeNegativeLookup )->Range( start, end );

BENCHMARK_REGISTER_F( FullBenchmark, LinearProbeDedupLookup )->Range( start, end );
BENCHMARK_REGISTER_F( FullBenchmark, LinearProbeDedupNegativeLookup )->Range( start, end );
