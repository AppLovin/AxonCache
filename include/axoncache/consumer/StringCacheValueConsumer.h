// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include "axoncache/cache/CacheBase.h"
#include "axoncache/consumer/CacheValueConsumerBase.h"
#include "axoncache/domain/CacheValue.h"

namespace axoncache
{
class StringCacheValueConsumer : public CacheValueConsumerBase
{
  public:
    StringCacheValueConsumer() = default;
    StringCacheValueConsumer( StringCacheValueConsumer && ) = delete;
    StringCacheValueConsumer( StringCacheValueConsumer & ) = delete;
    StringCacheValueConsumer( const StringCacheValueConsumer & ) = delete;
    auto operator=( const StringCacheValueConsumer & ) -> StringCacheValueConsumer & = delete;
    auto operator=( StringCacheValueConsumer & ) -> StringCacheValueConsumer & = delete;
    auto operator=( StringCacheValueConsumer && ) -> StringCacheValueConsumer & = delete;
    ~StringCacheValueConsumer() override = default;

    auto consumeValue( CacheKeyValue keyValuePair ) -> PutStats override;

    [[nodiscard]] auto output() const -> const std::string &
    {
        return mOutput;
    }

  private:
    std::string mOutput;
};

} // namespace axoncache
