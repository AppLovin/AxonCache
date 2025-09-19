// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include "axoncache/Constants.h"
#include <cstdint>
#include <ostream>
#include <vector>
#include <utility>

namespace axoncache
{

// for performance purpose, try to make the CacheHeader 8-byte aligned, i.e. the keyspacePtr will be 8-byte aligned too
struct CacheHeader_s
{
    uint16_t magicNumber;
    uint16_t headerSize;
    uint16_t nameStart;
    uint16_t version;

    uint16_t cacheType;
    uint16_t hashcodeBits; // for linear probe
    uint16_t offsetBits;   // for linear probe
    uint16_t hashFuncId;

    uint32_t reserved;
    uint32_t maxCollisions;

    double maxLoadFactor;
    uint64_t creationTimeMs;
    uint64_t numberOfKeySlots;
    uint64_t numberOfEntries;
    uint64_t dataSize;
    uint64_t size; // total size of cache file = header + keySpaceSize + dataSize

    // add more uint64_t here if needed. and will be backward compatible

    // this must be the last item
    char cacheName[Constants::kMaxCacheNameSize]; // 32. so the header & keySpacePtr are aligned to 8-bytes boundary
};

using CacheHeader = CacheHeader_s;

}

[[maybe_unused]] static auto operator<<( std::ostream & os, const axoncache::CacheHeader & header ) -> std::ostream &
{
    os << "{"
       << R"("magic_number")"
       << ":" << header.magicNumber << ","
       << R"("header_size")"
       << ":" << header.headerSize << ","
       << R"("name_start")"
       << ":" << header.nameStart << ","
       << R"("version")"
       << ":" << header.version << ","
       << R"("cache_type")"
       << ":" << header.cacheType << ","
       << R"("hashcode_bits")"
       << ":" << header.hashcodeBits << ","
       << R"("offset_bits")"
       << ":" << header.offsetBits << ","
       << R"("hash_func_id")"
       << ":" << header.hashFuncId << ","
       << R"("max_collisions")"
       << ":" << header.maxCollisions << ","
       << R"("max_load_factor")"
       << ":" << header.maxLoadFactor << ","
       << R"("creation_time_ms")"
       << ":" << header.creationTimeMs << ","
       << R"("number_of_key_slots")"
       << ":" << header.numberOfKeySlots << ","
       << R"("number_of_entries")"
       << ":" << header.numberOfEntries << ","
       << R"("data_size")"
       << ":" << header.dataSize << ","
       << R"("size")"
       << ":" << header.size << ","
       << R"("cache_name")"
       << ":\"" << header.cacheName << "\"}";
    return os;
}

[[maybe_unused]] static auto toHeaderInfo( const axoncache::CacheHeader & header ) -> std::vector<std::pair<std::string, std::string>>
{
    std::vector<std::pair<std::string, std::string>> vec;

    vec.push_back( { "magic_number", std::to_string( header.magicNumber ) } );
    vec.push_back( { "header_size", std::to_string( header.headerSize ) } );
    vec.push_back( { "name_start", std::to_string( header.nameStart ) } );
    vec.push_back( { "version", std::to_string( header.version ) } );

    vec.push_back( { "cache_type", std::to_string( header.cacheType ) } );
    vec.push_back( { "hashcode_bits", std::to_string( header.hashcodeBits ) } );
    vec.push_back( { "offset_bits", std::to_string( header.offsetBits ) } );
    vec.push_back( { "hash_func_id", std::to_string( header.hashFuncId ) } );

    vec.push_back( { "max_collisions", std::to_string( header.maxCollisions ) } );

    vec.push_back( { "max_load_factor", std::to_string( header.maxLoadFactor ) } );
    vec.push_back( { "creation_time_ms", std::to_string( header.creationTimeMs ) } );
    vec.push_back( { "number_of_key_slots", std::to_string( header.numberOfKeySlots ) } );
    vec.push_back( { "number_of_entries", std::to_string( header.numberOfEntries ) } );
    vec.push_back( { "data_size", std::to_string( header.dataSize ) } );
    vec.push_back( { "size", std::to_string( header.size ) } );

    vec.push_back( { "cache_name", header.cacheName } );

    return vec;
}
