// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/cache/probe/SimpleProbe.h"
#include "axoncache/cache/value/ChainedValue.h"
#include "axoncache/cache/base/HashedCacheBase.h"
#include "axoncache/cache/hasher/Xxh3Hasher.h"

namespace axoncache
{
using BucketChainCache = HashedCacheBase<Xxh3Hasher, SimpleProbe<sizeof( uint64_t )>, ChainedValue, CacheType::BUCKET_CHAIN>;
}
