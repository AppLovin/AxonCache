// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/CacheGenerator.h>
#include <axoncache/cache/CacheType.h>
#include <axoncache/version.h>

#include <axoncache/Constants.h>
#include <axoncache/loader/CacheOneTimeLoader.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/cache/MapCache.h>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/cache/factory/CacheFactory.h"
#include "axoncache/parser/CacheValueParser.h"

#include "CacheTestUtils.h"

#include "doctest/doctest.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>
#include <queue>
#include <unordered_set>
#include <cmath>
#include <ctime>

using namespace axoncache;

namespace
{

static auto currentTimeMillis() -> int64_t
{
    struct timespec curTime;
    clock_gettime( CLOCK_REALTIME, &curTime ); // signal-safe func, VS std::chrono::system_clock::now()

    return ( curTime.tv_sec * 1000 + curTime.tv_nsec / 1000000 );
}

}

static auto fullCacheTestWriteData( const std::string & dataFile, uint64_t numberOfStringKeys, uint64_t numberOfStringListKeys, uint64_t numberOfStringValues, uint64_t numberOfStringListValues, axoncache::SharedSettingsProvider * settings )
{
    srand( 0 );
    std::fstream dataFileOut( dataFile, std::ios::out | std::ios::binary );
    auto stringPairs = test_utils::gen_random_str_map_alpha_numeric( numberOfStringKeys, numberOfStringValues );
    const auto lineSeparator = settings->getChar( Constants::ConfKey::kControlCharLine, Constants::ConfDefault::kControlCharLine );
    const auto keyValueSeparator = settings->getChar( Constants::ConfKey::kControlCharKeyValue, Constants::ConfDefault::kControlCharKeyValue );
    const auto vectorIndicator = settings->getChar( Constants::ConfKey::kControlCharVectorType, Constants::ConfDefault::kControlCharVectorType );
    const auto elementSeparator = settings->getChar( Constants::ConfKey::kControlCharVectorElem, Constants::ConfDefault::kControlCharVectorElem );
    for ( const auto & pair : stringPairs )
    {
        dataFileOut << pair.first << keyValueSeparator << pair.second << lineSeparator;
    }

    auto stringListPairs = test_utils::gen_random_str_vec_map_alpha_numeric( numberOfStringListKeys, numberOfStringListValues );
    for ( const auto & pair : stringListPairs )
    {
        dataFileOut << vectorIndicator << pair.first << keyValueSeparator;
        std::string prefix;
        for ( const auto & str : pair.second )
        {
            dataFileOut << prefix << str;
            prefix = elementSeparator;
        }
        dataFileOut << lineSeparator;
    }

    dataFileOut.flush();
    dataFileOut.close();
}

static auto countLineGetTopValues( const std::string & cacheFile, const axoncache::SharedSettingsProvider * settings )
{
    const size_t kPickCount = 256;
    std::ifstream inputFile;
    std::string line;
    inputFile.open( cacheFile, std::ios::binary );
    CHECK( inputFile.is_open() );
    std::unordered_map<std::string, int> valueCounter;
    const auto lineSeparator = settings->getChar( Constants::ConfKey::kControlCharLine, Constants::ConfDefault::kControlCharLine );
    const auto keyValueSeparator = settings->getChar( Constants::ConfKey::kControlCharKeyValue, Constants::ConfDefault::kControlCharKeyValue );

    size_t lineCount = 0;
    while ( std::getline( inputFile, line, lineSeparator ) )
    {
        lineCount++;
        valueCounter[line.substr( line.find_first_of( keyValueSeparator ) + 1 )]++;
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
        if ( topValues.size() > kPickCount )
        {
            topValues.pop();
        }
    }
    std::vector<std::string> values;
    values.reserve( kPickCount );
    while ( !topValues.empty() )
    {
        values.emplace_back( topValues.top().second );
        topValues.pop();
    }
    return std::make_pair( std::move( values ), lineCount );
}

template<typename Cache>
static auto VerifyCache( const std::string & dataFile, const std::string & cacheName, const std::string & cacheFile, uint16_t offsetBits, axoncache::CacheType cacheType, double maxLoadFactor, const axoncache::SharedSettingsProvider & settings, const std::vector<std::string> & topValues, bool isIgnoreTopValues = false )
{
    // Load cache
    axoncache::CacheOneTimeLoader loader( &settings );
    auto cache = loader.loadAbsolutePath<Cache>( cacheName, cacheFile, false );

    CHECK( cache->hashcodeBits() == ( 64U - offsetBits ) );
    CHECK( cache->offsetBits() == offsetBits );
    CHECK( std::abs( cache->maxLoadFactor() - maxLoadFactor ) < 0.0001 );
    CHECK( cache->type() == cacheType );

    std::ifstream inputFile;
    std::string line;

    inputFile.open( dataFile, std::ios::binary );

    CHECK( inputFile.is_open() );
    auto parser = std::make_unique<CacheValueParser>( &settings );
    std::unordered_set<std::string> keySets;
    const char controlLine = settings.getChar( Constants::ConfKey::kControlCharLine, Constants::ConfDefault::kControlCharLine );
    while ( std::getline( inputFile, line, controlLine ) )
    {
        auto pair = parser->parseValue( line.data(), line.size() );
        if ( !keySets.insert( std::string{ pair.first.data(), pair.first.size() } ).second )
        {
            continue;
        }
        if ( pair.second.type() == CacheValueType::String )
        {
            auto value = cache->get( pair.first );
            CHECK( pair.second.asString() == value );
        }
        else if ( pair.second.type() == CacheValueType::StringList )
        {
            auto value = cache->getVector( pair.first );
            auto value2 = pair.second.asStringList();
            CHECK( value.size() == value2.size() );

            for ( auto i = 0U; i < value.size(); i++ )
            {
                CHECK( value[i] == value2[i] );
            }
        }
        else
        {
            CHECK( pair.second.type() == CacheValueType::String ); // fail if other types detected
        }
    }

    // Check duplicated values
    if constexpr ( std::is_same_v<Cache, axoncache::LinearProbeDedupCache> )
    {
        if ( !isIgnoreTopValues )
        {
            auto duplicatedValues = cache->getDuplicatedValues();
            CHECK( duplicatedValues.size() == topValues.size() );
            for ( auto index = 0U; index < topValues.size(); ++index )
            {
                CHECK( duplicatedValues[index] == topValues[index] );
            }
            CHECK_THROWS_WITH( cache->setDuplicatedValues( duplicatedValues ), "Values already loaded from memory" );
        }
    }
}

template<typename Cache>
static auto fullCacheTester( uint16_t offsetBits, double maxLoadFactor, uint64_t numberOfStringKeys, uint64_t numberOfStringListKeys, uint64_t numberOfKeySlots, const std::string & testName, uint64_t numberOfStringValues = 0UL, uint64_t numberOfStringListValues = 0UL, axoncache::CacheType cacheType = axoncache::CacheType::NONE ) -> void
{
    auto startMs = currentTimeMillis();
    std::string cacheName = "alcache_test_" + testName;
    if ( cacheType == axoncache::CacheType::NONE )
    {
        if constexpr ( std::is_same_v<Cache, axoncache::BucketChainCache> )
        {
            cacheType = axoncache::CacheType::BUCKET_CHAIN;
        }
        else if constexpr ( std::is_same_v<Cache, axoncache::LinearProbeCache> )
        {
            cacheType = axoncache::CacheType::LINEAR_PROBE;
        }
        else if constexpr ( std::is_same_v<Cache, axoncache::LinearProbeDedupCache> )
        {
            cacheType = axoncache::CacheType::LINEAR_PROBE_DEDUP;
        }
    }

    // write data file
    std::string dataFilename = "alcache_test_" + testName + "_";
    dataFilename.append( to_string( cacheType ) );
    dataFilename.append( "_input.dta" );
    const auto dataFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + dataFilename;

    // ex. "temp_dir/alcache_test.cache"
    const auto cacheFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + cacheName + std::string{ Constants::kCacheFileNameSuffix };

    std::string currentMsStr{ "1647455391370" };
    // ex. "temp_dir/alcache_test.cache.timestamp.latest"
    const auto latestTimestampFile = cacheFile + std::string{ Constants::kLatestTimestampFileNameSuffix };

    // ex. "temp_dir/alcache_test.1647455391370.cache"
    const auto latestCacheFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + cacheName + "." + currentMsStr + std::string{ Constants::kCacheFileNameSuffix };

    std::fstream foutStream = std::fstream( latestTimestampFile, std::ios::out | std::ios::binary );
    foutStream.write( currentMsStr.data(), currentMsStr.size() );
    foutStream.close();

    axoncache::SharedSettingsProvider settings( " " );

    settings.setSetting( axoncache::Constants::ConfKey::kCacheNames, cacheName ); // command line only supports one cache
    settings.setSetting( axoncache::Constants::ConfKey::kCacheType + "." + cacheName, std::to_string( static_cast<uint32_t>( cacheType ) ) );
    settings.setSetting( axoncache::Constants::ConfKey::kInputDir + "." + cacheName, std::filesystem::temp_directory_path().string() );
    settings.setSetting( axoncache::Constants::ConfKey::kLoadDir + "." + cacheName, std::filesystem::temp_directory_path().string() );
    settings.setSetting( axoncache::Constants::ConfKey::kInputFiles + "." + cacheName, dataFilename );
    settings.setSetting( axoncache::Constants::ConfKey::kOutputDir + "." + cacheName, std::filesystem::temp_directory_path().string() );
    settings.setSetting( axoncache::Constants::ConfKey::kKeySlots + "." + cacheName, std::to_string( numberOfKeySlots ) );
    settings.setSetting( axoncache::Constants::ConfKey::kOffsetBits + "." + cacheName, std::to_string( offsetBits ) );
    settings.setSetting( axoncache::Constants::ConfKey::kControlCharLine, "\n" );
    settings.setSetting( axoncache::Constants::ConfKey::kControlCharVectorType, "\t" );
    settings.setSetting( axoncache::Constants::ConfKey::kMaxLoadFactor + "." + cacheName, std::to_string( maxLoadFactor ) );

    fullCacheTestWriteData( dataFile, numberOfStringKeys, numberOfStringListKeys, numberOfStringValues, numberOfStringListValues, &settings );

    // Generate cache
    axoncache::CacheGenerator alCache( &settings );
    std::vector<std::string> topValues;
    if constexpr ( std::is_same_v<Cache, axoncache::LinearProbeDedupCache> )
    {
        auto info = countLineGetTopValues( dataFile, &settings );
        topValues = std::move( info.first );
        alCache.start( topValues );
    }
    else
    {
        alCache.start();
    }

    VerifyCache<Cache>( dataFile, cacheName, cacheFile, offsetBits, cacheType, maxLoadFactor, settings, topValues );

    axoncache::CacheOneTimeLoader loader( &settings );
    CHECK_THROWS_AS( loader.loadLatest<Cache>( cacheName ), std::runtime_error );
    std::filesystem::rename( cacheFile, latestCacheFile );

    auto cache = loader.loadLatest<Cache>( cacheName );
    CHECK( cache->creationTimeMs() >= startMs );
    CHECK( loader.getTimestamp() == currentMsStr );

    std::filesystem::remove( dataFile );
    std::filesystem::remove( latestTimestampFile );
    std::filesystem::remove( latestCacheFile );
}

TEST_CASE( "BucketCacheTest" )
{
    const uint16_t offsetBits = 64U;
    const auto maxLoadFactor = 1.0;
    const auto numberOfStringKeys = 10000;
    const auto numberOfStringListKeys = 1000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::BucketChainCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "bucket_cache" );
}

TEST_CASE( "LinearProbeCacheOfs16Test" )
{
    const uint16_t offsetBits = 16U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 5;
    const auto numberOfStringListKeys = 5;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe16" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe16" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe16", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeCacheOfs31Test" )
{
    const uint16_t offsetBits = 31U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 10000;
    const auto numberOfStringListKeys = 1000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe31" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe31" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe31", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeCacheOfs32Test" )
{
    const uint16_t offsetBits = 32U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 10000;
    const auto numberOfStringListKeys = 1000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe32" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe32" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe32", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeCacheOfs34Test" )
{
    const uint16_t offsetBits = 34U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 10000;
    const auto numberOfStringListKeys = 1000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe34" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe34" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe34", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeCacheOfs35Test" )
{
    const uint16_t offsetBits = 35U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 20000;
    const auto numberOfStringListKeys = 2000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe35" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe35" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe35", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeCacheOfs28Test" )
{
    const uint16_t offsetBits = 28U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 20000;
    const auto numberOfStringListKeys = 2000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe28" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe28" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe28", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeCacheBigOffsetTest" )
{
    const uint16_t offsetBits = 35U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 10000;
    const auto numberOfStringListKeys = 1000;
    const auto numberOfKeySlots = 17000000;

    fullCacheTester<axoncache::LinearProbeCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe35_big" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe35_big" );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linear_probe35_big", 0UL, 0UL, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeDedupCacheOfs28Test" )
{
    const uint16_t offsetBits = 28U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 20000;
    const auto numberOfStringValues = 2000;
    const auto numberOfStringListKeys = 2000;
    const auto numberOfStringListValues = 200;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linearprobededup28", numberOfStringValues, numberOfStringListValues );
    fullCacheTester<axoncache::LinearProbeDedupCache>( offsetBits, maxLoadFactor, numberOfStringKeys, numberOfStringListKeys, numberOfKeySlots, "linearprobededup28", numberOfStringValues, numberOfStringListValues, axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
}

TEST_CASE( "LinearProbeForwardIncompatibility" )
{
    CacheHeader header;
    header.cacheType = static_cast<uint16_t>( axoncache::CacheType::LINEAR_PROBE_DEDUP );
    header.offsetBits = static_cast<uint16_t>( 20 );
    header.headerSize = sizeof( CacheHeader );
    header.nameStart = offsetof( CacheHeader, cacheName );
    const std::string cacheName = "test_incompatibility";
    snprintf( header.cacheName, std::min( cacheName.size() + 1, Constants::kMaxCacheNameSize ), "%s", cacheName.data() );
    const std::string cacheFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + cacheName + std::string{ Constants::kCacheFileNameSuffix };
    std::fstream output = std::fstream( cacheFile, std::ios::out | std::ios::binary );
    output.write( reinterpret_cast<const char *>( &header ), sizeof( CacheHeader ) );
    output.put( 0 );
    output.close();
    axoncache::CacheOneTimeLoader loader( nullptr );
    CHECK_THROWS_WITH( loader.loadAbsolutePath<axoncache::LinearProbeCache>( cacheName, cacheFile, false ), "LINEAR_PROBE cache can't load LINEAR_PROBE_DEDUP or LINEAR_PROBE_DEDUP_TYPED cache data" );
    std::filesystem::remove( cacheFile );
}

TEST_CASE( "LinearProbeDedupSetDuplicatedValues" )
{
    const uint16_t offsetBits = 28U;
    const auto maxLoadFactor = 0.5;
    const auto numberOfStringKeys = 20000;
    const auto numberOfStringListKeys = 2000;
    const auto numberOfKeys = numberOfStringKeys + numberOfStringListKeys;
    const auto numberOfKeySlots = static_cast<uint64_t>( std::ceil( static_cast<double>( numberOfKeys ) / maxLoadFactor ) );

    // Check setDuplicatedValues API
    std::vector<std::string> duplicatedValues = { "value1", "value2" };
    auto cacheBase = CacheFactory::createCache( offsetBits, numberOfKeySlots, maxLoadFactor, axoncache::CacheType::LINEAR_PROBE_DEDUP );
    CHECK_THROWS_WITH( ( ( LinearProbeDedupCache * )cacheBase.get() )->setDuplicatedValues( std::vector<std::string>( 70000 ) ), "Should not set more than 65536 duplicated values" );
    ( ( LinearProbeDedupCache * )cacheBase.get() )->setDuplicatedValues( duplicatedValues );
    CHECK_THROWS_WITH( ( ( LinearProbeDedupCache * )cacheBase.get() )->setDuplicatedValues( duplicatedValues ), "Values already set, call this API only once" );
}

TEST_CASE( "GenerateCacheFromFileAndVerify" )
{
    std::cout << "Current path = " << std::filesystem::current_path().string() << " \n";
#ifdef BAZEL_BUILD
    const std::string inputFolder = "test/data"; // Bazel makes test data available directly
#else
    const std::string inputFolder = axoncache::test_utils::resolveResourcePath( "../../test/data", __FILE__ );
#endif
    const std::string inputFile = "example_fast_data.dta";
    const std::string cacheName = "generated_cache.cache";
    auto cacheType = axoncache::CacheType::LINEAR_PROBE_DEDUP;
    const uint16_t offsetBits = 30U;
    const double maxLoadFactor = 0.5f;
    const std::string outputFolder = std::filesystem::temp_directory_path().string();
    const auto dataFile = inputFolder + std::filesystem::path::preferred_separator + inputFile;
    const auto cacheFile = outputFolder + std::filesystem::path::preferred_separator + cacheName + std::string{ Constants::kCacheFileNameSuffix };

    axoncache::SharedSettingsProvider settings( " " );
    settings.setSetting( axoncache::Constants::ConfKey::kCacheNames, cacheName ); // command line only supports one cache
    settings.setSetting( axoncache::Constants::ConfKey::kCacheType + "." + cacheName, std::to_string( static_cast<uint32_t>( cacheType ) ) );
    settings.setSetting( axoncache::Constants::ConfKey::kInputDir + "." + cacheName, inputFolder );
    settings.setSetting( axoncache::Constants::ConfKey::kLoadDir + "." + cacheName, outputFolder );
    settings.setSetting( axoncache::Constants::ConfKey::kInputFiles + "." + cacheName, inputFile );
    settings.setSetting( axoncache::Constants::ConfKey::kOutputDir + "." + cacheName, outputFolder );
    settings.setSetting( axoncache::Constants::ConfKey::kOffsetBits + "." + cacheName, std::to_string( offsetBits ) );
    settings.setSetting( axoncache::Constants::ConfKey::kMaxLoadFactor + "." + cacheName, std::to_string( maxLoadFactor ) );

    auto info = countLineGetTopValues( dataFile, &settings );
    const std::vector<std::string> topValues = std::move( info.first );
    auto numberOfKeySlots = ( uint64_t )( info.second / maxLoadFactor );
    settings.setSetting( axoncache::Constants::ConfKey::kKeySlots + "." + cacheName, std::to_string( numberOfKeySlots ) );
    const axoncache::CacheGenerator alCache( &settings );
    alCache.start( topValues );
    VerifyCache<LinearProbeDedupCache>( dataFile, cacheName, cacheFile, offsetBits, cacheType, maxLoadFactor, settings, topValues );
}

TEST_CASE( "LoadCacheFromTmpAndVerify" )
{
#ifdef BAZEL_BUILD
    const std::string inputFolder = "test/data"; // Bazel makes test data available directly
#else
    const std::string inputFolder = axoncache::test_utils::resolveResourcePath( "../../test/data", __FILE__ );
#endif
    const std::string inputFile = "example_fast_data.dta";
    const std::string cacheName = "generated_cache.cache";
    auto cacheType = axoncache::CacheType::LINEAR_PROBE_DEDUP;
    const uint16_t offsetBits = 30U;
    const double maxLoadFactor = 0.5f;
    const std::string outputFolder = std::filesystem::temp_directory_path().string();
    const auto dataFile = inputFolder + std::filesystem::path::preferred_separator + inputFile;
    const auto cacheFile = outputFolder + std::filesystem::path::preferred_separator + cacheName + std::string{ Constants::kCacheFileNameSuffix };

    axoncache::SharedSettingsProvider settings( "" );
    settings.setSetting( axoncache::Constants::ConfKey::kCacheNames, cacheName ); // command line only supports one cache
    settings.setSetting( axoncache::Constants::ConfKey::kCacheType + "." + cacheName, std::to_string( static_cast<uint32_t>( cacheType ) ) );
    settings.setSetting( axoncache::Constants::ConfKey::kInputDir + "." + cacheName, inputFolder );
    settings.setSetting( axoncache::Constants::ConfKey::kLoadDir + "." + cacheName, outputFolder );
    settings.setSetting( axoncache::Constants::ConfKey::kInputFiles + "." + cacheName, inputFile );
    settings.setSetting( axoncache::Constants::ConfKey::kOutputDir + "." + cacheName, outputFolder );
    settings.setSetting( axoncache::Constants::ConfKey::kOffsetBits + "." + cacheName, std::to_string( offsetBits ) );
    settings.setSetting( axoncache::Constants::ConfKey::kMaxLoadFactor + "." + cacheName, std::to_string( maxLoadFactor ) );

    bool isFileExists = std::filesystem::exists( cacheFile );
    std::cout << "Current path = " << std::filesystem::current_path().string() << ", dataFile = " << dataFile << ", cacheFile = " << cacheFile << " is" << ( isFileExists ? "" : " not" ) << " exists\n";
    if ( isFileExists )
    {
        VerifyCache<LinearProbeDedupCache>( dataFile, cacheName, cacheFile, offsetBits, cacheType, maxLoadFactor, settings, std::vector<std::string>{}, true );
    }
}
