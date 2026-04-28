// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <memory>
#include <utility>
#include <cstdint>
#include <stdexcept>
#include "axoncache/logger/Logger.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/cache/LinearProbeCache.h"
#include "axoncache/domain/CacheHeader.h"
#include "axoncache/memory/MmapMemoryHandler.h"
namespace axoncache
{
class SharedSettingsProvider;
}

namespace axoncache
{
class CacheOneTimeLoader
{
  public:
    CacheOneTimeLoader( const axoncache::SharedSettingsProvider * settings ) :
        mSettings( settings )
    {
    }

    template<typename Cache>
    auto load( const std::string & cacheName, bool isPreloadMemoryEnabled = false ) -> std::shared_ptr<Cache>
    {
        auto cacheFileName = getFullCacheFileName( cacheName );
        return loadAbsolutePath<Cache>( cacheName, cacheFileName, isPreloadMemoryEnabled );
    }

    template<typename Cache>
    auto loadLatest( const std::string & cacheName, bool isPreloadMemoryEnabled = false ) -> std::shared_ptr<Cache>
    {
        auto cacheFileName = getLatestTimestampFullCacheFileName( cacheName );
        return loadAbsolutePath<Cache>( cacheName, cacheFileName, isPreloadMemoryEnabled );
    }

    template<typename Cache>
    auto loadAbsolutePath( const std::string & cacheName, const std::string & cacheFileName, bool isPreloadMemoryEnabled ) -> std::shared_ptr<Cache>
    {
        const auto [name, header] = loadHeader( cacheFileName );
        AL_LOG_INFO( "opened axoncache " + cacheFileName );
        if ( name != cacheName )
        {
            AL_LOG_WARN( "Loading cache name does not match the name in the header" );
        }

        if constexpr ( std::is_same_v<Cache, axoncache::LinearProbeCache> )
        {
            if ( header.cacheType == static_cast<uint16_t>( CacheType::LINEAR_PROBE_DEDUP ) || header.cacheType == static_cast<uint16_t>( CacheType::LINEAR_PROBE_DEDUP_TYPED ) )
            {
                throw std::runtime_error( "LINEAR_PROBE cache can't load LINEAR_PROBE_DEDUP or LINEAR_PROBE_DEDUP_TYPED cache data" );
            }
        }

        auto cache = std::make_shared<Cache>( header, std::make_unique<axoncache::MmapMemoryHandler>( header, cacheFileName, isPreloadMemoryEnabled ) );

        if ( header.version != cache->version() )
        {
            throw std::runtime_error( "trying to load file version " + std::to_string( header.version ) + " with a runtime version " + std::to_string( cache->version() ) );
        }

        mTimestamp = cacheFileName.substr( 0, cacheFileName.length() - Constants::kCacheFileNameSuffix.size() );
        mTimestamp = mTimestamp.substr( mTimestamp.find_last_not_of( "0123456789" ) + 1 );
        return cache;
    }

    [[nodiscard]] auto loadHeader( const std::string & cacheFile ) const -> std::pair<std::string, CacheHeader>;

    // return ex. /var/lib/applovin/datamover/test_cache.cache
    [[nodiscard]] auto getFullCacheFileName( const std::string & cacheName ) const -> std::string;

    // ex. open /var/lib/applovin/datamover/test_cache.cache.timestamp.latest
    // and return "/var/lib/applovin/datamover/test_cache.1651622570800.cache"
    [[nodiscard]] auto getLatestTimestampFullCacheFileName( const std::string & cacheName ) const -> std::string;

    // ex. open test_cache.timestamp.latest
    // and return "/var/lib/applovin/datamover/test_cache.1651622570800.cache"
    [[nodiscard]] auto getTimestampFullCacheFileName( const std::string & cacheName, const int64_t timestamp ) const -> std::string;
    auto getTimestamp() const -> std::string;

  protected:
    [[nodiscard]] auto settings() const -> const SharedSettingsProvider *
    {
        return mSettings;
    }

  private:
    const SharedSettingsProvider * mSettings{ nullptr };
    std::string mTimestamp{};
};
}
