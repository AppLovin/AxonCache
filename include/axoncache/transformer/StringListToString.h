// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace axoncache
{
class StringListToString
{
  public:
    static auto transform( const std::vector<std::string_view> & input ) -> std::string;

    static auto transform( std::string_view input ) -> std::vector<std::string_view>;
};
}
