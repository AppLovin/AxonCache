// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <algorithm>
#include <vector>
#include <string_view>
#include <cctype>

#include "axoncache/common/StringConversion.h"

namespace axoncache
{

template<typename T>
auto stringViewToNumber( const std::string_view sv ) -> T
{
    return axoncache::toNumber<T>( sv.data(), sv.data() + sv.size() );
}

template<typename T>
auto stringViewToNumber( const std::string_view sv, T defaultValue ) noexcept -> T
{
    return axoncache::toNumber<T>( sv.data(), sv.data() + sv.size(), defaultValue );
}

template<typename T>
auto stringViewToNormalNumber( const std::string_view sv ) -> T
{
    return axoncache::toNormalNumber<T>( sv.data(), sv.data() + sv.size() );
}

template<typename T>
auto stringViewToNormalNumber( const std::string_view sv, T defaultValue ) noexcept -> T
{
    return axoncache::toNormalNumber<T>( sv.data(), sv.data() + sv.size(), defaultValue );
}

template<typename UC = char>
auto stringViewSplit( const UC * first, const UC * last, UC delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<std::basic_string_view<UC>>
{
    if ( first == last )
    {
        return { "" };
    }

    std::vector<std::basic_string_view<UC>> output;
    output.reserve( sizeHint );

    const auto * current = first;

    while ( current < last )
    {
        if ( *current == delimiter )
        {
            output.emplace_back( first, current - first );
            first = current + 1; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
        }
        ++current; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
    }

    output.emplace_back( first, current - first );

    return output;
}

inline auto stringViewSplit( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<std::string_view>
{
    return stringViewSplit( input.data(), input.data() + input.size(), delimiter, sizeHint );
}

// Here std::search is good enough. if delimiter is long and complex,
// you can try std::strstr or std::search with std::boyer_moore_searcher
template<typename UC = char>
auto stringViewSplit( const UC * first, const UC * last, const UC * delimiter_first, const UC * delimiter_last,
                      std::size_t sizeHint = 8 ) -> std::vector<std::basic_string_view<UC>>
{
    if ( first == last )
    {
        return { "" };
    }

    if ( delimiter_first == delimiter_last )
    {
        return { std::string_view( first, last - first ) };
    }

    std::vector<std::basic_string_view<UC>> output;
    output.reserve( sizeHint );

    const UC * current = first;
    const UC * next = first;
    auto size = delimiter_last - delimiter_first;

    while ( current < last )
    {
        next = std::search( current, last, delimiter_first, delimiter_last );
        output.emplace_back( current, next - current );
        if ( next == last )
        {
            break;
        }
        current = next + size;
    }

    // If last substring is delimiter, we should add an empty string_view
    if ( next + size == last )
    {
        output.emplace_back();
    }

    return output;
}

inline auto stringViewSplit( const std::string_view input, const std::string_view delimiter,
                             std::size_t sizeHint = 8 ) -> std::vector<std::string_view>
{
    return stringViewSplit( input.data(), input.data() + input.size(), delimiter.data(), delimiter.data() + delimiter.size(), sizeHint );
}

template<typename UC = char>
auto stringViewSplitMultiDelimiters( const UC * first, const UC * last, const UC * delimiter_first, const UC * delimiter_last,
                                     std::size_t sizeHint = 8 ) -> std::vector<std::basic_string_view<UC>>
{
    if ( first == last )
    {
        return { "" };
    }

    if ( delimiter_first == delimiter_last )
    {
        return { std::string_view( first, last - first ) };
    }

    std::vector<std::basic_string_view<UC>> output;
    output.reserve( sizeHint );

    const UC * current = first;
    const UC * next = first;

    while ( current < last )
    {
        next = std::find_first_of( current, last, delimiter_first, delimiter_last );
        output.emplace_back( current, next - current );
        if ( next == last )
        {
            break;
        }
        current = next + 1;
    }

    // If last character is delimiter, we should add an empty string_view
    if ( next + 1 == last )
    {
        output.emplace_back();
    }

    return output;
}

inline auto stringViewSplitMultiDelimiters( const std::string_view input, const std::string_view delimiter,
                                            std::size_t sizeHint = 8 ) -> std::vector<std::string_view>
{
    return stringViewSplitMultiDelimiters( input.data(), input.data() + input.size(), delimiter.data(), delimiter.data() + delimiter.size(),
                                           sizeHint );
}

template<typename UC>
constexpr auto stringViewTrim( const std::basic_string_view<UC> input, const std::basic_string_view<UC> trimChars ) -> std::basic_string_view<UC>
{
    const auto first = input.find_first_not_of( trimChars );
    if ( first == std::basic_string_view<UC>::npos )
    {
        return {};
    }
    const auto last = input.find_last_not_of( trimChars );
    return input.substr( first, last - first + 1 );
}

inline constexpr auto stringViewTrim( const std::string_view input, const std::string_view trimChars ) -> std::string_view
{
    return stringViewTrim<char>( input, trimChars );
}

// Usually we only need to trim spaces, this specialized version can be more efficient
// std::isspace checks if the character is the following whitespace character:
// space (0x20, ' ')
// form feed (0x0c, '\f')
// line feed (0x0a, '\n')
// carriage return (0x0d, '\r')
// horizontal tab (0x09, '\t')
// vertical tab (0x0b, '\v')
template<typename UC = char>
constexpr auto trimSpaces( const UC * start, const UC * end ) -> std::string_view
{
    if ( start == end )
    {
        return {};
    }

    const auto * left = start;
    const auto * right = end - 1; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]

    while ( left < end && std::isspace( static_cast<unsigned char>( *left ) ) ) // NOLINT [readability-implicit-bool-conversion]
    {
        ++left; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
    }

    while ( right >= left && std::isspace( static_cast<unsigned char>( *right ) ) ) // NOLINT [readability-implicit-bool-conversion]
    {
        --right; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
    }

    return { left, static_cast<std::size_t>( right - left + 1 ) };
}

inline constexpr auto stringViewTrimSpaces( std::string_view input ) -> std::string_view
{
    return trimSpaces( input.data(), input.data() + input.size() );
}

template<typename T>
auto stringViewToVector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<T>
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );

    std::vector<T> output;
    output.reserve( sizeHint );

    const auto * start = input.data();
    const auto * current = input.data();
    const auto * end = input.data() + input.size();

    while ( current < end )
    {
        if ( *current == delimiter )
        {
            auto sv = trimSpaces( start, current );
            output.emplace_back( stringViewToNormalNumber<T>( sv ) );
            start = current + 1; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
        }
        ++current; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
    }

    auto sv = trimSpaces( start, current );
    output.emplace_back( stringViewToNormalNumber<T>( sv ) );

    return output;
}

template<typename T>
auto stringViewToVectorWithDefault( const std::string_view input, T defaultValue, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<T>
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );

    std::vector<T> output;
    output.reserve( sizeHint );

    const auto * start = input.data();
    const auto * current = input.data();
    const auto * end = input.data() + input.size();

    while ( current < end )
    {
        if ( *current == delimiter )
        {
            auto sv = trimSpaces( start, current );
            output.emplace_back( stringViewToNormalNumber<T>( sv, defaultValue ) );
            start = current + 1; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
        }
        ++current; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
    }

    auto sv = trimSpaces( start, current );
    output.emplace_back( stringViewToNormalNumber<T>( sv, defaultValue ) );

    return output;
}

template<typename T, typename Function>
auto stringViewToVector( const std::string_view input, char delimiter, Function processFunc, std::size_t sizeHint = 8 ) -> std::vector<T>
{
    static_assert( std::is_arithmetic_v<T>, "T must be an arithmetic type" );
    static_assert( std::is_invocable_r_v<std::string_view, Function, const char *, const char *>,
                   "processFunc must be callable with (const char*, const char*) and return std::string_view" );

    std::vector<T> output;
    output.reserve( sizeHint );

    const auto * start = input.data();
    const auto * current = input.data();
    const auto * end = input.data() + input.size();

    while ( current < end )
    {
        if ( *current == delimiter )
        {
            auto processed = processFunc( start, current );
            output.emplace_back( stringViewToNormalNumber<T>( processed ) );
            start = current + 1; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
        }
        ++current; // NOLINT [cppcoreguidelines-pro-bounds-pointer-arithmetic]
    }

    if ( start != current )
    {
        auto processed = processFunc( start, current );
        output.emplace_back( stringViewToNormalNumber<T>( processed ) );
    }

    return output;
}

inline auto toInt32( const std::string_view sv ) -> int32_t
{
    return stringViewToNumber<int32_t>( sv );
}

inline auto toInt32( const std::string_view sv, int32_t defaultValue ) -> int32_t
{
    return stringViewToNumber<int32_t>( sv, defaultValue );
}

inline auto toUint32( const std::string_view sv ) -> uint32_t
{
    return stringViewToNumber<uint32_t>( sv );
}

inline auto toUint32( const std::string_view sv, uint32_t defaultValue ) -> uint32_t
{
    return stringViewToNumber<uint32_t>( sv, defaultValue );
}

inline auto toInt64( const std::string_view sv ) -> int64_t
{
    return stringViewToNumber<int64_t>( sv );
}

inline auto toInt64( const std::string_view sv, int64_t defaultValue ) -> int64_t
{
    return stringViewToNumber<int64_t>( sv, defaultValue );
}

inline auto toUint64( const std::string_view sv ) -> uint64_t
{
    return stringViewToNumber<uint64_t>( sv );
}

inline auto toUint64( const std::string_view sv, uint64_t defaultValue ) -> uint64_t
{
    return stringViewToNumber<uint64_t>( sv, defaultValue );
}

inline auto toFloat( const std::string_view sv ) -> float
{
    return stringViewToNumber<float>( sv );
}

inline auto toFloat( const std::string_view sv, float defaultValue ) -> float
{
    return stringViewToNumber<float>( sv, defaultValue );
}

inline auto toDouble( const std::string_view sv ) -> double
{
    return stringViewToNumber<double>( sv );
}

inline auto toDouble( const std::string_view sv, double defaultValue ) -> double
{
    return stringViewToNumber<double>( sv, defaultValue );
}

inline auto toNormalFloat( const std::string_view sv ) -> float
{
    return stringViewToNormalNumber<float>( sv );
}

inline auto toNormalFloat( const std::string_view sv, float defaultValue ) -> float
{
    return stringViewToNormalNumber<float>( sv, defaultValue );
}

inline auto toNormalDouble( const std::string_view sv ) -> double
{
    return stringViewToNormalNumber<double>( sv );
}

inline auto toNormalDouble( const std::string_view sv, double defaultValue ) -> double
{
    return stringViewToNormalNumber<double>( sv, defaultValue );
}

inline auto toInt32Vector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<int32_t>
{
    return stringViewToVector<int32_t>( input, delimiter, sizeHint );
}

inline auto toInt32VectorWithDefault( const std::string_view input, int32_t defaultValue, char delimiter = ' ',
                                      std::size_t sizeHint = 8 ) -> std::vector<int32_t>
{
    return stringViewToVectorWithDefault<int32_t>( input, defaultValue, delimiter, sizeHint );
}

inline auto toInt64Vector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<int64_t>
{
    return stringViewToVector<int64_t>( input, delimiter, sizeHint );
}

inline auto toInt64VectorWithDefault( const std::string_view input, int64_t defaultValue, char delimiter = ' ',
                                      std::size_t sizeHint = 8 ) -> std::vector<int64_t>
{
    return stringViewToVectorWithDefault<int64_t>( input, defaultValue, delimiter, sizeHint );
}

inline auto toUint32Vector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<uint32_t>
{
    return stringViewToVector<uint32_t>( input, delimiter, sizeHint );
}

inline auto toUint32VectorWithDefault( const std::string_view input, uint32_t defaultValue, char delimiter = ' ',
                                       std::size_t sizeHint = 8 ) -> std::vector<uint32_t>
{
    return stringViewToVectorWithDefault<uint32_t>( input, defaultValue, delimiter, sizeHint );
}

inline auto toUint64Vector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<uint64_t>
{
    return stringViewToVector<uint64_t>( input, delimiter, sizeHint );
}

inline auto toUint64VectorWithDefault( const std::string_view input, uint64_t defaultValue, char delimiter = ' ',
                                       std::size_t sizeHint = 8 ) -> std::vector<uint64_t>
{
    return stringViewToVectorWithDefault<uint64_t>( input, defaultValue, delimiter, sizeHint );
}

inline auto toFloatVector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<float>
{
    return stringViewToVector<float>( input, delimiter, sizeHint );
}

inline auto toFloatVectorWithDefault( const std::string_view input, float defaultValue, char delimiter = ' ',
                                      std::size_t sizeHint = 8 ) -> std::vector<float>
{
    return stringViewToVectorWithDefault<float>( input, defaultValue, delimiter, sizeHint );
}

inline auto toDoubleVector( const std::string_view input, char delimiter = ' ', std::size_t sizeHint = 8 ) -> std::vector<double>
{
    return stringViewToVector<double>( input, delimiter, sizeHint );
}

inline auto toDoubleVectorWithDefault( const std::string_view input, double defaultValue, char delimiter = ' ',
                                       std::size_t sizeHint = 8 ) -> std::vector<double>
{
    return stringViewToVectorWithDefault<double>( input, defaultValue, delimiter, sizeHint );
}

}
