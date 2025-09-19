// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <cstdint>
#include <iosfwd>
#include <stdexcept>
#include <vector>
#include <span>

#include "axoncache/Constants.h"
#include "axoncache/cache/CacheBase.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheHeader.h"
#include "axoncache/domain/CacheValue.h"
#include "axoncache/transformer/StringListToString.h"
#include "axoncache/transformer/StringViewToNullTerminatedString.h"
#include "axoncache/transformer/TypeToString.h"

namespace axoncache
{
class MemoryHandler;
}

namespace axoncache
{

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
class HashedCacheBase : public CacheBase
{
  public:
    HashedCacheBase( uint16_t offsetBits, uint64_t numberOfKeySlots, double maxLoadFactor, std::unique_ptr<MemoryHandler> memoryHandler ) :
        CacheBase( std::move( memoryHandler ) ),
        mMaxNumberOfEntries( numberOfKeySlots * maxLoadFactor ),
        mKeySpacePtr( nullptr ),
        mHeader(),
        mProbe( offsetBits, numberOfKeySlots ),
        mValueMgr( offsetBits, numberOfKeySlots, mProbe.hashcodeMask(), mProbe.offsetMask() )
    {
        if ( mProbe.cacheType() == CacheType::LINEAR_PROBE && maxLoadFactor > Constants::ConfDefault::kLinearProbeMaxLoadFactor )
        {
            throw std::runtime_error( "LoadFactor for LINEAR_PROBE can't greater than " + std::to_string( Constants::ConfDefault::kLinearProbeMaxLoadFactor ) );
        }
        mHeader.numberOfKeySlots = numberOfKeySlots;
        mHeader.maxLoadFactor = maxLoadFactor;
        mutableMemoryHandler()->allocate( mProbe.calculateKeySpaceSize() );
        updateKeySpacePtr();
    }

    HashedCacheBase( const CacheHeader & header, std::unique_ptr<MemoryHandler> memoryHandler ) :
        CacheBase( std::move( memoryHandler ) ),
        mMaxNumberOfEntries( 0 ),
        mKeySpacePtr( nullptr ),
        mHeader( header ),
        mProbe( header.offsetBits, header.numberOfKeySlots ),
        mValueMgr( header.offsetBits, header.numberOfKeySlots, mProbe.hashcodeMask(), mProbe.offsetMask() )
    {
        updateKeySpacePtr();
    }

    HashedCacheBase( const HashedCacheBase & ) = delete;
    HashedCacheBase( HashedCacheBase & ) = delete;
    HashedCacheBase( HashedCacheBase && ) = delete;
    auto operator=( const HashedCacheBase & ) -> HashedCacheBase & = delete;
    auto operator=( HashedCacheBase & ) -> HashedCacheBase & = delete;
    auto operator=( HashedCacheBase && ) -> HashedCacheBase & = delete;

    ~HashedCacheBase() override = default;

    auto updateKeySpacePtr() -> void
    {
        mKeySpacePtr = mutableMemoryHandler()->data();
    }

    auto getKeySpacePtr() -> uint8_t *
    {
        return mutableMemoryHandler()->data();
    }

    [[nodiscard]] auto type() const -> CacheType override
    {
        return CacheTypeVal;
    }

    [[nodiscard]] auto hashcodeBits() const -> uint16_t override
    {
        return mProbe.hashcodeBits();
    }

    [[nodiscard]] auto offsetBits() const -> uint16_t override
    {
        return mProbe.offsetBits();
    }

    [[nodiscard]] auto hashFuncId() const -> uint16_t override
    {
        return HashAlgo::hashFuncId();
    }

    [[nodiscard]] auto maxLoadFactor() const -> double override
    {
        return this->mHeader.maxLoadFactor;
    }

    [[nodiscard]] auto maxCollisions() const -> uint32_t override
    {
        return this->mHeader.maxCollisions;
    }

    [[nodiscard]] auto creationTimeMs() const -> uint64_t override
    {
        return this->mHeader.creationTimeMs;
    }

    [[nodiscard]] auto numberOfEntries() const -> uint64_t override
    {
        return this->mHeader.numberOfEntries;
    }

    [[nodiscard]] auto maxNumberEntries() const -> uint64_t override
    {
        return mMaxNumberOfEntries; // not useful for bucket chained
    }

    [[nodiscard]] auto numberOfKeySlots() const -> uint64_t override
    {
        return mProbe.numberOfKeySlots();
    }

    [[nodiscard]] auto dataSize() const -> uint64_t override
    {
        return memoryHandler()->dataSize() - mProbe.keyspaceSize();
    }

    [[nodiscard]] auto size() const -> uint64_t override
    {
        return memoryHandler()->dataSize() + sizeof( CacheHeader );
    }

    [[nodiscard]] auto headerInfo() const -> std::vector<std::pair<std::string, std::string>> override
    {
        return toHeaderInfo( mHeader );
    }

    auto output( std::ostream & output ) const -> void override
    {
        output.write( ( const char * )memoryHandler()->data(), memoryHandler()->dataSize() );
    }

    auto put( std::string_view key, std::string_view value ) -> std::pair<bool, uint32_t> override
    {
        const auto str = StringViewToNullTerminatedString::transform( value );
        return putInternal( key, CacheValueType::String, std::string_view{ str.data(), str.size() } );
    }

    auto put( std::string_view key, const std::vector<std::string_view> & value ) -> std::pair<bool, uint32_t> override
    {
        const auto str = StringListToString::transform( value );
        return putInternal( key, CacheValueType::StringList, std::string_view{ str.data(), str.size() } );
    }

    auto put( std::string_view key, bool & value ) -> std::pair<bool, uint32_t> override
    {
        const auto str = transform<bool>( value );
        return putInternal( key, CacheValueType::Bool, std::string_view{ str.data(), str.size() } );
    }

    auto put( std::string_view key, int64_t & value ) -> std::pair<bool, uint32_t> override
    {
        const auto str = transform<int64_t>( value );
        return putInternal( key, CacheValueType::Int64, std::string_view{ str.data(), str.size() } );
    }

    auto put( std::string_view key, double & value ) -> std::pair<bool, uint32_t> override
    {
        const auto str = transform<double>( value );
        return putInternal( key, CacheValueType::Double, std::string_view{ str.data(), str.size() } );
    }

    auto put( std::string_view key, const std::vector<float> & value ) -> std::pair<bool, uint32_t> override
    {
        const auto str = transform<std::vector<float>>( value );
        return putInternal( key, CacheValueType::FloatList, std::string_view{ str.data(), str.size() } );
    }

    [[nodiscard]] auto get( std::string_view key, std::string_view defaultValue = {} ) const -> std::string_view
    {
        auto str = getInternal( key, CacheValueType::String );
        if ( !str.empty() )
        {
            str = StringViewToNullTerminatedString::trimExtraNullTerminator( str );
        }
        return str.empty() ? defaultValue : str;
    }

    [[nodiscard]] auto getVector( std::string_view key, const std::vector<std::string_view> & defaultValue = {} ) const -> std::vector<std::string_view>
    {
        const auto str = getInternal( key, CacheValueType::StringList );
        const auto retValue = str.empty() ? defaultValue : StringListToString::transform( str );
        return retValue.empty() ? defaultValue : retValue;
    }

    [[nodiscard]] auto getString( std::string_view key, std::string_view defaultValue = {} ) const -> std::pair<std::string_view, bool>;

    [[nodiscard]] auto getBool( std::string_view key, bool defaultValue = false ) const -> std::pair<bool, bool>;

    [[nodiscard]] auto getInt64( std::string_view key, int64_t defaultValue = 0 ) const -> std::pair<int64_t, bool>;

    [[nodiscard]] auto getDouble( std::string_view key, double defaultValue = 0 ) const -> std::pair<double, bool>;

    [[nodiscard]] auto getWithType( std::string_view key ) const -> std::pair<std::string_view, CacheValueType>
    {
        const auto valueAndType = getWithTypeInternal( key );
        switch ( valueAndType.second )
        {
            case CacheValueType::String:
                return std::make_pair( valueAndType.first.empty() ? valueAndType.first : StringViewToNullTerminatedString::trimExtraNullTerminator( valueAndType.first ), valueAndType.second );
            case CacheValueType::StringList:
                return std::make_pair( std::string_view{}, valueAndType.second ); // Not supported
            default:
                break;
        }
        return valueAndType;
    }

    [[nodiscard]] auto getFloatVector( std::string_view key ) const -> std::vector<float>;
    [[nodiscard]] auto getFloatAtIndices( std::string_view key, const std::vector<int32_t> & indices ) const -> std::vector<float>;
    [[nodiscard]] auto getFloatAtIndex( std::string_view key, int32_t index ) const -> float;

    [[nodiscard]] auto getFloatSpan( std::string_view key ) const -> std::span<const float>;

    [[nodiscard]] auto getKeyType( std::string_view key ) const -> std::string;

    [[nodiscard]] auto contains( std::string_view key ) const -> bool
    {
        auto hash = HashAlgo::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        return mValueMgr.contains( mKeySpacePtr, keySlotOffset, key );
    }

    // C-Cache migration methods
    auto readKey( std::string_view key ) -> std::string_view;
    auto readKeys( std::string_view key ) -> std::vector<std::string_view>;

  protected:
    virtual auto putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t>;

    [[nodiscard]] virtual auto getInternal( std::string_view key, CacheValueType type ) const -> std::string_view
    {
        auto hash = HashAlgo::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        bool isExist = false;
        return mValueMgr.get( mKeySpacePtr, keySlotOffset, key, static_cast<uint8_t>( type ), &isExist );
    }

    [[nodiscard]] virtual auto getInternal( std::string_view key, CacheValueType type, bool * isExist ) const -> std::string_view
    {
        auto hash = HashAlgo::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        *isExist = ( keySlotOffset != Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND );
        return mValueMgr.get( mKeySpacePtr, keySlotOffset, key, static_cast<uint8_t>( type ), isExist );
    }

    [[nodiscard]] virtual auto getWithTypeInternal( std::string_view key ) const -> std::pair<std::string_view, CacheValueType>
    {
        auto hash = HashAlgo::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        return mValueMgr.getWithType( mKeySpacePtr, keySlotOffset, {} );
    }

    uint64_t mMaxNumberOfEntries;
    uint8_t * mKeySpacePtr;
    CacheHeader mHeader;
    Probe mProbe;
    ValueMgr mValueMgr;
};

} // axoncache
