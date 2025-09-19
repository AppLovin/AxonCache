// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/cache/CacheBase.h"
#include "axoncache/consumer/CacheValueConsumerBase.h"
#include "axoncache/domain/CacheValue.h"

namespace axoncache
{
class CacheValueConsumer : public CacheValueConsumerBase
{
  public:
    CacheValueConsumer( CacheBase * cache );
    CacheValueConsumer( CacheValueConsumer && ) = delete;
    CacheValueConsumer( CacheValueConsumer & ) = delete;
    CacheValueConsumer( const CacheValueConsumer & ) = delete;
    auto operator=( const CacheValueConsumer & ) -> CacheValueConsumer & = delete;
    auto operator=( CacheValueConsumer & ) -> CacheValueConsumer & = delete;
    auto operator=( CacheValueConsumer && ) -> CacheValueConsumer & = delete;
    ~CacheValueConsumer() override = default;

    auto consumeValue( CacheKeyValue keyValuePair ) -> PutStats override;

  private:
    [[nodiscard]] auto cache() const -> CacheBase *
    {
        return mCache;
    }

    CacheBase * mCache;
};

} // namespace axoncache
