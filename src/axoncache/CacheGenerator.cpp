// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/CacheGenerator.h"
#include <filesystem>
#include <memory>
#include <utility>
#include <sstream>
#include <string_view>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/Constants.h"
#include "axoncache/builder/CacheFileBuilder.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/cache/factory/CacheFactory.h"
#include "axoncache/logger/Logger.h"

using namespace axoncache;

CacheGenerator::CacheGenerator( const SharedSettingsProvider * settings ) :
    mSettings( settings )
{
    auto splitStr = []( char delimiter, const std::string & s )
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream( s );
        while ( std::getline( tokenStream, token, delimiter ) )
        {
            tokens.push_back( token );
        }
        return tokens;
    };

    auto cacheNames = splitStr( ',', settings->getString( Constants::ConfKey::kCacheNames.data(), Constants::ConfDefault::kCacheNames.data() ) );

    for ( const auto & cacheName : cacheNames )
    {
        CacheArgs args;

        args.cacheType = static_cast<CacheType>( settings->getInt( std::string{ Constants::ConfKey::kCacheType } + "." + cacheName, Constants::ConfDefault::kCacheType ) );

        if ( args.cacheType == CacheType::BUCKET_CHAIN )
        {
            args.offsetBits = settings->getInt( std::string{ Constants::ConfKey::kOffsetBits } + "." + cacheName, Constants::ConfDefault::kBucketChainOffsetBits );
        }
        else if ( args.cacheType == CacheType::LINEAR_PROBE )
        {
            args.offsetBits = settings->getInt( std::string{ Constants::ConfKey::kOffsetBits } + "." + cacheName, Constants::ConfDefault::kLinearProbeOffsetBits );
        }
        else
        {
            args.offsetBits = settings->getInt( std::string{ Constants::ConfKey::kOffsetBits } + "." + cacheName, 0 );
        }

        args.numberOfKeySlots = settings->getInt( std::string{ Constants::ConfKey::kKeySlots } + "." + cacheName, Constants::ConfDefault::kKeySlots );
        args.maxLoadFactor = settings->getDouble( std::string{ Constants::ConfKey::kMaxLoadFactor.data() } + "." + cacheName, Constants::ConfDefault::kMaxLoadFactor );

        args.cacheName = cacheName;
        args.outputDirectory = settings->getString( std::string{ Constants::ConfKey::kOutputDir } + "." + cacheName, Constants::ConfDefault::kOutputDir.data() );

        auto inputDir = settings->getString( std::string{ Constants::ConfKey::kInputDir } + "." + cacheName, Constants::ConfDefault::kInputDir.data() );
        const auto inputFiles = settings->getString( std::string{ Constants::ConfKey::kInputFiles } + "." + cacheName, "" );

        for ( const auto & inputFile : splitStr( ',', inputFiles ) )
        {
            inputDir.push_back( std::filesystem::path::preferred_separator );
            inputDir.append( inputFile );
            args.inputFiles.push_back( inputDir );
        }

        mCacheArgs.emplace_back( std::move( args ) );
    }
}

auto CacheGenerator::start( const std::vector<std::string> & values ) const -> void
{
    for ( const auto & cacheArg : mCacheArgs )
    {
        std::ostringstream oss;
        oss << "Creating cache " << cacheArg.cacheName
            << " with " << cacheArg.numberOfKeySlots
            << " key slots with type " << static_cast<uint32_t>( cacheArg.cacheType );

        AL_LOG_INFO( oss.str() );

        auto cache = CacheFactory::createCache( cacheArg.offsetBits, cacheArg.numberOfKeySlots, cacheArg.maxLoadFactor, cacheArg.cacheType );
        if ( !values.empty() && ( cacheArg.cacheType == CacheType::LINEAR_PROBE_DEDUP || cacheArg.cacheType == CacheType::LINEAR_PROBE_DEDUP_TYPED ) )
        {
            ( ( LinearProbeDedupCache * )cache.get() )->setDuplicatedValues( values );
        }
        CacheFileBuilder cacheFileBuilder( settings(), cacheArg.outputDirectory, cacheArg.cacheName, cacheArg.inputFiles, std::move( cache ) );
        cacheFileBuilder.build();
    }
}
