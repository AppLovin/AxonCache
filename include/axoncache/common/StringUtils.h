// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <system_error>
#include <charconv>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <sstream>

namespace axoncache
{

class StringUtils
{
  public:
    static inline std::string lowercase( std::string_view str )
    {
        std::string result = std::string( str );
        std::transform( result.begin(), result.end(), result.begin(), ::tolower );
        return result;
    }

    static inline bool toBool( std::string_view str )
    {
        if ( ( StringUtils::lowercase( str ).compare( "true" ) == 0 ) || str.compare( "1" ) == 0 )
        {
            return true;
        }

        return false;
    }

    static int64_t toLong( std::string_view str )
    {
        int64_t val = 0;
        if ( auto [p, ec] = std::from_chars( str.data(), str.data() + str.size(), val );
             ec == std::errc() )
        {
            return val;
        }

        return 0;
    }

    static int32_t toInteger( std::string_view str )
    {
        int32_t val = 0;
        if ( auto [p, ec] = std::from_chars( str.data(), str.data() + str.size(), val );
             ec == std::errc() )
        {
            return val;
        }

        return 0;
    }

    static inline double toDouble( std::string_view str )
    {
        if ( str.empty() )
            return 0;

        char * end = ( char * )( str.data() + str.size() );
        char * endResult = end;
        double result = strtod( str.data(), &endResult );

        // If every character in the string is valid
        if ( end == endResult )
        {
            return result;
        }
        //Unsafe, never throws any exceptions
        return 0;
    }

    static inline std::string trim( std::string_view string )
    {
        const std::string kWhitespaces = std::string( " \t\f\v\n\r" );

        std::string result = std::string( string );

        std::string::size_type pos = result.find_last_not_of( kWhitespaces );
        if ( pos != std::string::npos )
        {
            result.erase( pos + 1 );
            pos = result.find_first_not_of( kWhitespaces );
            if ( pos != std::string::npos )
                result.erase( 0, pos );
        }
        else
        {
            result.erase( result.begin(), result.end() );
        }

        return result;
    }

    static inline std::vector<std::string> splitFirstOccurence( const char delim, const std::string & string )
    {
        std::vector<std::string> result;

        std::string::size_type pos = string.find( delim, 0 );

        if ( pos == std::string::npos )
        {
            result.push_back( string );
            return result;
        }

        result.push_back( string.substr( 0, pos ) );
        result.push_back( string.substr( pos + 1 ) );

        return result;
    }

    template<typename T>
    static inline auto split( const char delim, const T & string ) -> std::vector<std::string>
    {
        std::vector<std::string> result = std::vector<std::string>();

        bool inSpace = false;
        bool inToken = false;

        char const * pTokenBegin = nullptr; // Init to satisfy compiler.
        for ( const auto & elem : string )
        {
            // See if we are in a space or token
            inSpace = ( elem == delim );

            // If we are in a space
            if ( inSpace )
            {
                // If we have a valid token
                if ( inToken )
                {
                    // Save the whole token
                    result.push_back( std::string( pTokenBegin, &elem - pTokenBegin ) );
                }
                else
                {
                    // we are in
                    result.push_back( "" );
                }

                // We are not in a token
                inToken = false;
            }
            else
            {
                // If we were not in the token
                if ( !inToken )
                {
                    // Move the token pointer to the start
                    pTokenBegin = &elem;
                }

                // We are back in the token
                inToken = true;
            }
        }

        // If we have a remaining token
        if ( inToken )
        {
            // Add it to the result
            result.push_back( std::string( pTokenBegin, ( string.data() + string.size() ) - pTokenBegin ) );
        }

        result.resize( result.size() );
        return result;
    }

    template<typename T>
    static inline auto splitStringView( const char delim, const T & string ) -> std::vector<std::string_view>
    {
        std::vector<std::string_view> result;

        bool inSpace = false;
        bool inToken = false;

        char const * pTokenBegin = nullptr; // Init to satisfy compiler.
        for ( const auto & elem : string )
        {
            // See if we are in a space or token
            inSpace = ( elem == delim );

            // If we are in a space
            if ( inSpace )
            {
                // If we have a valid token
                if ( inToken )
                {
                    // Save the whole token
                    result.push_back( std::string_view( pTokenBegin, &elem - pTokenBegin ) );
                }
                else
                {
                    // we are in
                    result.push_back( "" );
                }

                // We are not in a token
                inToken = false;
            }
            else
            {
                // If we were not in the token
                if ( !inToken )
                {
                    // Move the token pointer to the start
                    pTokenBegin = &elem;
                }

                // We are back in the token
                inToken = true;
            }
        }

        // If we have a remaining token
        if ( inToken )
        {
            // Add it to the result
            result.push_back( std::string_view( pTokenBegin, ( string.data() + string.size() ) - pTokenBegin ) );
        }

        return result;
    }

    [[maybe_unused]] static auto join( char separator, const std::vector<std::string> & parts ) -> std::string
    {
        if ( parts.empty() )
        {
            return "";
        }

        std::ostringstream oss;
        auto iter = parts.begin();
        oss << *iter; // first element, no separator
        ++iter;

        for ( ; iter != parts.end(); ++iter )
        {
            oss << separator << *iter;
        }

        return oss.str();
    }
};

}
