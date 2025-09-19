// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string_view>
#include <cstdint>
#include <vector>
#include "axoncache/domain/CacheValue.h"

namespace axoncache
{
class SharedSettingsProvider;

class CacheValueParser
{
  public:
    CacheValueParser( const SharedSettingsProvider * settings );
    virtual ~CacheValueParser() = default;

    CacheValueParser( CacheValueParser && ) = delete;
    CacheValueParser( CacheValueParser & ) = delete;
    CacheValueParser( const CacheValueParser & ) = delete;
    auto operator=( const CacheValueParser & ) -> CacheValueParser & = delete;
    auto operator=( CacheValueParser & ) -> CacheValueParser & = delete;
    auto operator=( CacheValueParser && ) -> CacheValueParser & = delete;

    [[nodiscard]] auto parseValue( const char * line, uint64_t size ) const -> CacheKeyValue;

  private:
    [[nodiscard]] auto parseValue( const char * line, uint64_t size, std::string_view key, bool isVector ) const -> CacheKeyValue;
    [[nodiscard]] auto parseVector( std::string_view valueStr ) const -> std::vector<std::string_view>;

    const char mVectorType;
    const char mVectorElemSeparator;
    const char mKeyValueSeparator;
};
} // namespace axoncache
