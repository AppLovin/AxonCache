// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string_view>
#include <utility>
#include <axoncache/domain/CacheValue.h>
#include <doctest/doctest.h>
#include <sstream>
#include <string>
#include <vector>

using namespace axoncache;

TEST_CASE( "CacheValueTestStringType" )
{
    CacheValue value( "helloworld" );
    CHECK( value.type() == CacheValueType::String );
    CHECK( value.asString() == "helloworld" );
    CHECK( value.toDebugString() == R"({"type":"String", "value":"helloworld"})" );
}

TEST_CASE( "CacheValueTestStringList" )
{
    CacheValue value( std::vector<std::string_view>{ "hello", "world" } );
    CHECK( value.type() == CacheValueType::StringList );
    const auto result = value.asStringList();
    CHECK( result.size() == 2 );
    CHECK( result[0] == "hello" );
    CHECK( result[1] == "world" );
    CHECK( value.toDebugString() == R"({"type":"StringList", "value":["hello", "world"]})" );

    std::stringstream ss;
    ss << value;
    CHECK( ss.str() == R"({"type":"StringList", "value":["hello", "world"]})" );
}

TEST_CASE( "CacheValueTestEqualityCheck" )
{
    CacheValue valueStr( std::string_view{ "hello" } );
    CacheValue valueStrEql( std::string_view{ "hello" } );
    CacheValue valueStrDiff( std::string_view{ "world" } );

    CacheValue valueList( std::vector<std::string_view>{ "hello", "world" } );
    CacheValue valueListEql( std::vector<std::string_view>{ "hello", "world" } );
    CacheValue valueListDiff( std::vector<std::string_view>{ "this", "diff" } );
    CacheValue valueListDiff2( std::vector<std::string_view>{ "diff" } );

    CacheValue valueNone;
    CacheValue valueNoneEql;
    bool result;
    result = ( valueStr == valueStr );
    CHECK( result );
    result = ( valueStr == valueStrEql );
    CHECK( result );
    result = ( valueStr != valueStrDiff );
    CHECK( result );

    result = ( valueList == valueList );
    CHECK( result );
    result = ( valueList == valueListEql );
    CHECK( result );
    result = ( valueList != valueListDiff );
    CHECK( result );
    result = ( valueList != valueListDiff2 );
    CHECK( result );

    result = ( valueNone == valueNone );
    CHECK( result );
    result = ( valueNone == valueNoneEql );
    CHECK( result );
    result = ( valueNone != valueStr );
    CHECK( result );
    result = ( valueNone != valueList );
    CHECK( result );
}

TEST_CASE( "CacheValueTestKeyValuePair" )
{
    CacheValue value( std::vector<std::string_view>{ "hello", "world" } );

    std::stringstream ss;
    ss << std::make_pair( std::string_view{ "key" }, value );
    CHECK( ss.str() == R"({"key":{"type":"StringList", "value":["hello", "world"]}})" );

    std::stringstream typeSs;
    typeSs << value.type();
}

TEST_CASE( "CacheValueTestTypeToString" )
{
    CacheValue value( std::vector<std::string_view>{ "hello", "world" } );

    std::stringstream typeSs;
    typeSs << value.type();

    CHECK( typeSs.str() == "StringList" );

    CacheValue none;

    std::stringstream noneSs;
    noneSs << none.type();
    CHECK( noneSs.str() == "String" );
}
