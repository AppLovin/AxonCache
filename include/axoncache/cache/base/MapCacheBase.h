// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <string_view>
#include <memory>
#include <utility>
#include <cstdint>
#include <iosfwd>
#include <unordered_map>
#include <vector>
#include "axoncache/Constants.h"
#include "axoncache/cache/CacheBase.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheHeader.h"
#include "axoncache/memory/MemoryHandler.h"

namespace axoncache
{
class MapCacheBase : public CacheBase
{
  public:
    explicit MapCacheBase( std::unique_ptr<MemoryHandler> handler ) :
        CacheBase( std::move( handler ) )
    {
    }

    MapCacheBase( [[maybe_unused]] const CacheHeader & header, std::unique_ptr<MemoryHandler> handler ) :
        CacheBase( std::move( handler ) )
    {
    }

    MapCacheBase( const MapCacheBase & ) = delete;
    MapCacheBase( MapCacheBase & ) = delete;
    MapCacheBase( MapCacheBase && ) = delete;
    auto operator=( const MapCacheBase & ) -> MapCacheBase & = delete;
    auto operator=( MapCacheBase & ) -> MapCacheBase & = delete;
    auto operator=( MapCacheBase && ) -> MapCacheBase & = delete;

    ~MapCacheBase() override = default;
    auto put( std::string_view key, std::string_view value ) -> std::pair<bool, uint32_t> override;
    auto put( std::string_view key, const std::vector<std::string_view> & value ) -> std::pair<bool, uint32_t> override;
    auto put( std::string_view /* key */, bool & /* value */ ) -> std::pair<bool, uint32_t> override
    {
        // not supported
        return std::make_pair( false, 0 );
    }

    auto put( std::string_view /* key */, int64_t & /* value */ ) -> std::pair<bool, uint32_t> override
    {
        // not supported
        return std::make_pair( false, 0 );
    }

    auto put( std::string_view /* key */, double & /* value */ ) -> std::pair<bool, uint32_t> override
    {
        // not supported
        return std::make_pair( false, 0 );
    }

    auto put( std::string_view /* key */, const std::vector<float> & /* value */ ) -> std::pair<bool, uint32_t> override
    {
        // not supported
        return std::make_pair( false, 0 );
    }

    [[nodiscard]] auto type() const -> CacheType override
    {
        return CacheType::MAP;
    }

    [[nodiscard]] auto hashcodeBits() const -> uint16_t override
    {
        return 0; // not used for MapCacheBase
    }

    [[nodiscard]] auto offsetBits() const -> uint16_t override
    {
        return 0; // not used for MapCacheBase
    }

    [[nodiscard]] auto hashFuncId() const -> uint16_t override
    {
        return Constants::HashFuncId::UNKNOWN;
    }

    [[nodiscard]] auto maxLoadFactor() const -> double override
    {
        return 0.0f;
    }

    [[nodiscard]] auto maxCollisions() const -> uint32_t override
    {
        return 0;
    }

    [[nodiscard]] auto numberOfEntries() const -> uint64_t override
    {
        return mStrings.size() + mStringLists.size();
    }

    [[nodiscard]] auto maxNumberEntries() const -> uint64_t override
    {
        return numberOfKeySlots(); // not used for MapCacheBase
    }

    [[nodiscard]] auto numberOfKeySlots() const -> uint64_t override
    {
        return mStrings.size() + mStringLists.size();
    }

    [[nodiscard]] auto creationTimeMs() const -> uint64_t override
    {
        return 0; // not supported for MapCacheBase
    }

    [[nodiscard]] auto dataSize() const -> uint64_t override
    {
        return 0; // not supported for MapCacheBase
    }

    [[nodiscard]] auto size() const -> uint64_t override
    {
        return 0; // not supported for MapCacheBase
    }

    [[nodiscard]] auto headerInfo() const -> std::vector<std::pair<std::string, std::string>> override
    {
        return {};
    }

    auto output( std::ostream & output ) const -> void override;

    auto get( std::string_view key ) const -> std::string_view;
    auto getVector( std::string_view key ) const -> std::vector<std::string_view>;

    auto contains( std::string_view key ) const -> bool;

  private:
    std::unordered_map<std::string, std::string> mStrings;
    std::unordered_map<std::string, std::vector<std::string>> mStringLists;
};
} // axoncache
