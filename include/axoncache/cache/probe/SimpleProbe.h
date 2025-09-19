// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string_view>
#include <axoncache/cache/CacheType.h>
#include <cstdint>
#include "axoncache/Math.h"

namespace axoncache
{
template<uint32_t KeyWidth>
class SimpleProbe
{
  public:
    SimpleProbe( [[maybe_unused]] uint16_t offsetBits, uint64_t numberOfKeySlots ) :
        mLog2OfKeyWidth( __builtin_clz( KeyWidth ) ^ 31 ),
        mHashcodeBits( 0U ), // not used
        mOffsetBits( 64U ),  // not used
        mNumberOfKeySlots( math::roundUpToPowerOfTwo( numberOfKeySlots ) ), mKeySpaceSizeMask( mNumberOfKeySlots - 1 )
    {
        static_assert( ( KeyWidth & ( KeyWidth - 1 ) ) == 0, "KeyWidth must be power of 2" );

        // +----------+-----------------+
        // | KeyWidth | mLog2OfKeyWidth |
        // +----------+-----------------+
        // |    1     |        0        |
        // |    2     |        1        |
        // |    4     |        2        |
        // |    8     |        3        |
        // ...
    }

    [[nodiscard]] auto log2OfKeyWidth() const -> uint16_t
    {
        return mLog2OfKeyWidth;
    }

    [[nodiscard]] auto cacheType() const -> axoncache::CacheType
    {
        return CacheType::BUCKET_CHAIN;
    }

    [[nodiscard]] auto hashcodeBits() const -> uint16_t
    {
        return 0; // we don't keep hashcode in KeySpace
    }

    [[nodiscard]] auto offsetBits() const -> uint16_t
    {
        return mOffsetBits;
    }

    [[nodiscard]] auto numberOfKeySlots() const -> uint64_t
    {
        return mNumberOfKeySlots;
    }

    [[nodiscard]] auto keyspaceSize() const -> uint64_t
    {
        return mNumberOfKeySlots * KeyWidth;
    }

    [[nodiscard]] auto hashcodeMask() const -> uint64_t
    {
        return 0; // not used
    }

    [[nodiscard]] auto offsetMask() const -> uint64_t
    {
        return 0; // not used
    }

    [[nodiscard]] auto keySlotToPtrOffset( uint64_t keySlot ) const -> uint64_t
    {
        return ( keySlot << mLog2OfKeyWidth ); // same as: keySlot * KeyWidth;
    }

    [[nodiscard]] auto calculateKeySpaceSize() const -> uint64_t
    {
        return keySlotToPtrOffset( mNumberOfKeySlots );
    }

    // for read
    [[nodiscard]] auto findKeySlotOffset( [[maybe_unused]] std::string_view key, uint64_t hashcode, [[maybe_unused]] const uint8_t * keySpacePtr ) const -> int64_t
    {
        return keySlotToPtrOffset( hashcode & mKeySpaceSizeMask );
    }

    // for write
    [[nodiscard]] auto findFreeKeySlotOffset( [[maybe_unused]] std::string_view key, uint64_t hashcode, uint8_t * keySpacePtr, uint32_t & collisions ) -> int64_t
    {
        collisions = 0;
        return findKeySlotOffset( key, hashcode, keySpacePtr );
    }

  private:
    uint16_t mLog2OfKeyWidth;

    // SimpleProbe doesn't use mHashcodeBits or mOffsetBits
    uint16_t mHashcodeBits;
    uint16_t mOffsetBits;

    uint64_t mNumberOfKeySlots;
    uint64_t mKeySpaceSizeMask;
};
} // namespace axoncache
