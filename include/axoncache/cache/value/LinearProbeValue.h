// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <cstdint>
#include <vector>
#include "axoncache/domain/CacheValue.h"
namespace axoncache
{
class MemoryHandler;
}

namespace axoncache
{
class LinearProbeValue
{
  public:
    LinearProbeValue( uint16_t offsetBits, uint64_t numberOfKeySlots, uint64_t hashcodeMask, uint64_t offsetMask ) :
        mKeyspaceSizeOffset( numberOfKeySlots * 8 - 8 ),
        mHashcodeMask( hashcodeMask ),
        mOffsetMask( offsetMask ),
        mOffsetBitsStr( std::to_string( offsetBits ) )
    {
    }

    // return the collisions in DataSpace
    // For linear probe, collisions happen in KeySpace, so this function always returns 0
    auto add( int64_t keySpaceOffset, std::string_view key, uint64_t hashcode, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint32_t;
    auto add( int64_t keySpaceOffset, std::string_view key, uint64_t hashcode, uint8_t type, uint32_t valueSize, uint16_t index, MemoryHandler * memory ) -> uint32_t;

    // return empty string if not found
    auto get( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key, uint8_t type, bool * isExist ) const -> std::string_view;

    auto get( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key, uint8_t type, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::string_view;

    auto getWithType( const uint8_t * dataSpace, int64_t keySpaceOffset, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::pair<std::string_view, CacheValueType>;

    auto contains( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key ) const -> bool;

  protected:
    auto addToEnd( std::string_view key, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint64_t;
    auto addToEnd( std::string_view key, uint8_t type, uint32_t valueSize, uint16_t index, MemoryHandler * memory ) -> uint64_t;

    auto calculateSize( std::string_view key, std::string_view value ) -> uint64_t;
    auto calculateSize( std::string_view key, uint16_t index ) -> uint64_t;

    uint64_t mKeyspaceSizeOffset;

    uint64_t mHashcodeMask;

    uint64_t mOffsetMask;

    std::string mOffsetBitsStr;
};
} // namespace axoncache
