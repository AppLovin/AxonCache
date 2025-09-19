// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/CacheGenerator.h>
#include <axoncache/Constants.h>
#include <axoncache/cache/value/ChainedValue.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <benchmark/benchmark.h>
#include "CacheBenchmarkUtils.h"

#include <string>
using namespace axoncache;

static void ValueGetChained( benchmark::State & state )
{
    const auto numberOfKeysToTest = state.range( 0 );
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );
    const auto strMap = benchmark_utils::gen_random_str_map( numberOfKeysToTest );
    auto keys = std::vector<std::string>();

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    auto keyOffset = 0UL;
    for ( const auto & str : strMap )
    {
        value.add( keyOffset, str.first, unusedHashcode, 1, str.second, &memoryHandler );
        keys.push_back( str.first );
        keyOffset += sizeof( uint64_t );
    }

    auto ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= keys.size() ? 0UL : ix;
        const auto & key = keys[ix];
        benchmark::DoNotOptimize( value.get( memoryHandler.data(), ix * sizeof( uint64_t ), key.data(), key.size() ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

static void ValueGetChainedNegativeLookup( benchmark::State & state )
{
    const auto numberOfKeysToTest = state.range( 0 );
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * numberOfKeysToTest );
    const auto strMap = benchmark_utils::gen_random_str_map( numberOfKeysToTest );

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    auto keyOffset = 0UL;
    for ( const auto & str : strMap )
    {
        value.add( keyOffset, str.first, unusedHashcode, 1, str.second, &memoryHandler );
        keyOffset += sizeof( uint64_t );
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
        benchmark::DoNotOptimize( value.get( memoryHandler.data(), ix * sizeof( uint64_t ), key.data(), key.size() ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

static void ValueGetChainedHighCollision( benchmark::State & state )
{
    // All keys go to one bucket/chain
    const auto numberOfKeysToTest = state.range( 0 );
    MallocMemoryHandler memoryHandler;
    memoryHandler.allocate( sizeof( uint64_t ) * 1024 );
    const auto strMap = benchmark_utils::gen_random_str_map( numberOfKeysToTest );
    auto keys = std::vector<std::string>();

    ChainedValue value( 0U, 0UL, 0UL, 0UL );
    uint64_t unusedHashcode = 0xabcd;
    auto keyOffset = 0UL;
    for ( const auto & str : strMap )
    {
        value.add( keyOffset, str.first, unusedHashcode, 1, str.second, &memoryHandler );
        keys.push_back( str.first );
    }

    auto ix = 0UL;
    for ( auto _ : state )
    {
        ix = ix >= keys.size() ? 0UL : ix;
        const auto & key = keys[ix];
        benchmark::DoNotOptimize( value.get( memoryHandler.data(), 0UL, key.data(), key.size() ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

// BENCHMARK( ValueGetChained )->Range( 8UL, 1UL << 12U );
// BENCHMARK( ValueGetChainedNegativeLookup )->Range( 8UL, 1UL << 12U );
// BENCHMARK( ValueGetChainedHighCollision )->Range( 8UL, 1UL << 12U );
