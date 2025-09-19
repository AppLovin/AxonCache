// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/cache/base/MapCacheBase.h"
#include <algorithm>
#include <iterator>
#include <ios>

using namespace axoncache;

auto MapCacheBase::put( std::string_view key, std::string_view value ) -> std::pair<bool, uint32_t>
{
    const auto ret = mStrings.emplace( std::make_pair( std::string{ key }, std::string{ value } ) );
    return std::make_pair( ret.second, 0 );
}

auto MapCacheBase::put( std::string_view key, const std::vector<std::string_view> & value ) -> std::pair<bool, uint32_t>
{
    std::vector<std::string> result;
    result.reserve( value.size() );
    for ( const auto & s : value )
    {
        result.emplace_back( std::string{ s } );
    }
    const auto ret = mStringLists.emplace( std::make_pair( std::string{ key }, std::move( result ) ) );
    return std::make_pair( ret.second, 0 );
}

auto MapCacheBase::get( std::string_view key ) const -> std::string_view
{
    auto it = mStrings.find( std::string{ key } );
    if ( it == mStrings.end() )
    {
        return {};
    }
    return it->second;
}

auto MapCacheBase::getVector( std::string_view key ) const -> std::vector<std::string_view>
{
    auto it = mStringLists.find( std::string{ key } );
    if ( it == mStringLists.end() )
    {
        return std::vector<std::string_view>{};
    }

    std::vector<std::string_view> result;
    std::copy( it->second.begin(), it->second.end(), std::back_inserter( result ) );

    return result;
}

auto MapCacheBase::contains( std::string_view key ) const -> bool
{
    return mStrings.contains( std::string{ key } ) || mStringLists.contains( std::string{ key } );
}

auto MapCacheBase::output( std::ostream & output ) const -> void
{
    for ( const auto & entry : mStrings )
    {
        output.write( entry.first.data(), static_cast<std::streamsize>( entry.first.size() ) );
        output.write( "=", 1 );
        output.write( entry.second.data(), static_cast<std::streamsize>( entry.second.size() ) );
        output.write( "\n", 1 );
    }

    for ( const auto & entry : mStringLists )
    {
        output.write( entry.first.data(), static_cast<std::streamsize>( entry.first.size() ) );
        output.write( "=", 1 );
        std::string prefix;
        for ( const auto & str : entry.second )
        {
            output.write( prefix.data(), static_cast<std::streamsize>( prefix.size() ) );
            output.write( str.data(), static_cast<std::streamsize>( str.size() ) );
            prefix = "|";
        }
        output.write( "\n", 1 );
    }
}
