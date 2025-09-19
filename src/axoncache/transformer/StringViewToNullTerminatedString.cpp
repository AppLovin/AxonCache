// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/transformer/StringViewToNullTerminatedString.h"
#include <string>

using namespace axoncache;

auto StringViewToNullTerminatedString::transform( std::string_view input ) -> std::string
{
    std::string str( input );
    str.push_back( '\0' ); // Add an extra null terminator
    return str;
}
