// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/logger/Logger.h"
#include "axoncache/cache/value/LinearProbeValue.h"
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <sstream>
#include "axoncache/Constants.h"
#include "axoncache/cache/probe/LinearProbe.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

auto LinearProbeValue::calculateSize( std::string_view key, std::string_view value ) -> uint64_t
{
    return sizeof( linear::LinearProbeRecord ) + key.size() + value.size();
}

auto LinearProbeValue::add( int64_t keySpaceOffset, std::string_view key, uint64_t hashcode, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint32_t
{
    // Add new value to end of dataspace
    auto newValueOffset = addToEnd( key, type, value, memory ) - mKeyspaceSizeOffset;
    auto slotOffset = ( newValueOffset & mOffsetMask );

    if ( slotOffset != newValueOffset )
    {
        throw std::runtime_error( "offset bits " + mOffsetBitsStr + " too short" );
    }

    auto * dataPtr = memory->data();
    auto * slot = reinterpret_cast<uint64_t *>( dataPtr + keySpaceOffset );

    uint64_t slotValue = ( hashcode & mHashcodeMask ) | slotOffset;
    *slot = slotValue;

    return 0;
}

auto LinearProbeValue::get( const uint8_t * dataSpace, int64_t keySpaceOffset, [[maybe_unused]] std::string_view key, uint8_t type, [[maybe_unused]] bool * isExist ) const -> std::string_view
{
    if ( keySpaceOffset == Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND )
    {
        return {};
    }

    const auto slot = *( reinterpret_cast<const uint64_t *>( dataSpace + keySpaceOffset ) );
    uint64_t slotOffset = ( slot & mOffsetMask ) + mKeyspaceSizeOffset;
    const auto * record = reinterpret_cast<const linear::LinearProbeRecord *>( dataSpace + static_cast<uint64_t>( slotOffset ) );
    const auto * dataPtr = reinterpret_cast<const char *>( dataSpace + static_cast<uint64_t>( slotOffset ) + sizeof( const linear::LinearProbeRecord ) );

    if ( static_cast<uint8_t>( record->type ) != type )
    {
        std::ostringstream oss;
        oss << "Type mismatch for key " << record->data
            << " expected " << type
            << " type in cache was " << record->type;
        AL_LOG_ERROR( oss.str() );
        return {};
    }

    return { dataPtr + record->keySize, record->valSize };
}

auto LinearProbeValue::get( const uint8_t * dataSpace, int64_t keySpaceOffset, [[maybe_unused]] std::string_view key, uint8_t type, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::string_view
{
    if ( keySpaceOffset == Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND )
    {
        return {};
    }

    const auto slot = *( reinterpret_cast<const uint64_t *>( dataSpace + keySpaceOffset ) );
    uint64_t slotOffset = ( slot & mOffsetMask ) + mKeyspaceSizeOffset;
    const auto * record = reinterpret_cast<const linear::LinearProbeRecord *>( dataSpace + static_cast<uint64_t>( slotOffset ) );
    const auto * dataPtr = reinterpret_cast<const char *>( dataSpace + static_cast<uint64_t>( slotOffset ) + sizeof( const linear::LinearProbeRecord ) );

    if ( static_cast<uint8_t>( record->type ) != type )
    {
        std::ostringstream oss;
        oss << "Type mismatch for key " << record->data
            << " expected " << type
            << " type in cache was " << record->type;
        return {};
    }

    if ( ( record->dedupIndex & linear::kDedupFlag ) && !frequentValues.empty() )
    {
        return frequentValues[*( uint8_t * )( dataPtr + record->keySize )];
    }
    else if ( ( record->dedupIndex & linear::kDedupExtendedFlag ) && !frequentValues.empty() )
    {
        return frequentValues[*( uint16_t * )( dataPtr + record->keySize )];
    }
    return { dataPtr + record->keySize, record->valSize };
}

auto LinearProbeValue::getWithType( const uint8_t * dataSpace, int64_t keySpaceOffset, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::pair<std::string_view, CacheValueType>
{
    if ( keySpaceOffset == Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND )
    {
        return {};
    }

    const auto slot = *( reinterpret_cast<const uint64_t *>( dataSpace + keySpaceOffset ) );
    uint64_t slotOffset = ( slot & mOffsetMask ) + mKeyspaceSizeOffset;
    const auto * record = reinterpret_cast<const linear::LinearProbeRecord *>( dataSpace + static_cast<uint64_t>( slotOffset ) );
    const auto * dataPtr = reinterpret_cast<const char *>( dataSpace + static_cast<uint64_t>( slotOffset ) + sizeof( const linear::LinearProbeRecord ) );

    if ( ( record->dedupIndex & linear::kDedupFlag ) && !frequentValues.empty() )
    {
        return std::make_pair( frequentValues[*( uint8_t * )( dataPtr + record->keySize )], static_cast<CacheValueType>( record->type ) );
    }
    else if ( ( record->dedupIndex & linear::kDedupExtendedFlag ) && !frequentValues.empty() )
    {
        return std::make_pair( frequentValues[*( uint16_t * )( dataPtr + record->keySize )], static_cast<CacheValueType>( record->type ) );
    }
    return std::make_pair( std::string_view{ dataPtr + record->keySize, record->valSize }, static_cast<CacheValueType>( record->type ) );
}

auto LinearProbeValue::contains( [[maybe_unused]] const uint8_t * dataSpace, int64_t keySpaceOffset, [[maybe_unused]] std::string_view key ) const -> bool
{
    return keySpaceOffset != Constants::ProbeStatus::AXONCACHE_KEY_NOT_FOUND;
}

auto LinearProbeValue::addToEnd( std::string_view key, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint64_t
{
    if ( key.size() > Constants::Limit::kKeyLength )
    {
        std::ostringstream oss;
        oss << "input key size " << key.size()
            << " is too large. max=" << Constants::Limit::kKeyLength;
        AL_LOG_ERROR( oss.str() );

        throw std::runtime_error( "key size " + std::to_string( key.size() ) + " too large. max=" + std::to_string( Constants::Limit::kKeyLength ) );
    }

    if ( value.size() > Constants::Limit::kValueLength )
    {
        std::ostringstream oss;
        oss << "input value size " << value.size()
            << " is too large. max=" << Constants::Limit::kValueLength;
        AL_LOG_ERROR( oss.str() );

        throw std::runtime_error( "value size " + std::to_string( value.size() ) + " too large. max=" + std::to_string( Constants::Limit::kValueLength ) );
    }

    auto newSize = calculateSize( key, value );
    auto * valueSpace = memory->grow( newSize );
    auto * record = reinterpret_cast<linear::LinearProbeRecord *>( valueSpace );
    auto * dataPtr = valueSpace + sizeof( const linear::LinearProbeRecord );

    record->keySize = key.size();
    record->dedupIndex = 0;
    record->type = type;
    record->valSize = value.size();
    std::memcpy( dataPtr, key.data(), key.size() );
    std::memcpy( dataPtr + key.size(), value.data(), value.size() );

    return valueSpace - memory->data();
}

auto LinearProbeValue::calculateSize( std::string_view key, uint16_t index ) -> uint64_t
{
    return sizeof( linear::LinearProbeRecord ) + key.size() + ( index < 256U ? 1U : 2U );
}

auto LinearProbeValue::addToEnd( std::string_view key, uint8_t type, uint32_t valueSize, uint16_t index, MemoryHandler * memory ) -> uint64_t
{
    if ( key.size() > Constants::Limit::kKeyLength )
    {
        std::ostringstream oss;
        oss << "input key size " << key.size()
            << " is too large. max=" << Constants::Limit::kKeyLength;

        AL_LOG_ERROR( oss.str() );
        throw std::runtime_error( "key size " + std::to_string( key.size() ) + " too large. max=" + std::to_string( Constants::Limit::kKeyLength ) );
    }

    auto newSize = calculateSize( key, index );
    auto * valueSpace = memory->grow( newSize );
    auto * record = reinterpret_cast<linear::LinearProbeRecord *>( valueSpace );
    auto * dataPtr = valueSpace + sizeof( const linear::LinearProbeRecord );

    record->keySize = key.size();
    record->type = type;
    record->dedupIndex = linear::kDedupFlag;
    record->valSize = valueSize;
    std::memcpy( dataPtr, key.data(), key.size() );
    if ( index < 256U )
    {
        auto shortIndex = static_cast<uint8_t>( index );
        std::memcpy( dataPtr + key.size(), ( void * )&shortIndex, 1U );
    }
    else
    {
        record->dedupIndex = linear::kDedupExtendedFlag;
        std::memcpy( dataPtr + key.size(), ( void * )&index, 2U );
    }
    return valueSpace - memory->data();
}

auto LinearProbeValue::add( int64_t keySpaceOffset, std::string_view key, uint64_t hashcode, uint8_t type, uint32_t valueSize, uint16_t index, MemoryHandler * memory ) -> uint32_t
{
    // Add new value to end of dataspace
    auto newValueOffset = addToEnd( key, type, valueSize, index, memory ) - mKeyspaceSizeOffset;
    auto slotOffset = ( newValueOffset & mOffsetMask );

    if ( slotOffset != newValueOffset )
    {
        throw std::runtime_error( "offset bits " + mOffsetBitsStr + " too short" );
    }

    auto * dataPtr = memory->data();
    auto * slot = reinterpret_cast<uint64_t *>( dataPtr + keySpaceOffset );

    uint64_t slotValue = ( hashcode & mHashcodeMask ) | slotOffset;
    *slot = slotValue;

    return 0;
}
