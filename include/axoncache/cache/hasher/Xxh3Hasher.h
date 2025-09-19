// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string_view>
#include <cstdint>

namespace axoncache
{
class Xxh3Hasher
{
  public:
    static auto hash( std::string_view val ) -> uint64_t;

    static auto hashFuncId() -> uint16_t;
};
} // namespace axoncache
