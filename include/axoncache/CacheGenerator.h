// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <cstdint>
#include <vector>
namespace axoncache
{
class SharedSettingsProvider;
}
namespace axoncache
{
enum class CacheType;
}

namespace axoncache
{

/**
 * @brief CacheGenerator */
class CacheGenerator
{
  public:
    /**
     * @brief Creates a new CacheGenerator
     */
    CacheGenerator( const SharedSettingsProvider * settings );
    virtual ~CacheGenerator() = default;

    CacheGenerator( CacheGenerator && ) = delete;
    CacheGenerator( const CacheGenerator & ) = delete;
    auto operator=( const CacheGenerator & ) -> CacheGenerator = delete;
    auto operator=( CacheGenerator && ) -> CacheGenerator = delete;

    /**
     * @brief Starts program
     */
    auto start( const std::vector<std::string> & values = {} ) const -> void;

  protected:
    struct CacheArgs_s
    {
        uint16_t offsetBits;
        uint64_t numberOfKeySlots;
        double maxLoadFactor;

        std::string cacheName;
        CacheType cacheType;
        std::string outputDirectory;
        std::vector<std::string> inputFiles;
    };
    using CacheArgs = struct CacheArgs_s;

    [[nodiscard]] auto settings() const -> const SharedSettingsProvider *
    {
        return mSettings;
    }

  private:
    const SharedSettingsProvider * mSettings;

    std::vector<CacheArgs> mCacheArgs;
};

} // namespace axoncache
