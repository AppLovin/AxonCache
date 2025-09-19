// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <benchmark/benchmark.h>
#include <axoncache/CacheGenerator.h>
#include <axoncache/cache/probe/SimpleProbe.h>
#include <axoncache/Constants.h>
#include <axoncache/cache/probe/SimpleProbe.h>

#include <string>
using namespace axoncache;

static void ProbeFindSimple( benchmark::State & state )
{
    std::vector<uint64_t> keySlots;
    SimpleProbe<8> probe( 64U, state.range( 0 ) );
    auto ix = 0UL;
    auto numberOfKeys = 10000UL;
    uint32_t collisions;
    std::string_view dummy;

    for ( ; ix < numberOfKeys; ++ix )
    {
        keySlots.push_back( rand() % probe.numberOfKeySlots() );
    }

    for ( auto _ : state )
    {
        ix = ix >= numberOfKeys ? 0UL : ix;
        benchmark::DoNotOptimize( probe.findFreeKeySlotOffset( dummy, keySlots[ix++], nullptr, collisions ) );
        benchmark::ClobberMemory();
        ++ix;
    }
}

//BENCHMARK( ProbeFindSimple )->Range( 8UL, 1UL << 12 );
