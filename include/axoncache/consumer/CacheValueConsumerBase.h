// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/cache/CacheBase.h"
#include "axoncache/domain/CacheValue.h"

namespace axoncache
{
class CacheValueConsumerBase
{
  public:
    CacheValueConsumerBase() = default;
    CacheValueConsumerBase( CacheValueConsumerBase && ) = delete;
    CacheValueConsumerBase( CacheValueConsumerBase & ) = delete;
    CacheValueConsumerBase( const CacheValueConsumerBase & ) = delete;
    auto operator=( const CacheValueConsumerBase & ) -> CacheValueConsumerBase & = delete;
    auto operator=( CacheValueConsumerBase & ) -> CacheValueConsumerBase & = delete;
    auto operator=( CacheValueConsumerBase && ) -> CacheValueConsumerBase & = delete;
    virtual ~CacheValueConsumerBase() = default;
    virtual auto consumeValue( CacheKeyValue keyValuePair ) -> PutStats = 0;
};

} // namespace axoncache
