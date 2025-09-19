// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string_view>
#include <utility>
#include <stdint.h>
#include <vector>
#include "axoncache/domain/CacheValue.h"
namespace axoncache
{
class MemoryHandler;
}

namespace axoncache
{
class ChainedValue
{
  public:
    ChainedValue( [[maybe_unused]] uint16_t offsetBits, [[maybe_unused]] uint64_t keyspaceSize, [[maybe_unused]] uint64_t hashcodeMask, [[maybe_unused]] uint64_t offsetMask )
    {
    }

    // return the collisions
    auto add( int64_t keySpaceOffset, std::string_view key, [[maybe_unused]] uint64_t hashcode, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint32_t;

    // return empty string if not found
    auto get( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key, uint8_t type, bool * isExist ) const -> std::string_view;

    auto get( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key, uint8_t type, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::string_view;

    auto getWithType( const uint8_t * dataSpace, int64_t keySpaceOffset, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::pair<std::string_view, CacheValueType>;

    auto contains( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key ) const -> bool;

  protected:
    auto addToEnd( std::string_view key, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint64_t;

    auto calculateSize( std::string_view key, std::string_view value ) -> uint64_t;
};
} // namespace axoncache
