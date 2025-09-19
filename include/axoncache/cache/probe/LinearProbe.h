// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/Constants.h"
#include <string_view>
#include <cstring>

namespace axoncache
{
namespace linear
{

struct __attribute__( ( __packed__ ) ) LinearProbeRecord
{
    uint16_t keySize;
    uint32_t dedupIndex : 5; // Note: dedupIndex is left most 5 bits, type is right most 3 bits of 3rd byte
    uint32_t type : 3;       // ex. String, StringList,
    uint32_t valSize : 24;
    char data[];
};

static_assert( sizeof( LinearProbeRecord ) == 6, "sizeof( LinearProbeRecord ) != 6" );
[[maybe_unused]] constexpr uint8_t kDedupFlag = 1 << 4;
[[maybe_unused]] constexpr uint8_t kDedupExtendedFlag = 1;
}

template<uint32_t KeyWidth>
class LinearProbe
{
  public:
    LinearProbe( uint16_t offsetBits, uint64_t numberOfKeySlots ) :
        mLog2OfKeyWidth( 3U ),
        mHashcodeBits( 64U - offsetBits ),
        mOffsetBits( offsetBits ),
        mNumberOfKeySlots( numberOfKeySlots ),
        mKeyspaceSizeOffset( numberOfKeySlots * KeyWidth - 8 ),
        mHashcodeMask( 0UL ),
        mOffsetMask( 0UL )
    {
        static_assert( KeyWidth == 8, "Only support 8-byte KeyWidth" );

        if ( ( offsetBits < Constants::kMinLinearProbeOffsetBits ) || ( Constants::kMaxLinearProbeOffsetBits < offsetBits ) )
        {
            throw std::runtime_error( "offset bits must in range of [ " + std::to_string( Constants::kMinLinearProbeOffsetBits ) +
                                      ", " + std::to_string( Constants::kMaxLinearProbeOffsetBits ) + " ]" );
        }

        uint64_t maxUint64 = ~0UL;
        mHashcodeMask = ( maxUint64 >> mOffsetBits ) << mOffsetBits;
        mOffsetMask = ( 1UL << mOffsetBits ) - 1;
    }

    [[nodiscard]] auto log2OfKeyWidth() const -> uint16_t
    {
        return mLog2OfKeyWidth;
    }

    [[nodiscard]] auto cacheType() const -> axoncache::CacheType
    {
        return CacheType::LINEAR_PROBE;
    }

    [[nodiscard]] auto hashcodeBits() const -> uint16_t
    {
        return mHashcodeBits;
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
        return mHashcodeMask;
    }

    [[nodiscard]] auto offsetMask() const -> uint64_t
    {
        return mOffsetMask;
    }

    [[nodiscard]] auto keySlotToPtrOffset( uint64_t keySlot ) const -> uint64_t
    {
        return ( keySlot << mLog2OfKeyWidth ); // same as: keySlot * KeyWidth;
    }

    [[nodiscard]] auto calculateKeySpaceSize() const -> uint64_t
    {
        return keySlotToPtrOffset( mNumberOfKeySlots );
    }

    // for read. If found, return the offset to the record. Otherwise, AXONCACHE_KEY_NOT_FOUND
    [[nodiscard]] auto findKeySlotOffset( std::string_view key, uint64_t hashcode, const uint8_t * keySpacePtr ) const -> int64_t
    {
        auto cmpHashcode = ( hashcode & mHashcodeMask );
        auto slotId = ( hashcode % mNumberOfKeySlots );
        while ( true )
        {
            auto slot = *( reinterpret_cast<const uint64_t *>( keySpacePtr ) + slotId );
            auto slotOffset = ( slot & mOffsetMask );
            if ( slotOffset == 0UL )
            {
                return Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND;
            }

            if ( ( slot & mHashcodeMask ) == cmpHashcode )
            {
                const auto * record = reinterpret_cast<const linear::LinearProbeRecord *>( keySpacePtr + slotOffset + mKeyspaceSizeOffset );
                if ( record->keySize == key.size() && std::memcmp( ( const void * )record->data, key.data(), key.size() ) == 0 )
                {
                    return keySlotToPtrOffset( slotId );
                }
            }

            slotId = ( slotId + 1 ) % mNumberOfKeySlots;
        }
    }

    // find an empty slot for write. return AXONCACHE_KEY_EXIST if key already exists in cache
    [[nodiscard]] auto findFreeKeySlotOffset( std::string_view key, uint64_t hashcode, const uint8_t * keySpacePtr, uint32_t & collisions ) -> int64_t
    {
        auto cmpHashcode = ( hashcode & mHashcodeMask );
        auto slotId = ( hashcode % mNumberOfKeySlots );
        collisions = 0;
        while ( true )
        {
            auto slot = *( reinterpret_cast<const uint64_t *>( keySpacePtr ) + slotId );
            auto slotOffset = ( slot & mOffsetMask );
            if ( slotOffset == 0UL )
            {
                return keySlotToPtrOffset( slotId ); // we found an empty slot
            }

            collisions++;

            if ( ( slot & mHashcodeMask ) == cmpHashcode )
            {
                const auto * record = reinterpret_cast<const linear::LinearProbeRecord *>( keySpacePtr + slotOffset + mKeyspaceSizeOffset );
                if ( record->keySize == key.size() && std::memcmp( ( const void * )record->data, key.data(), key.size() ) == 0 )
                {
                    return Constants::ProbeStatus::AXONCACHE_KEY_EXISTS;
                }
            }

            slotId = ( slotId + 1 ) % mNumberOfKeySlots;
        }
    }

  private:
    uint16_t mLog2OfKeyWidth;
    uint16_t mHashcodeBits;
    uint16_t mOffsetBits;

    uint64_t mNumberOfKeySlots;
    uint64_t mKeyspaceSizeOffset;

    uint64_t mHashcodeMask;
    uint64_t mOffsetMask;
};
} // namespace axoncache
