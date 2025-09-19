// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/cache/factory/CacheFactory.h"
#include <stdexcept>
#include "axoncache/cache/BucketChainCache.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/cache/LinearProbeCache.h"
#include "axoncache/cache/LinearProbeDedupCache.h"
#include "axoncache/cache/MapCache.h"
#include "axoncache/memory/MallocMemoryHandler.h"
namespace axoncache
{
class CacheBase;
}

using namespace axoncache;

auto CacheFactory::createCache( uint16_t offsetBits, uint64_t numberOfKeySlots, double maxLoadFactor, CacheType type ) -> std::unique_ptr<CacheBase>
{
    switch ( type )
    {
        case CacheType::MAP:
            return std::make_unique<MapCache>( std::make_unique<MallocMemoryHandler>() );
        case CacheType::BUCKET_CHAIN:
            return std::make_unique<BucketChainCache>( offsetBits, numberOfKeySlots, maxLoadFactor, std::make_unique<MallocMemoryHandler>() );
        case CacheType::LINEAR_PROBE:
            return std::make_unique<LinearProbeCache>( offsetBits, numberOfKeySlots, maxLoadFactor, std::make_unique<MallocMemoryHandler>() );
        case CacheType::LINEAR_PROBE_DEDUP:
            return std::make_unique<LinearProbeDedupCache>( offsetBits, numberOfKeySlots, maxLoadFactor, std::make_unique<MallocMemoryHandler>(), CacheType::LINEAR_PROBE_DEDUP );
        case CacheType::LINEAR_PROBE_DEDUP_TYPED:
            return std::make_unique<LinearProbeDedupCache>( offsetBits, numberOfKeySlots, maxLoadFactor, std::make_unique<MallocMemoryHandler>(), CacheType::LINEAR_PROBE_DEDUP_TYPED );
        case CacheType::NONE:
            throw std::runtime_error( "CacheFactory::createCache: CacheType::None is not a valid CacheType" );
    }

    throw std::runtime_error( "CacheFactory::createCache: CacheType::None is not a valid CacheType" );
}
