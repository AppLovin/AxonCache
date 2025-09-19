// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/CacheGenerator.h>
#include <axoncache/Constants.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <benchmark/benchmark.h>
#include "CacheBenchmarkUtils.h"

#include <string>
using namespace axoncache;

static void BucketedChainedLookup( benchmark::State & state )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = state.range( 0 );
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = benchmark_utils::gen_random_str_map( numberOfKeysToTest );
    auto keys = std::vector<std::string>();
    const auto maxLoadFactor = 0.8;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    for ( const auto & str : strMap )
    {
        cache.put( str.first, str.second );
        keys.push_back( str.first );
    }

    auto ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= keys.size() ? 0UL : ix;
        const auto & key = keys[ix];
        benchmark::DoNotOptimize( cache.get( key ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

static void BucketedChainedNegativeLookup( benchmark::State & state )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = state.range( 0 );
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = benchmark_utils::gen_random_str_map( numberOfKeysToTest );
    const auto maxLoadFactor = 0.8;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    for ( const auto & str : strMap )
    {
        cache.put( str.first, str.second );
    }

    auto keys = std::vector<std::string>();
    auto ix = 0UL;
    for ( ; ix < static_cast<uint64_t>( numberOfKeysToTest ); ++ix )
    {
        auto key = benchmark_utils::gen_random( ( rand() % 10 ) + 10 );
        while ( strMap.contains( key ) )
        {
            key = benchmark_utils::gen_random( ( rand() % 10 ) + 10 );
        }
        keys.push_back( key );
    }

    ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= keys.size() ? 0UL : ix;
        const auto & key = keys[ix];
        benchmark::DoNotOptimize( cache.get( key ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

static void BucketedChainedListLookup( benchmark::State & state )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = state.range( 0 );
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( numberOfKeysToTest * sizeof( uint64_t ) );
    const auto strMap = benchmark_utils::gen_random_str_vec_map( numberOfKeysToTest );
    auto keys = std::vector<std::string>();
    const auto maxLoadFactor = 0.8;

    BucketChainCache cache( offsetBits, numberOfKeysToTest, maxLoadFactor, std::move( memoryHandler ) );

    for ( const auto & str : strMap )
    {
        std::vector<std::string_view> list;
        for ( const auto & elem : str.second )
        {
            list.push_back( std::string_view( elem ) );
        }
        cache.put( str.first, std::move( list ) );
        keys.push_back( str.first );
    }

    auto ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= keys.size() ? 0UL : ix;
        const auto & key = keys[ix];
        benchmark::DoNotOptimize( cache.getVector( key ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

static void BucketedChainedHighCollision( benchmark::State & state )
{
    const uint16_t offsetBits = 64U;
    const auto numberOfKeysToTest = state.range( 0 );
    auto memoryHandler = std::make_unique<MallocMemoryHandler>( 1024 * sizeof( uint64_t ) );
    const auto strMap = benchmark_utils::gen_random_str_map( numberOfKeysToTest );
    auto keys = std::vector<std::string>();
    const auto maxLoadFactor = 0.8;

    BucketChainCache cache( offsetBits, 1024, maxLoadFactor, std::move( memoryHandler ) );

    for ( const auto & str : strMap )
    {
        cache.put( str.first, str.second );
        keys.push_back( str.first );
    }

    auto ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= keys.size() ? 0UL : ix;
        const auto & key = keys[ix];
        benchmark::DoNotOptimize( cache.get( key ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

// BENCHMARK( BucketedChainedLookup )->Range( 4096UL, 1UL << 15U );
// BENCHMARK( BucketedChainedNegativeLookup )->Range( 4096UL, 1UL << 15U );
// BENCHMARK( BucketedChainedListLookup )->Range( 4096UL, 1UL << 15U );
// BENCHMARK( BucketedChainedHighCollision )->Range( 4096UL, 1UL << 15U );
