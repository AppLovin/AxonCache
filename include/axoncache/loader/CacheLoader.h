// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <memory>
#include <utility>
#include "axoncache/logger/Logger.h"
#include "axoncache/loader/CacheOneTimeLoader.h"
class SharedSettingsProvider;

namespace axoncache
{
template<typename Cache>
class CacheLoader
{
  public:
    CacheLoader( std::string cacheName, const SharedSettingsProvider * settings ) :
        mCacheName( std::move( cacheName ) ),
        mSettings( settings ),
        mCacheOneTimeLoader( settings ),
        mLastCacheFileModificationTime( std::filesystem::file_time_type::min() )
    {
    }

    auto load( bool isPreloadMemoryEnabled = true ) -> std::pair<bool, std::shared_ptr<Cache>>
    {
        // ex. fullCacheFileName = "/var/lib/applovin/datamover/test_cache.cache"
        const auto fullCacheFileName = mCacheOneTimeLoader.getFullCacheFileName( cacheName() );
        return loadWithName( fullCacheFileName, isPreloadMemoryEnabled );
    }

    auto loadLatest( bool isPreloadMemoryEnabled = true ) -> std::pair<bool, std::shared_ptr<Cache>>
    {
        // ex. fullCacheFileName = "/var/lib/applovin/datamover/test.1647455391370.cache"
        const auto fullCacheFileName = mCacheOneTimeLoader.getLatestTimestampFullCacheFileName( cacheName() );
        return loadWithName( fullCacheFileName, isPreloadMemoryEnabled );
    }

    [[nodiscard]] auto cacheName() const -> const std::string &
    {
        return mCacheName;
    }
    auto type() const -> CacheType
    {
        return mType;
    }

    auto getTimestamp() const -> std::string
    {
        return mCacheOneTimeLoader.getTimestamp();
    }

  protected:
    [[nodiscard]] auto settings() const -> const SharedSettingsProvider *
    {
        return mSettings;
    }

    auto loadWithName( const std::string & fullCacheFileName, bool isPreloadMemoryEnabled = true ) -> std::pair<bool, std::shared_ptr<Cache>>
    {
        auto lastModifiedAt = std::filesystem::last_write_time( fullCacheFileName );
        if ( lastModifiedAt > mLastCacheFileModificationTime )
        {
            std::ostringstream oss;
            AL_LOG_INFO( "reloading " + fullCacheFileName );
            mLastCacheFileModificationTime = lastModifiedAt;
            auto retval = mCacheOneTimeLoader.loadAbsolutePath<Cache>( cacheName(), fullCacheFileName, isPreloadMemoryEnabled );
            if ( retval != nullptr )
            {
                mType = retval->type();
                return { true, std::move( retval ) };
            }

            AL_LOG_ERROR( "failed to load axoncache " + fullCacheFileName );
        }
        else
        {
            AL_LOG_INFO( "not reloading " + fullCacheFileName );
        }

        return { false, nullptr };
    }

  private:
    const std::string mCacheName;
    const SharedSettingsProvider * mSettings{ nullptr };
    CacheOneTimeLoader mCacheOneTimeLoader;
    CacheType mType{ CacheType::NONE };
    std::chrono::milliseconds mReloadAttemptIntervalMs{ std::chrono::milliseconds::zero() };

    std::filesystem::file_time_type mLastCacheFileModificationTime;
};
}
