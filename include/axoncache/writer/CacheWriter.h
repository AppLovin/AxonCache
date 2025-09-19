// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

namespace axoncache
{
class CacheBase;
}

namespace axoncache
{
class CacheWriter
{
  public:
    CacheWriter( CacheBase * cache ) :
        mCache( cache )
    {
    }

    virtual ~CacheWriter() = default;

    CacheWriter( const CacheWriter & ) = delete;
    CacheWriter( CacheWriter & ) = delete;
    CacheWriter( CacheWriter && ) = delete;
    auto operator=( const CacheWriter & ) -> CacheWriter & = delete;
    auto operator=( CacheWriter & ) -> CacheWriter & = delete;
    auto operator=( CacheWriter && ) -> CacheWriter & = delete;

    virtual auto write() -> void
    {
        startWrite();
        writeData();
        endWrite();
    }

  protected:
    virtual auto startWrite() -> void = 0;

    virtual auto writeData() -> void = 0;

    virtual auto endWrite() -> void = 0;

    auto cache() -> CacheBase *
    {
        return mCache;
    }

  private:
    CacheBase * mCache;
};
}
