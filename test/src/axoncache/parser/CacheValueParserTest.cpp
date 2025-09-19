// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <axoncache/Constants.h>
#include <axoncache/parser/CacheValueParser.h>
#include <doctest/doctest.h>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/domain/CacheValue.h"

using namespace axoncache;

TEST_CASE( "CacheValueParserSingleLine" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line = "hello=world";
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::String );
    CHECK( value.second.asString() == std::string_view{ "world" } );
}

TEST_CASE( "CacheValueParserEmptyValue" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line = "hello=";
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::String );
    CHECK( value.second.asString() == std::string_view{ "" } );
}

TEST_CASE( "CacheValueParserEmptyLine" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line = "";
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "" );
    CHECK( value.second.type() == CacheValueType::String );
    CHECK( value.second.asString() == std::string_view{ "" } );
}

TEST_CASE( "CacheValueParserEmptyLines" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line = "\n\n";
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "" );
    CHECK( value.second.type() == CacheValueType::String );
    CHECK( value.second.asString() == std::string_view{ "" } );
}

TEST_CASE( "CacheValueParserInvalidKeyPair" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line = "hellothisisnotakeypair";
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "" );
    CHECK( value.second.type() == CacheValueType::String );
    CHECK( value.second.asString() == std::string_view{ "" } );
}

TEST_CASE( "CacheValueParserVector" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line;
    line.push_back( Constants::ConfDefault::kControlCharVectorType );
    line.append( "hello=a|b|1337|world" );
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::StringList );
    const auto & vec = value.second.asStringList();
    CHECK( vec.size() == 4 );
}

TEST_CASE( "CacheValueParserVectorSorting" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line;
    line.push_back( Constants::ConfDefault::kControlCharVectorType );
    line.append( "hello=a|b|1337|world" );
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::StringList );
    const auto & vec = value.second.asStringList();
    CHECK( vec.size() == 4 );
    CHECK( vec[0] == "1337" );
    CHECK( vec[1] == "a" );
    CHECK( vec[2] == "b" );
    CHECK( vec[3] == "world" );
}

TEST_CASE( "CacheValueParserVectorEmpty" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line;
    line.push_back( Constants::ConfDefault::kControlCharVectorType );
    line.append( "hello=" );
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::StringList );
    const auto & vec = value.second.asStringList();
    CHECK( vec.size() == 0 );
}

TEST_CASE( "CacheValueParserVectorSingleValue" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line;
    line.push_back( Constants::ConfDefault::kControlCharVectorType );
    line.append( "hello=a" );
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::StringList );
    const auto & vec = value.second.asStringList();
    CHECK( vec.size() == 1 );
    CHECK( vec[0] == "a" );
}

TEST_CASE( "CacheValueParserVectorAllEmptyValues" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line;
    line.push_back( Constants::ConfDefault::kControlCharVectorType );
    line.append( "hello=|||" );
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::StringList );
    const auto & vec = value.second.asStringList();
    CHECK( vec.size() == 4 );
    CHECK( vec[0] == "" );
    CHECK( vec[1] == "" );
    CHECK( vec[2] == "" );
    CHECK( vec[3] == "" );
}

TEST_CASE( "CacheValueParserVectorSomeEmptyValues" )
{
    SharedSettingsProvider settings( "" );
    CacheValueParser parser( &settings );
    std::string line;
    line.push_back( Constants::ConfDefault::kControlCharVectorType );
    line.append( "hello=b||a|world" );
    auto value = parser.parseValue( line.data(), line.size() );
    CHECK( value.first == "hello" );
    CHECK( value.second.type() == CacheValueType::StringList );
    const auto & vec = value.second.asStringList();
    CHECK( vec.size() == 4 );
    CHECK( vec[0] == "" );
    CHECK( vec[1] == "a" );
    CHECK( vec[2] == "b" );
    CHECK( vec[3] == "world" );
}

// TODO: check max key length, value length, etc.
