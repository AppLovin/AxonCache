// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <fstream>
#include "axoncache/writer/CacheWriter.h"
namespace axoncache
{
class CacheBase;
}

namespace axoncache
{
class CacheFileWriter : public CacheWriter
{
  public:
    CacheFileWriter( std::string outputDirectory, std::string cacheName, CacheBase * cache );

    CacheFileWriter( const CacheFileWriter & ) = delete;
    CacheFileWriter( CacheFileWriter & ) = delete;
    CacheFileWriter( CacheFileWriter && ) = delete;
    auto operator=( const CacheFileWriter & ) -> CacheFileWriter & = delete;
    auto operator=( CacheFileWriter & ) -> CacheFileWriter & = delete;
    auto operator=( CacheFileWriter && ) -> CacheFileWriter & = delete;

    ~CacheFileWriter() override;

  protected:
    auto startWrite() -> void override;
    auto writeData() -> void override;
    auto endWrite() -> void override;

  private:
    auto cacheName() const -> const std::string &
    {
        return mCacheName;
    }

    auto outputDirectory() const -> const std::string &
    {
        return mOutputDirectory;
    }

    std::string mOutputDirectory;

    std::string mCacheName;

    std::string mFullCacheFileName;

    std::fstream mOutput;
};
}
