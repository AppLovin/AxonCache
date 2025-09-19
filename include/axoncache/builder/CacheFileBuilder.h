// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <axoncache/common/SharedSettingsProvider.h>
#include <string>
#include <memory>
#include <utility>
#include <stdexcept>
#include <vector>
#include "axoncache/Constants.h"
#include "axoncache/builder/CacheBuilder.h"
#include "axoncache/cache/CacheBase.h"
#include "axoncache/consumer/CacheValueConsumer.h"
#include "axoncache/parser/CacheValueParser.h"
#include "axoncache/reader/DataFileReader.h"
#include "axoncache/reader/DataReader.h"
#include "axoncache/writer/CacheFileWriter.h"
#include "axoncache/writer/CacheWriter.h"
namespace axoncache
{
class CacheValueConsumerBase;
}

namespace axoncache
{
class CacheFileBuilder : public CacheBuilder
{
  public:
    CacheFileBuilder( const SharedSettingsProvider * settings, std::string outputDirectory, std::string cacheName, std::vector<std::string> fileNames, std::unique_ptr<CacheBase> cache ) :
        CacheBuilder( settings, std::move( cache ) ),
        mCacheName( std::move( cacheName ) ), mOutputDirectory( std::move( outputDirectory ) ), mFileNames( std::move( fileNames ) ),
        mReader( nullptr ),
        mConsumer( this->cache() )
    {
        if ( mFileNames.empty() )
        {
            throw std::runtime_error( "No input datafile" );
        }

        mReader = std::make_unique<DataFileReader>( mFileNames, std::make_unique<CacheValueParser>( settings ), settings->getChar( Constants::ConfKey::kControlCharLine, Constants::ConfDefault::kControlCharLine ) );
    }

    virtual ~CacheFileBuilder() = default;

    [[nodiscard]] auto reader() -> DataReader * override
    {
        return mReader.get();
    }

    [[nodiscard]] auto consumer() -> CacheValueConsumerBase * override
    {
        return &mConsumer;
    }

    [[nodiscard]] auto createWriter() -> std::unique_ptr<CacheWriter> override
    {
        return std::make_unique<CacheFileWriter>( outputDirectory(), cacheName(), cache() );
    }

  protected:
    [[nodiscard]] auto outputDirectory() const -> std::string
    {
        return mOutputDirectory;
    }

    [[nodiscard]] auto cacheName() const -> std::string
    {
        return mCacheName;
    }

  private:
    std::string mCacheName;
    std::string mOutputDirectory;
    std::vector<std::string> mFileNames;

    std::unique_ptr<DataReader> mReader;
    CacheValueConsumer mConsumer;
};

};
