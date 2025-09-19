// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <type_traits>
#include <system_error>

// The fast_float library provides fast header-only implementations for the C++ from_chars functions
// Some restrictions:
// 1. The from_chars function does not skip leading white-space characters.
// 2. A leading + sign is forbidden.
// 3. Round to an even mantissa when they are in-between two binary floating-point numbers.
// 4. Only support int, float and double types
// 5. Only support the decimal format: we do not support hexadecimal strings.
// 6. For values that are either very large or very small (e.g., 1e9999),
//    using the infinity or negative infinity value and the returned ec is set to std::errc::result_out_of_range.
//
// Use fast float as a single head
// https://github.com/fastfloat/fast_float/releases/download/v6.1.1/fast_float.h
#include "fast_float.h"

namespace axoncache
{

template<typename T, typename UC = char>
constexpr auto toNumber( const UC * first, const UC * last ) -> T
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );

    T value;
    auto [ptr, error_code] = fast_float::from_chars( first, last, value );
    if ( error_code == std::errc() ) [[likely]]
    {
        return value;
    }
    else
    {
        throw std::runtime_error( std::string( "Failed to convert string to number. error_code = " + std::to_string( static_cast<int>( error_code ) ) ) );
    }
}

template<typename T, typename UC = char>
constexpr auto toNumber( const UC * first, const UC * last, T defaultValue ) noexcept -> T
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );

    T value;
    auto [ptr, error_code] = fast_float::from_chars( first, last, value );
    if ( error_code == std::errc() ) [[likely]]
    {
        return value;
    }
    else
    {
        return defaultValue;
    }
}

template<typename T, typename UC = char>
auto toNormalNumber( const UC * first, const UC * last ) -> T
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );

    T value;
    auto [ptr, error_code] = fast_float::from_chars( first, last, value );

    if ( error_code == std::errc() ) [[likely]]
    {
        if constexpr ( std::is_floating_point_v<T> )
        {
            if ( !std::isnormal( value ) && value != 0 ) [[unlikely]]
            {
                throw std::runtime_error( "not a normal floating point number: " + std::to_string( value ) );
            }
        }
        return value;
    }

    throw std::runtime_error( std::string( "Failed to convert string to number. error_code = " + std::to_string( static_cast<int>( error_code ) ) ) );
}

template<typename T, typename UC = char>
auto toNormalNumber( const UC * first, const UC * last, T defaultValue ) -> T
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );

    T value;
    auto [ptr, error_code] = fast_float::from_chars( first, last, value );

    if ( error_code == std::errc() ) [[likely]]
    {
        if constexpr ( std::is_floating_point_v<T> )
        {
            if ( !std::isnormal( value ) && value != 0 ) [[unlikely]]
            {
                return defaultValue;
            }
        }
        return value;
    }
    else
    {
        return defaultValue;
    }
}

} // namespace alc
