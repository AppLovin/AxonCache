// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string_view>
#include <string>
#include <vector>
#include <ostream>
#include <memory>
#include <axoncache/version.h>
#include "axoncache/cache/CacheType.h"
#include "axoncache/memory/MemoryHandler.h"
#include "axoncache/Constants.h"

namespace axoncache
{
using PutStats = std::pair<bool, uint32_t>;

class CacheBase
{
  public:
    CacheBase( std::unique_ptr<MemoryHandler> handler ) :
        mMemoryHandler( std::move( handler ) )
    {
    }

    virtual ~CacheBase() = default;

    // return {  true, collisions } if key inserted to the Cache
    // return { false, collisions } if key is not inserted to the Cache, ex. key already exists
    virtual auto put( std::string_view key, std::string_view value ) -> PutStats = 0;
    virtual auto put( std::string_view key, const std::vector<std::string_view> & value ) -> PutStats = 0;
    virtual auto put( std::string_view key, bool & value ) -> PutStats = 0;
    virtual auto put( std::string_view key, int64_t & value ) -> PutStats = 0;
    virtual auto put( std::string_view key, double & value ) -> PutStats = 0;
    virtual auto put( std::string_view key, const std::vector<float> & value ) -> PutStats = 0;

    [[nodiscard]] virtual auto type() const -> CacheType = 0;
    [[nodiscard]] virtual auto hashcodeBits() const -> uint16_t = 0;
    [[nodiscard]] virtual auto offsetBits() const -> uint16_t = 0;
    [[nodiscard]] virtual auto hashFuncId() const -> uint16_t = 0;
    [[nodiscard]] virtual auto maxLoadFactor() const -> double = 0;
    [[nodiscard]] virtual auto maxCollisions() const -> uint32_t = 0;
    [[nodiscard]] virtual auto numberOfEntries() const -> uint64_t = 0;
    [[nodiscard]] virtual auto maxNumberEntries() const -> uint64_t = 0; // only meaningful for linear probe
    [[nodiscard]] virtual auto numberOfKeySlots() const -> uint64_t = 0;
    [[nodiscard]] virtual auto creationTimeMs() const -> uint64_t = 0;
    [[nodiscard]] virtual auto dataSize() const -> uint64_t = 0; // dataSpace size in bytes
    [[nodiscard]] virtual auto size() const -> uint64_t = 0;     // total size in bytes
    [[nodiscard]] virtual auto headerInfo() const -> std::vector<std::pair<std::string, std::string>> = 0;

    virtual auto output( std::ostream & output ) const -> void = 0;

    [[nodiscard]] constexpr auto virtual version() const -> uint16_t final
    {
        // Version has to fit in a uint16_t so limit max version of each part
        static_assert( AXONCACHE_VERSION_MAJOR < 64, "Major version version number limit reached" );
        static_assert( AXONCACHE_VERSION_MINOR < 99, "Minor version version number limit reached" );
        static_assert( AXONCACHE_VERSION_PATCH < 10, "Patch version version number limit reached" );
        return ( AXONCACHE_VERSION_MAJOR * 1000 ) + ( AXONCACHE_VERSION_MINOR * 10 ) + AXONCACHE_VERSION_PATCH;
    }

    // Get/contains are explicitly not mark virtual. In production we do not want to pay the cost of the virtual call.
    /* auto get( std::string_view key, std::string_view defaultValue ) const -> std::string_view; */
    /* auto get( std::string_view key, std::vector<std::string_view> defaultValue ) const -> std::vector<std::string_view>; */
    /* auto contains( std::string_view key ) const -> bool; */

  protected:
    auto memoryHandler() const -> const MemoryHandler *
    {
        return mMemoryHandler.get();
    }

    auto mutableMemoryHandler() -> MemoryHandler *
    {
        return mMemoryHandler.get();
    }

    std::unique_ptr<MemoryHandler> mMemoryHandler;
};
} // axoncache
