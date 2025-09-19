// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <memory>
#include <stdint.h>
namespace axoncache
{
class CacheBase;
}
namespace axoncache
{
enum class CacheType;
}

namespace axoncache
{
class CacheFactory
{
  public:
    static auto createCache( uint16_t offsetBits, uint64_t numberOfKeySlots, double maxLoadFactor, CacheType type ) -> std::unique_ptr<CacheBase>;
};
}
