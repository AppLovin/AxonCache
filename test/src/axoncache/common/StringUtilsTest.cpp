// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <array>
#include <cmath>
#include <string_view>
#include "axoncache/common/StringUtils.h"
#include "doctest/doctest.h"

using namespace axoncache;

TEST_CASE( "StringUtilsToDoubleHonorsStringViewBounds" )
{
    constexpr std::array<char, 5> buffer{ '1', '2', '.', '5', '9' };
    constexpr std::array<char, 4> exactBuffer{ '1', '2', '.', '5' };

    // The adjacent '9' is deliberately numeric. Parsing must stop at the
    // string_view boundary instead of treating the buffer as a C string.
    CHECK( StringUtils::toDouble( std::string_view{ buffer.data(), 4 } ) == 12.5 );
    CHECK( StringUtils::toDouble( std::string_view{ exactBuffer.data(), exactBuffer.size() } ) == 12.5 );
}

TEST_CASE( "StringUtilsToDoubleRequiresTheEntireSpan" )
{
    CHECK( StringUtils::toDouble( "-12.5" ) == -12.5 );
    CHECK( StringUtils::toDouble( "6.25e2" ) == 625.0 );
    CHECK( StringUtils::toDouble( "1.0E+10" ) == 1.0e10 );
    CHECK( StringUtils::toDouble( "12.5x" ) == 0.0 );
    CHECK( StringUtils::toDouble( "" ) == 0.0 );
    CHECK( std::isinf( StringUtils::toDouble( "inf" ) ) );
    CHECK( std::isinf( StringUtils::toDouble( "Infinity" ) ) );
    CHECK( std::isnan( StringUtils::toDouble( "nan" ) ) );
    CHECK( std::isnan( StringUtils::toDouble( "NaN" ) ) );
}

TEST_CASE( "StringUtilsToDoubleCompatiblePreservesLegacyGrammarWithinBounds" )
{
    constexpr std::array<char, 5> buffer{ '+', '1', '2', '.', '5' };

    CHECK( StringUtils::toDoubleCompatible( " 12.5" ) == 12.5 );
    CHECK( StringUtils::toDoubleCompatible( std::string_view{ buffer.data(), buffer.size() } ) == 12.5 );
    CHECK( StringUtils::toDoubleCompatible( "0x1.8p1" ) == 3.0 );
    CHECK( StringUtils::toDoubleCompatible( "12.5x" ) == 0.0 );
}
