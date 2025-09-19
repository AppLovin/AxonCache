// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/cache/hasher/Xxh3Hasher.h"
#include <axoncache/Constants.h>
#include <axoncache/common/xxh3.h>
#include <string_view>

using namespace axoncache;

auto Xxh3Hasher::hash( std::string_view val ) -> uint64_t
{
    return XXH3_64bits( val.data(), val.size() );
}

auto Xxh3Hasher::hashFuncId() -> uint16_t
{
    return Constants::HashFuncId::XXH3;
}
