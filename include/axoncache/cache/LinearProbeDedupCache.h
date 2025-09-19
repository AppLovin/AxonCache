// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/cache/LinearProbeCache.h"
#include "axoncache/cache/probe/LinearProbe.h"
#include "axoncache/memory/MallocMemoryHandler.h"
#include <map>
#include <unordered_map>

namespace axoncache
{
class LinearProbeDedupCache : public LinearProbeCache
{
  public:
    LinearProbeDedupCache( uint16_t offsetBits, uint64_t numberOfKeySlots, double maxLoadFactor, std::unique_ptr<MemoryHandler> memoryHandler, CacheType cacheType ) :
        LinearProbeCache( offsetBits, numberOfKeySlots, maxLoadFactor, std::move( memoryHandler ) ), mCacheType( cacheType )
    {
    }

    LinearProbeDedupCache( const CacheHeader & header, std::unique_ptr<MemoryHandler> memoryHandler ) :
        LinearProbeCache( header, std::move( memoryHandler ) ), mCacheType( static_cast<CacheType>( header.cacheType ) )
    {
        if ( mCacheType != CacheType::LINEAR_PROBE )
        {
            setFrequentValue();
        }
    }

    auto output( std::ostream & output ) const -> void override;

    [[nodiscard]] auto type() const -> CacheType override
    {
        return mCacheType;
    }

    auto setDuplicatedValues( const std::vector<std::string> & values ) -> void
    {
        if ( values.size() > 65536 )
        {
            throw std::runtime_error( "Should not set more than 65536 duplicated values" );
        }
        if ( mIsValuesLoaded )
        {
            throw std::runtime_error( "Values already loaded from memory" );
        }
        if ( mValuesMemoryHandler != nullptr )
        {
            throw std::runtime_error( "Values already set, call this API only once" );
        }

        mValuesMemoryHandler = std::make_unique<MallocMemoryHandler>();
        for ( const auto & value : values )
        {
            auto * ptr = mValuesMemoryHandler->grow( value.size() );
            std::memcpy( ( void * )ptr, ( void * )value.data(), value.size() );
        }

        mValues.clear();
        mValuesToIndex.clear();
        mValues.reserve( values.size() );

        uint16_t index = 0U;
        auto ptr = ( char * )mValuesMemoryHandler->data();
        for ( const auto & value : values )
        {
            mValues.emplace_back( ptr, value.size() );
            auto & valuesToIndex = mValuesToIndex[value.size()];
            valuesToIndex.emplace( std::string_view{ ptr, value.size() }, index++ );
            ptr = ptr + value.size();
        }
    }

    auto getDuplicatedValues() const -> std::vector<std::string>
    {
        std::vector<std::string> duplicatedValues;
        duplicatedValues.reserve( mValues.size() );
        for ( const auto & value : mValues )
        {
            duplicatedValues.emplace_back( value );
        }
        return duplicatedValues;
    }

  protected:
    [[nodiscard]] auto getInternal( std::string_view key, CacheValueType type ) const -> std::string_view override
    {
        auto hash = Xxh3Hasher::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        return mValueMgr.get( mKeySpacePtr, keySlotOffset, key, static_cast<uint8_t>( type ), mValues );
    }

    [[nodiscard]] auto getInternal( std::string_view key, CacheValueType type, bool * isExists ) const -> std::string_view override
    {
        auto hash = Xxh3Hasher::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        *isExists = ( keySlotOffset != Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND );
        return mValueMgr.get( mKeySpacePtr, keySlotOffset, key, static_cast<uint8_t>( type ), mValues );
    }

    [[nodiscard]] auto getWithTypeInternal( std::string_view key ) const -> std::pair<std::string_view, CacheValueType> override
    {
        auto hash = Xxh3Hasher::hash( key );
        auto keySlotOffset = mProbe.findKeySlotOffset( key, hash, mKeySpacePtr );
        return mValueMgr.getWithType( mKeySpacePtr, keySlotOffset, mValues );
    }

    auto putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t> override;

    auto frequentValuesOutput( const std::vector<std::string_view> & valuesInOrder, MallocMemoryHandler * handler, std::ostream & output ) const -> uint64_t;

    auto setFrequentValue() -> void;

    std::vector<std::string_view> mValues;
    std::map<size_t, std::unordered_map<std::string_view, uint16_t>> mValuesToIndex;
    std::unique_ptr<MallocMemoryHandler> mValuesMemoryHandler{ nullptr };
    bool mIsValuesLoaded{ false };
    CacheType mCacheType{ CacheType::LINEAR_PROBE };
};
}
