// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <memory>
#include <utility>
#include "axoncache/cache/CacheBase.h"
#include "axoncache/reader/DataReader.h"
#include "axoncache/writer/CacheWriter.h"
namespace axoncache
{
class SharedSettingsProvider;
}
namespace axoncache
{
class CacheValueConsumerBase;
}

namespace axoncache
{

class CacheBuilder
{
  public:
    CacheBuilder( const SharedSettingsProvider * settings, std::unique_ptr<CacheBase> cache ) :
        mSettings{ settings }, mCache( std::move( cache ) )
    {
    }

    virtual auto build() -> void
    {
        reader()->readValues( consumer() );
        save();
    }

    [[nodiscard]] virtual auto cache() -> CacheBase *
    {
        return mCache.get();
    }

  protected:
    virtual auto save() -> void
    {
        auto writer = createWriter();
        writer->write();
    }

    [[nodiscard]] virtual auto settings() -> const SharedSettingsProvider *
    {
        return mSettings;
    }

    [[nodiscard]] virtual auto reader() -> DataReader * = 0;
    [[nodiscard]] virtual auto consumer() -> CacheValueConsumerBase * = 0;
    [[nodiscard]] virtual auto createWriter() -> std::unique_ptr<CacheWriter> = 0;

  private:
    const SharedSettingsProvider * mSettings;
    std::unique_ptr<CacheBase> mCache;
};

};
