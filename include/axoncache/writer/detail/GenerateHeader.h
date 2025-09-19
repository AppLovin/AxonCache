// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <iostream>
#include "axoncache/domain/CacheHeader.h"
namespace axoncache
{
class CacheBase;
}

namespace axoncache
{
class GenerateHeader
{
  public:
    auto write( const CacheBase * cache, std::string_view cacheName, std::ostream & output ) const -> void;

    auto read( std::istream & input ) const -> std::pair<std::string, CacheHeader>;

  protected:
  private:
};
}
