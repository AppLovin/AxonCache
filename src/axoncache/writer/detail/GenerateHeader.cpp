// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/writer/detail/GenerateHeader.h"
#include <algorithm>
#include <chrono>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include "axoncache/Constants.h"
#include "axoncache/cache/CacheBase.h"
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

auto GenerateHeader::write( const CacheBase * cache, std::string_view cacheName, std::ostream & output ) const -> void
{
    /*
     * Layout (bytes):
     * - magicNumber      (2)
     * - headerSize       (2)
     * - nameStart        (2)
     * - version          (2)
     *
     * - cacheType        (2)
     * - hashcodeBits     (2)
     * - offsetBits       (2)
     * - hashFuncId       (2)
     *
     * - reserved         (4)
     * - maxCollisions    (4)
     *
     * - maxLoafFactor    (8)
     * - creationTimeMs   (8)
     * - numberOfKeySlots (8)
     * - numberOfEntries  (8)
     * - dataSize         (8)
     * - size             (8)
     * - cacheName        (32)
     */
    auto headerSize = sizeof( CacheHeader );
    CacheHeader info;
    info.magicNumber = Constants::kCacheHeaderMagicNumber;
    info.headerSize = headerSize;
    info.nameStart = offsetof( CacheHeader, cacheName );
    info.version = cache->version();

    info.cacheType = static_cast<uint16_t>( cache->type() );
    info.hashcodeBits = cache->hashcodeBits();
    info.offsetBits = cache->offsetBits();
    info.hashFuncId = cache->hashFuncId();

    info.reserved = 0U;
    info.maxCollisions = cache->maxCollisions();

    info.maxLoadFactor = cache->maxLoadFactor();
    info.creationTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
    info.numberOfKeySlots = cache->numberOfKeySlots();
    info.numberOfEntries = cache->numberOfEntries();
    info.dataSize = cache->dataSize();
    info.size = cache->size();
    snprintf( info.cacheName, std::min( cacheName.size() + 1, Constants::kMaxCacheNameSize ), "%s", cacheName.data() );

    output.write( reinterpret_cast<const char *>( &info ), sizeof( CacheHeader ) );
}

auto GenerateHeader::read( std::istream & input ) const -> std::pair<std::string, CacheHeader>
{
    CacheHeader info;
    input.read( reinterpret_cast<char *>( &info ), sizeof( CacheHeader ) );

    if ( static_cast<uint64_t>( input.gcount() ) < sizeof( CacheHeader ) )
    {
        throw std::runtime_error( "malformed cache" );
    }

    // malformed cache may have a not null-terminated cacheName
    std::string cacheName;

    // to make the CacheHeader flexible to add more fields, we make info.cacheName the last item
    // if we don't change definition of CacheHeader, infoNameStart == info.cacheName
    if ( ( info.headerSize - info.nameStart ) != Constants::kMaxCacheNameSize )
    {
        throw std::runtime_error( "malformed cache, header size " + std::to_string( info.headerSize ) + ", name start " + std::to_string( info.nameStart ) );
    }
    char * infoNameStart = reinterpret_cast<char *>( &info ) + info.nameStart;

    for ( uint32_t i = 0; i < Constants::kMaxCacheNameSize; i++ )
    {
        if ( *infoNameStart == '\0' )
        {
            break;
        }
        cacheName.push_back( *infoNameStart );
        infoNameStart++;
    }

    return { cacheName, info };
}
