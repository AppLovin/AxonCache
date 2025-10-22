// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string>
#include <string_view>
#include <axoncache/transformer/TypeToString.h>
#include "doctest/doctest.h"
#include <stdint.h>
#include <vector>
using namespace axoncache;

TEST_CASE( "TypeToStringTest" )
{
    {
        bool value = true;
        auto transformed = transform<bool>( value );
        CHECK_EQ( value, transform<bool>( std::string_view{ transformed } ) );
        CHECK_EQ( transformed.size(), 1 );
        CHECK_EQ( transformed[0], static_cast<char>( 0x01 ) );
        CHECK_EQ( transform<int64_t>( std::string_view{ transformed } ), int64_t{} );
    }
    {
        int64_t value = 0x4142434461626364; // ABCDabcd
        auto transformed = transform<int64_t>( value );
        CHECK_EQ( value, transform<int64_t>( std::string_view{ transformed } ) );
        CHECK_EQ( transformed.size(), 8 );
        CHECK_EQ( transformed, "dcbaDCBA" ); // little-endian
        CHECK_EQ( transform<bool>( std::string_view{ transformed } ), bool{} );
    }
    {
        float value = 1.0f; // 0x3f800000
        auto transformed = transform<float>( value );
        CHECK_EQ( value, transform<float>( std::string_view{ transformed } ) );
        CHECK_EQ( transformed.size(), 4 );
        CHECK_EQ( transformed[3], static_cast<char>( 0x3f ) );
        CHECK_EQ( transformed[2], static_cast<char>( 0x80 ) );
        CHECK_EQ( transformed[1], static_cast<char>( 0x00 ) );
        CHECK_EQ( transformed[0], static_cast<char>( 0x00 ) );
        CHECK_EQ( transform<double>( std::string_view{ transformed } ), double{} );
    }
    {
        double value = 1.0; // 0x3ff0000000000000
        auto transformed = transform<double>( value );
        CHECK_EQ( value, transform<double>( std::string_view{ transformed } ) );
        CHECK_EQ( transformed.size(), 8 );
        CHECK_EQ( transformed[7], static_cast<char>( 0x3f ) );
        CHECK_EQ( transformed[6], static_cast<char>( 0xf0 ) );
        CHECK_EQ( transformed[5], static_cast<char>( 0x00 ) );
        CHECK_EQ( transformed[4], static_cast<char>( 0x00 ) );
        CHECK_EQ( transformed[3], static_cast<char>( 0x00 ) );
        CHECK_EQ( transformed[2], static_cast<char>( 0x00 ) );
        CHECK_EQ( transformed[1], static_cast<char>( 0x00 ) );
        CHECK_EQ( transformed[0], static_cast<char>( 0x00 ) );
        CHECK_EQ( transform<float>( std::string_view{ transformed } ), float{} );
    }
    {
        int32_t value = 0x41424344; // ABCD
        auto transformed = transform<int32_t>( value );
        CHECK_EQ( value, transform<int32_t>( std::string_view{ transformed } ) );
        CHECK_EQ( transformed.size(), 4 );
        CHECK_EQ( transformed, "DCBA" ); // little-endian
        CHECK_EQ( transform<double>( std::string_view{ transformed } ), double{} );
    }
    {
        std::vector<float> value( { 1.0, 2.0, 2.5 } );
        auto transformed = transform<std::vector<float>>( value );
        CHECK_EQ( transformed.size(), value.size() * sizeof( float ) );
        auto readValue = transform<std::vector<float>>( std::string_view{ transformed } );
        CHECK_EQ( readValue.size(), value.size() );
        for ( size_t index = 0; index < value.size(); index++ )
        {
            CHECK_EQ( value[index], readValue[index] );
        }
    }
}
