// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/logger/Logger.h"
#include "axoncache/cache/value/ChainedValue.h"
#include "axoncache/Constants.h"
#include "axoncache/memory/MemoryHandler.h"
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>

using namespace axoncache;

auto ChainedValue::calculateSize( std::string_view key, std::string_view value ) -> uint64_t
{
    return sizeof( uint8_t * )          // for the next ptr
           + 2                          // for the key length
           + 1                          // for the type
           + 3                          // for the value length
           + key.size() + value.size(); // for the key & value
}

auto ChainedValue::add( int64_t keySpaceOffset, std::string_view key, [[maybe_unused]] uint64_t hashcode, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint32_t
{
    uint32_t collisions = 0;
    auto valueOffset = keySpaceOffset;
    auto * dataPtr = memory->data();
    auto * valuePtrOffset = ( uint64_t * )( dataPtr + valueOffset );

    // Find end of chain
    while ( *valuePtrOffset != 0UL )
    {
        valueOffset = *valuePtrOffset;
        valuePtrOffset = ( uint64_t * )( dataPtr + valueOffset );
        collisions++;
    }

    // Add new value to end of dataspace
    auto newValueOffset = addToEnd( key, type, value, memory );

    // Recalc increase memory was reallocated
    dataPtr = memory->data();
    valuePtrOffset = ( uint64_t * )( dataPtr + valueOffset );

    *valuePtrOffset = newValueOffset;

    return collisions;
}

auto ChainedValue::get( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key, [[maybe_unused]] uint8_t type, bool * isExist ) const -> std::string_view
{
    *isExist = false;
#ifdef DEBUG
    if ( keySpaceOffset < 0 )
    {
        AL_LOG_ERROR( "Keyspace offset not set " + std::to_string( keySpaceOffset ) );
        throw std::runtime_error( "Keyspace offset not set" );
    }
#endif

    uint64_t valueOffset = *( ( uint64_t * )( uint8_t * )( dataSpace + keySpaceOffset ) );
    // Find end of chain
    while ( valueOffset != 0UL )
    {
        const uint8_t * const valueSpacePtr = dataSpace + valueOffset;

        auto foundKeySize = ( uint64_t )( ( ( uint16_t * )( valueSpacePtr + sizeof( uint64_t ) ) )[0] );

        if ( foundKeySize == key.size() )
        {
            const uint8_t * keyPtr = ( ( const uint8_t * )valueSpacePtr ) + sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint32_t );
            if ( std::memcmp( keyPtr, key.data(), key.size() ) == 0UL )
            {
                auto valueSizeAndType = ( ( uint32_t * )( valueSpacePtr + sizeof( uint64_t ) + sizeof( uint16_t ) ) )[0];

                auto valueType = ( uint8_t )( valueSizeAndType >> 24 );
                if ( valueType != type )
                {
                    std::ostringstream oss;
                    oss << "Type mismatch for key " << key
                        << " expected " << type
                        << " type in cache was " << valueType;
                    AL_LOG_ERROR( oss.str() );

                    return {};
                }

                *isExist = true;
                auto valueSize = valueSizeAndType & 0xFFFFFFU;
                return { ( const char * )keyPtr + key.size(), valueSize };
            }
        }
        valueOffset = ( ( uint64_t * )valueSpacePtr )[0];
    }

    return {};
}

auto ChainedValue::get( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key, [[maybe_unused]] uint8_t type, [[maybe_unused]] const std::vector<std::string_view> & frequentValues ) const -> std::string_view
{
    bool isExist = false;
    return get( dataSpace, keySpaceOffset, key, type, &isExist );
}

auto ChainedValue::getWithType( const uint8_t * /* dataSpace */, int64_t /* keySpaceOffset */, [[maybe_unused]] const std::vector<std::string_view> & /* frequentValues */ ) const -> std::pair<std::string_view, CacheValueType>
{
    return {};
}

auto ChainedValue::contains( const uint8_t * dataSpace, int64_t keySpaceOffset, std::string_view key ) const -> bool
{
#ifdef DEBUG
    if ( keySpaceOffset < 0 )
    {
        AL_LOG_ERROR( "Keyspace offset not set" );
        throw std::runtime_error( "Keyspace offset not set" );
    }
#endif

    // TODO(dpayne): ideally this code is merged with the get() code above
    uint64_t valueOffset = *( ( uint64_t * )( uint8_t * )( dataSpace + keySpaceOffset ) );
    // Find end of chain
    while ( valueOffset != 0UL )
    {
        const uint8_t * const valueSpacePtr = dataSpace + valueOffset;

        auto foundKeySize = ( uint64_t )( ( ( uint16_t * )( valueSpacePtr + sizeof( uint64_t ) ) )[0] );

        if ( foundKeySize == key.size() )
        {
            const uint8_t * keyPtr = ( ( const uint8_t * )valueSpacePtr ) + sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint32_t );
            if ( std::memcmp( keyPtr, key.data(), key.size() ) == 0UL )
            {
                return true;
            }
        }
        valueOffset = ( ( uint64_t * )valueSpacePtr )[0];
    }

    return false;
}

auto ChainedValue::addToEnd( std::string_view key, uint8_t type, std::string_view value, MemoryHandler * memory ) -> uint64_t
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
        oss << "input value size " << key.size()
            << " is too large. max=" << Constants::Limit::kKeyLength;
        AL_LOG_ERROR( oss.str() );

        throw std::runtime_error( "value size " + std::to_string( value.size() ) + " too large. max=" + std::to_string( Constants::Limit::kValueLength ) );
    }

    auto newSize = calculateSize( key, value );
    auto * valueSpace = memory->grow( newSize );

    ( ( uint64_t * )( valueSpace + 0UL ) )[0] = 0UL; // zero out the next ptr
    ( ( uint16_t * )( valueSpace + sizeof( uint64_t ) ) )[0] = key.size();
    ( ( uint32_t * )( valueSpace + sizeof( uint64_t ) + sizeof( uint16_t ) ) )[0] = ( ( ( uint32_t )type ) << 24 ) | ( 0xFFFFFFU & value.size() );
    std::memcpy( ( void * )( valueSpace + sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint32_t ) ), key.data(), key.size() );
    std::memcpy( ( void * )( valueSpace + sizeof( uint64_t ) + sizeof( uint16_t ) + sizeof( uint32_t ) + key.size() ), value.data(), value.size() );

    return valueSpace - memory->data();
}
