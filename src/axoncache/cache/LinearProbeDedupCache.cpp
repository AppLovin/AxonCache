// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <sstream>
#include "axoncache/logger/Logger.h"
#include "axoncache/cache/LinearProbeDedupCache.h"

using namespace axoncache;

auto LinearProbeDedupCache::output( std::ostream & output ) const -> void
{
    std::ostringstream oss;
    oss << "values loaded from file "
        << ( mIsValuesLoaded ? "true" : "false" )
        << ", set values "
        << ( mValuesMemoryHandler != nullptr ? "true" : "false" );

    AL_LOG_INFO( oss.str() );

    if ( mCacheType == CacheType::LINEAR_PROBE || mIsValuesLoaded )
    {
        // cache is loaded from file with values, just write as is
        output.write( ( const char * )memoryHandler()->data(), memoryHandler()->dataSize() );
        return;
    }

    [[maybe_unused]] uint64_t wroteSize = 0UL;
    if ( mValuesMemoryHandler != nullptr )
    {
        // values set by an API, we don't need to count, replace data
        output.write( ( const char * )memoryHandler()->data(), memoryHandler()->dataSize() );
        wroteSize = memoryHandler()->dataSize();
        wroteSize += frequentValuesOutput( mValues, mValuesMemoryHandler.get(), output );
    }
    else
    {
        output.write( ( const char * )memoryHandler()->data(), memoryHandler()->dataSize() );
        wroteSize = memoryHandler()->dataSize();
        uint64_t valueOffset = 0UL;
        output.write( ( char * )&valueOffset, sizeof( uint64_t ) );
        wroteSize += sizeof( uint64_t );
    }
}

auto LinearProbeDedupCache::putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t>
{
    if ( this->mHeader.numberOfEntries >= this->mMaxNumberOfEntries ) // linear requires more empty slots for efficient get operations
    {
        std::ostringstream oss;
        oss << "keySpace is full, numOfEntries=" << this->mHeader.numberOfEntries
            << " numberOfKeySlots=" << this->numberOfKeySlots()
            << " maxLoadFactor=" << mHeader.maxLoadFactor;

        AL_LOG_ERROR( oss.str() );
        throw std::runtime_error( "keySpace is full" );
    }

    uint32_t collisions = 0;
    auto hashcode = Xxh3Hasher::hash( key );
    auto keySlotOffset = this->mProbe.findFreeKeySlotOffset( key, hashcode, this->mKeySpacePtr, collisions );
    if ( keySlotOffset != Constants::ProbeStatus::AXONCACHE_KEY_EXISTS )
    {
        int index = -1;
        {
            const auto & iter = mValuesToIndex.find( value.size() );
            if ( iter != mValuesToIndex.end() )
            {
                const auto & subIter = iter->second.find( value );
                if ( subIter != iter->second.end() )
                {
                    index = subIter->second;
                }
            }
        }
        if ( index == -1 )
        {
            collisions = std::max( collisions, this->mValueMgr.add( keySlotOffset, key, hashcode, static_cast<uint8_t>( type ), value, this->mutableMemoryHandler() ) );
        }
        else
        {
            collisions = std::max( collisions, this->mValueMgr.add( keySlotOffset, key, hashcode, static_cast<uint8_t>( type ), static_cast<uint32_t>( value.size() ), static_cast<uint16_t>( index ), this->mutableMemoryHandler() ) );
        }
        this->mHeader.maxCollisions = std::max( collisions, this->mHeader.maxCollisions );
        ++( this->mHeader.numberOfEntries );
        // Data ptr could have changed, so update the pointer
        this->updateKeySpacePtr();
        return std::make_pair( true, collisions );
    }

    return std::make_pair( false, collisions );
}

auto LinearProbeDedupCache::frequentValuesOutput( const std::vector<std::string_view> & valuesInOrder, MallocMemoryHandler * handler, std::ostream & output ) const -> uint64_t
{
    AL_LOG_INFO( "Write frequent value data" );
    const auto topValueCount = static_cast<uint16_t>( valuesInOrder.size() );
    uint64_t wroteSize = 0UL;

    if ( topValueCount == 0U )
    {
        uint64_t valueOffset = 0UL;
        output.write( ( char * )&valueOffset, sizeof( uint64_t ) );
        wroteSize += sizeof( uint64_t );
        return wroteSize;
    }

    output.write( ( const char * )&topValueCount, sizeof( uint16_t ) );
    wroteSize += sizeof( uint16_t );
    std::vector<uint32_t> topValuesLength;
    topValuesLength.resize( topValueCount );
    uint64_t checkSize = 0UL;
    size_t index = 0UL;
    for ( const auto & value : valuesInOrder )
    {
        topValuesLength[index++] = static_cast<uint32_t>( value.size() );
        checkSize += value.size();
    }

    output.write( ( char * )topValuesLength.data(), topValueCount * sizeof( uint32_t ) );
    wroteSize += topValueCount * sizeof( uint32_t );
    output.write( ( char * )handler->data(), handler->dataSize() );
    wroteSize += handler->dataSize();
    if ( checkSize != handler->dataSize() )
    {
        throw std::runtime_error( "frequent values total size doesn't match" );
    }

    uint64_t valueOffset = sizeof( uint16_t ) + topValueCount * sizeof( uint32_t ) + handler->dataSize() + sizeof( uint64_t );
    output.write( ( char * )&valueOffset, sizeof( uint64_t ) );
    wroteSize += sizeof( uint64_t );
    return wroteSize;
}

auto LinearProbeDedupCache::setFrequentValue() -> void
{
    mIsValuesLoaded = true;
    auto dataPtr = memoryHandler()->data();
    auto valueOffset = *( uint64_t * )( dataPtr + memoryHandler()->dataSize() - sizeof( uint64_t ) );
    if ( valueOffset == 0UL )
    {
        return;
    }

    auto currentOffset = memoryHandler()->dataSize() - valueOffset;
    const auto frequentValuesCount = *reinterpret_cast<const uint16_t *>( dataPtr + currentOffset );

    currentOffset += sizeof( uint16_t );
    mValues.reserve( frequentValuesCount );
    std::vector<uint32_t> freqValuesLength;
    freqValuesLength.resize( frequentValuesCount );
    std::memcpy( ( void * )freqValuesLength.data(), ( void * )( dataPtr + currentOffset ), frequentValuesCount * sizeof( uint32_t ) );
    currentOffset += frequentValuesCount * sizeof( uint32_t );
    uint8_t index = 0U;
    for ( const auto & length : freqValuesLength )
    {
        mValues.emplace_back( ( const char * )( dataPtr + currentOffset ), length );
        auto & valuesToIndex = mValuesToIndex[length];
        valuesToIndex.emplace( std::string_view{ ( const char * )( dataPtr + currentOffset ), length }, index++ );
        currentOffset += length;
    }
    currentOffset += sizeof( uint64_t );

    std::ostringstream oss;
    oss << "Number of frequent values = " << mValues.size()
        << " last offset = " << currentOffset
        << " data size = " << memoryHandler()->dataSize();
    AL_LOG_INFO( oss.str() );

    if ( currentOffset != memoryHandler()->dataSize() )
    {
        throw std::runtime_error( "data size doesn't match" );
    }
}
