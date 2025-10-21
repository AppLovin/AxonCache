// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <cstdint>

namespace axoncache // NOLINT
{
namespace math
{

[[maybe_unused]] [[nodiscard]] static auto roundUpToPowerOfTwo( uint64_t number ) -> uint64_t
{
    if ( number <= 1 )
        return 1;
    return 1UL << ( 64UL - __builtin_clzl( number - 1UL ) );
}

}
}
