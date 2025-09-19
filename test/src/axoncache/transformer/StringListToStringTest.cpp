// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <axoncache/Constants.h>
#include <axoncache/transformer/StringListToString.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include <vector>
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

TEST_CASE( "StringListToStringToStringOvermaxLength" )
{
    // Try to insert a vector with an individual entry that is too large
    std::string_view largeStrView{ ( const char * )1024UL, 1 << 20 }; // NOLINT
    CHECK_THROWS_WITH( StringListToString::transform( std::vector<std::string_view>{ largeStrView } ), "input vector element 1048576 too large. max=65535" );

    // Try to insert a vector with too many elements
    std::vector<std::string_view> largeVector( Constants::Limit::kVectorLength + 1 );
    CHECK_THROWS_WITH( StringListToString::transform( largeVector ), "input vector size 65536 too large. max=65535" );
}

TEST_CASE( "StringListToStringToStringSingle" )
{
    const auto str = StringListToString::transform( std::vector<std::string_view>( { "hello" } ) );
    std::string expected;
    uint16_t numberOfElements = 1u;                                           // NOLINT
    expected.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                           // NOLINT
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "hello" );                                     // NOLINT
    expected.push_back( '\0' );
    CHECK( str.size() == expected.size() );
    CHECK( str == expected );
}

TEST_CASE( "StringListToStringToStringMulti" )
{
    const auto str = StringListToString::transform( std::vector<std::string_view>( { "hello", "world" } ) );
    std::string expected;

    uint16_t numberOfElements = 2u;                                           // NOLINT
    expected.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                           // NOLINT
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "hello" );                                     // NOLINT
    expected.push_back( '\0' );
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "world" );                                     // NOLINT
    expected.push_back( '\0' );
    CHECK( str.size() == expected.size() );
    CHECK( str == expected );
}

TEST_CASE( "StringListToStringToStringEmpty" )
{
    [[maybe_unused]] StringListToString trans;
    const auto str = StringListToString::transform( std::vector<std::string_view>() );
    std::string expected;

    uint16_t numberOfElements = 0u;                                           // NOLINT
    expected.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT
    CHECK( str.size() == expected.size() );
    CHECK( str == expected );
}

TEST_CASE( "StringListToStringToStringEmptyString" )
{
    [[maybe_unused]] StringListToString trans;
    const auto str = StringListToString::transform( std::vector<std::string_view>( { "" } ) );
    std::string expected;

    uint16_t numberOfElements = 1u;                                           // NOLINT
    expected.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 0u;                                           // NOLINT
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.push_back( '\0' );

    CHECK( str.size() == expected.size() );
    CHECK( str == expected );
}

TEST_CASE( "StringListToStringToStringSomeEmptyString" )
{
    const auto str = StringListToString::transform( std::vector<std::string_view>( { "hello", "", "world" } ) );
    std::string expected;

    uint16_t numberOfElements = 3u;                                           // NOLINT
    expected.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                           // NOLINT
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "hello" );                                     // NOLINT
    expected.push_back( '\0' );

    uint16_t length2 = 0u;                                           // NOLINT
    expected.append( ( const char * )&length2, sizeof( uint16_t ) ); // NOLINT
    expected.push_back( '\0' );

    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "world" );                                     // NOLINT
    expected.push_back( '\0' );
    CHECK( str.size() == expected.size() );
    CHECK( str == expected );
}

TEST_CASE( "StringListToStringToStringSomeEmptyStringAtEnd" )
{
    const auto str = StringListToString::transform( std::vector<std::string_view>( { "hello", "world", "" } ) );
    std::string expected;

    uint16_t numberOfElements = 3u;                                           // NOLINT
    expected.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                           // NOLINT
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "hello" );                                     // NOLINT
    expected.push_back( '\0' );
    expected.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    expected.append( "world" );                                     // NOLINT
    expected.push_back( '\0' );

    uint16_t length2 = 0u;                                           // NOLINT
    expected.append( ( const char * )&length2, sizeof( uint16_t ) ); // NOLINT
    expected.push_back( '\0' );

    CHECK( str.size() == expected.size() );
    CHECK( str == expected );
}

TEST_CASE( "StringListToStringToVectorSingle" )
{
    const auto expected = std::vector<std::string_view>( { "hello" } );
    std::string input;
    uint16_t numberOfElements = 1u;                                        // NOLINT
    input.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                        // NOLINT
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "hello" );                                     // NOLINT
    input.push_back( '\0' );

    const auto output = StringListToString::transform( input );
    CHECK( expected.size() == output.size() );
    CHECK( expected[0] == output[0] );
}

TEST_CASE( "StringListToStringToVectorMulti" )
{
    const auto expected = std::vector<std::string_view>( { "hello", "world" } );
    std::string input;

    uint16_t numberOfElements = 2u;                                        // NOLINT
    input.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                        // NOLINT
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "hello" );                                     // NOLINT
    input.push_back( '\0' );
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "world" );                                     // NOLINT
    input.push_back( '\0' );

    const auto output = StringListToString::transform( input );
    CHECK( expected.size() == output.size() );
    CHECK( expected[0] == output[0] );
    CHECK( expected[1] == output[1] );
}

TEST_CASE( "StringListToStringToVectorEmpty" )
{
    [[maybe_unused]] StringListToString trans;
    const auto expected = std::vector<std::string_view>();
    std::string input;

    uint16_t numberOfElements = 0u;                                        // NOLINT
    input.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    const auto output = StringListToString::transform( input );
    CHECK( expected.size() == output.size() );
    CHECK( expected.empty() );
}

TEST_CASE( "StringListToStringToVectorEmptyString" )
{
    [[maybe_unused]] StringListToString trans;
    const auto expected = std::vector<std::string_view>( { "" } );
    std::string input;

    uint16_t numberOfElements = 1u;                                        // NOLINT
    input.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 0u;                                        // NOLINT
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.push_back( '\0' );

    const auto output = StringListToString::transform( input );
    CHECK( expected.size() == output.size() );
    CHECK( expected[0] == output[0] );
}

TEST_CASE( "StringListToStringToVectorSomeEmptyString" )
{
    const auto expected = std::vector<std::string_view>( { "hello", "", "world" } );
    std::string input;

    uint16_t numberOfElements = 3u;                                        // NOLINT
    input.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                        // NOLINT
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "hello" );                                     // NOLINT
    input.push_back( '\0' );

    uint16_t length2 = 0u;                                        // NOLINT
    input.append( ( const char * )&length2, sizeof( uint16_t ) ); // NOLINT
    input.push_back( '\0' );

    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "world" );                                     // NOLINT
    input.push_back( '\0' );

    const auto output = StringListToString::transform( input );
    CHECK( expected.size() == output.size() );
    CHECK( expected[0] == output[0] );
    CHECK( expected[1] == output[1] );
    CHECK( expected[2] == output[2] );
}

TEST_CASE( "StringListToStringToVectorSomeEmptyStringAtEnd" )
{
    const auto expected = std::vector<std::string_view>( { "hello", "world", "" } );
    std::string input;

    uint16_t numberOfElements = 3u;                                        // NOLINT
    input.append( ( const char * )&numberOfElements, sizeof( uint16_t ) ); // NOLINT

    uint16_t length = 5u;                                        // NOLINT
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "hello" );                                     // NOLINT
    input.push_back( '\0' );
    input.append( ( const char * )&length, sizeof( uint16_t ) ); // NOLINT
    input.append( "world" );                                     // NOLINT
    input.push_back( '\0' );

    uint16_t length2 = 0u;                                        // NOLINT
    input.append( ( const char * )&length2, sizeof( uint16_t ) ); // NOLINT
    input.push_back( '\0' );

    const auto output = StringListToString::transform( input );
    CHECK( expected.size() == output.size() );
    CHECK( expected[0] == output[0] );
    CHECK( expected[1] == output[1] );
    CHECK( expected[2] == output[2] );
}
