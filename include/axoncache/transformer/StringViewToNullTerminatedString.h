// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <string_view>

namespace axoncache
{
class StringViewToNullTerminatedString
{
  public:
    static auto transform( std::string_view input ) -> std::string;

    [[nodiscard]] static inline auto trimExtraNullTerminator( std::string_view input ) -> std::string_view
    {
        return std::string_view{ input.data(), input.size() - 1 }; // Size is guaranteed to be more than one byte
    }
};
}
