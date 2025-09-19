// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/writer/CacheFileWriter.h"
#include <filesystem>
#include <utility>
#include "axoncache/Constants.h"
#include "axoncache/cache/CacheBase.h"
#include "axoncache/writer/detail/GenerateHeader.h"
#include "axoncache/logger/Logger.h"

using namespace axoncache;

CacheFileWriter::CacheFileWriter( std::string outputDirectory, std::string cacheName, CacheBase * cache ) :
    CacheWriter( cache ), mOutputDirectory( std::move( outputDirectory ) ), mCacheName( std::move( cacheName ) ), mFullCacheFileName()
{
    mFullCacheFileName.reserve( 64 );
    mFullCacheFileName.append( mOutputDirectory )
        .append( std::string{ std::filesystem::path::preferred_separator } )
        .append( mCacheName )
        .append( std::string{ Constants::kCacheFileNameSuffix } );
    mOutput = std::fstream( mFullCacheFileName, std::ios::out | std::ios::binary );
}

CacheFileWriter::~CacheFileWriter()
{
    mOutput.close();
}

auto CacheFileWriter::startWrite() -> void
{
    GenerateHeader generator;
    AL_LOG_INFO( "Writing " + mFullCacheFileName );
    generator.write( cache(), cacheName(), mOutput );
}

auto CacheFileWriter::writeData() -> void
{
    cache()->output( mOutput );
}

auto CacheFileWriter::endWrite() -> void
{
    mOutput.flush();
    AL_LOG_INFO( "Flushed " + mFullCacheFileName );
}
