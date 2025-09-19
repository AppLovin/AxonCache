// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/parser/CacheValueParser.h"
#include <algorithm>
#include <utility>
#include <cstring>
#include <string_view>
#include "axoncache/logger/Logger.h"
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/Constants.h"

using namespace axoncache;

CacheValueParser::CacheValueParser( const SharedSettingsProvider * settings ) :
    mVectorType( settings->getChar( Constants::ConfKey::kControlCharVectorType, Constants::ConfDefault::kControlCharVectorType ) ),
    mVectorElemSeparator( settings->getChar( Constants::ConfKey::kControlCharVectorElem, Constants::ConfDefault::kControlCharVectorElem ) ),
    mKeyValueSeparator( settings->getChar( Constants::ConfKey::kControlCharKeyValue, Constants::ConfDefault::kControlCharKeyValue ) )
{
}

auto CacheValueParser::parseValue( const char * line, uint64_t size ) const -> CacheKeyValue
{
    if ( size != 0UL )
    {
        const auto isVector = ( line[0] == mVectorType );
        auto * keyEnd = ( char * )std::memchr( line, mKeyValueSeparator, size );
        if ( keyEnd != nullptr )
        {
            auto keyStr = std::string_view( line + ( isVector ? 1 : 0 ), keyEnd - line - ( isVector ? 1 : 0 ) ); // skip the vector type
            return parseValue( line, size, keyStr, isVector );
        }
    }

    AL_LOG_ERROR( "Invalid line: " + std::string( line, size ) );
    return {};
}

auto CacheValueParser::parseValue( const char * line, uint64_t size, std::string_view key, bool isVector ) const -> CacheKeyValue
{
    const auto * keyEnd = key.data() + key.size();
    auto valueStr = std::string_view( keyEnd + 1, size - ( keyEnd - line ) - 1 );
    if ( isVector )
    {
        return std::make_pair( key, CacheValue{ parseVector( valueStr ) } );
    }

    return std::make_pair( key, CacheValue{ valueStr } );
}

auto CacheValueParser::parseVector( std::string_view valueStr ) const -> std::vector<std::string_view>
{
    std::vector<std::string_view> values;
    // vector is non-empty
    if ( !valueStr.empty() )
    {
        while ( true )
        {
            auto * valueEnd = ( char * )std::memchr( valueStr.data(), mVectorElemSeparator, valueStr.size() );
            if ( valueEnd == nullptr ) // last value
            {
                values.emplace_back( std::string_view{ valueStr.data(), valueStr.size() } );
                break;
            }

            values.emplace_back( valueStr.data(), valueEnd - valueStr.data() );
            valueStr = std::string_view( valueEnd + 1, valueStr.size() - values.back().size() - 1 );
        }

        std::sort( values.begin(), values.end() );
    }

    return values;
}
