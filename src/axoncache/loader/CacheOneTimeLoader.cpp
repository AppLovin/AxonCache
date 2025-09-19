// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/loader/CacheOneTimeLoader.h"
#include <fstream>
#include <string_view>
#include <sstream>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/Constants.h"
#include "axoncache/writer/detail/GenerateHeader.h"

using namespace axoncache;

auto CacheOneTimeLoader::loadHeader( const std::string & cacheFile ) const -> std::pair<std::string, CacheHeader>
{
    std::ifstream file( cacheFile, std::ios::binary );
    if ( !file.is_open() )
    {
        throw std::runtime_error( "Could not open axoncache : " + cacheFile );
    }

    GenerateHeader generateHeader;
    auto retVal = generateHeader.read( file );
    file.close();

    return retVal;
}

auto CacheOneTimeLoader::getFullCacheFileName( const std::string & cacheName ) const -> std::string
{
    return settings()->getString( Constants::ConfKey::kLoadDir + "." + cacheName, Constants::ConfDefault::kLoadDir.data() ) + "/" + cacheName + std::string{ Constants::kCacheFileNameSuffix };
}

// ex. open /var/lib/applovin/datamover/test_cache.cache.timestamp.latest
// and return "/var/lib/applovin/datamover/test_cache.1651622570800.cache"
auto CacheOneTimeLoader::getLatestTimestampFullCacheFileName( const std::string & cacheName ) const -> std::string
{
    const auto & loadDir = settings()->getString( Constants::ConfKey::kLoadDir + "." + cacheName, Constants::ConfDefault::kLoadDir.data() );
    std::string latestTimestampFile;
    latestTimestampFile.reserve( 64 );
    latestTimestampFile.append( loadDir )
        .append( "/" )
        .append( cacheName )
        .append( Constants::kCacheFileNameSuffix )
        .append( Constants::kLatestTimestampFileNameSuffix );

    std::ifstream timestampFile( latestTimestampFile );
    std::string latestTimestamp;
    if ( timestampFile )
    {
        std::ostringstream oss;
        oss << timestampFile.rdbuf();
        latestTimestamp = oss.str();
        timestampFile.close();
    }
    else
    {
        throw std::runtime_error( "can't open " + latestTimestampFile );
    }

    if ( latestTimestamp.empty() )
    {
        throw std::runtime_error( latestTimestampFile + " is empty" );
    }

    return loadDir + "/" + cacheName + "." + latestTimestamp + std::string{ Constants::kCacheFileNameSuffix };
}

auto CacheOneTimeLoader::getTimestampFullCacheFileName( const std::string & cacheName, const int64_t timestamp ) const -> std::string
{
    const auto & loadDir = settings()->getString( Constants::ConfKey::kLoadDir + "." + cacheName, Constants::ConfDefault::kLoadDir.data() );
    return loadDir + "/" + cacheName + "." + std::to_string( timestamp ) + std::string{ Constants::kCacheFileNameSuffix };
}

auto CacheOneTimeLoader::getTimestamp() const -> std::string
{
    return mTimestamp;
}
