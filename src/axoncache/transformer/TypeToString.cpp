// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//
// Created by Anthony Alayo on 10/28/23.
//

#include "axoncache/transformer/TypeToString.h"
#include "axoncache/logger/Logger.h"
#include <cstring>
#include <vector>
#include <span>
#include <sstream>

namespace axoncache
{
template<typename T>
auto transform( const T & input ) -> std::string
{
    return std::string{ ( const char * )&input, sizeof( T ) };
}

template<>
auto transform<std::vector<float>>( const std::vector<float> & input ) -> std::string
{
    return std::string{ ( const char * )input.data(), sizeof( float ) * input.size() };
}

template<typename T>
auto transform( std::string_view input ) -> T
{
    if ( input.size() != sizeof( T ) )
    {
        std::ostringstream oss;
        oss << "Data size " << input.size()
            << " doesn't match with type size " << sizeof( T );
        AL_LOG_ERROR( oss.str() );

        return T{};
    }
    return *( ( T * )input.data() );
}

template<>
auto transform<std::vector<float>>( std::string_view input ) -> std::vector<float>
{
    size_t outputSize = input.size() / sizeof( float );
    if ( input.size() % sizeof( float ) != 0 )
    {
        std::ostringstream oss;
        oss << "Data size " << input.size()
            << " doesn't match with type float vector size "
            << outputSize * sizeof( float );
        AL_LOG_ERROR( oss.str() );

        return {};
    }
    std::vector<float> result( outputSize );
    auto * buffer = ( char * )result.data();
    memcpy( buffer, input.data(), input.size() );
    return result;
}

template<>
auto transform<std::span<const float>>( std::string_view input ) -> std::span<const float>
{
    auto outputSize = input.size() / sizeof( float );
    if ( input.size() % sizeof( float ) != 0 )
    {
        std::ostringstream oss;
        oss << "Data size " << input.size()
            << " doesn't match with type float span size "
            << outputSize * sizeof( float );
        AL_LOG_ERROR( oss.str() );

        return {};
    }
    auto data = reinterpret_cast<const float *>( input.data() );
    return { data, outputSize };
}

template auto transform( const bool & input ) -> std::string;
template auto transform( const int & input ) -> std::string;
template auto transform( const int64_t & input ) -> std::string;
template auto transform( const double & input ) -> std::string;
template auto transform( const float & input ) -> std::string;

template auto transform<bool>( std::string_view input ) -> bool;
template auto transform<int>( std::string_view input ) -> int;
template auto transform<int64_t>( std::string_view input ) -> int64_t;
template auto transform<double>( std::string_view input ) -> double;
template auto transform<float>( std::string_view input ) -> float;
}
