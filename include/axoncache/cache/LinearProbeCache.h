// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/cache/base/HashedCacheBase.h"
#include "axoncache/cache/probe/LinearProbe.h"
#include "axoncache/cache/value/LinearProbeValue.h"
#include "axoncache/cache/hasher/Xxh3Hasher.h"

namespace axoncache
{
using LinearProbeCache = HashedCacheBase<Xxh3Hasher, LinearProbe<sizeof( uint64_t )>, LinearProbeValue, CacheType::LINEAR_PROBE>;
}
