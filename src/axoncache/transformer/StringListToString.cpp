// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/transformer/StringListToString.h"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sstream>
#include "axoncache/Constants.h"
#include "axoncache/logger/Logger.h"

using namespace axoncache;

auto StringListToString::transform( const std::vector<std::string_view> & input ) -> std::string
{
    std::string result;

    if ( input.size() > Constants::Limit::kVectorLength )
    {
        std::ostringstream oss;
        oss << "input vector size " << input.size()
            << " is too large. max=" << Constants::Limit::kVectorLength;
        AL_LOG_ERROR( oss.str() );

        throw std::runtime_error( "input vector size " + std::to_string( input.size() ) + " too large. max=" + std::to_string( Constants::Limit::kVectorLength ) );
    }

    auto numberOfElements = static_cast<uint16_t>( input.size() );
    result.append( ( const char * )( &numberOfElements ), sizeof( uint16_t ) );

    for ( const auto & s : input )
    {
        if ( s.size() > Constants::Limit::kVectorElementLength )
        {
            std::ostringstream oss;
            oss << "input vector element is too large " << s.size()
                << ". max=" << Constants::Limit::kVectorElementLength;
            AL_LOG_ERROR( oss.str() );

            throw std::runtime_error( "input vector element " + std::to_string( s.size() ) + " too large. max=" + std::to_string( Constants::Limit::kVectorElementLength ) );
        }

        auto elemLength = static_cast<uint16_t>( s.size() );
        result.append( ( const char * )( &elemLength ), sizeof( uint16_t ) );
        result.append( s.data(), s.size() );
        result.push_back( '\0' ); // Always null terminated
    }

    return result;
}

auto StringListToString::transform( std::string_view input ) -> std::vector<std::string_view>
{
    const auto numberOfElements = *( ( uint16_t * )input.data() );
    std::vector<std::string_view> result;
    result.reserve( numberOfElements );

    const auto * dataPtr = input.data() + sizeof( uint16_t );
    for ( auto elemIx = 0UL; elemIx < numberOfElements; ++elemIx )
    {
        auto elemLength = *( ( uint16_t * )( dataPtr ) );
        dataPtr += sizeof( uint16_t );
        result.emplace_back( dataPtr, elemLength );
        dataPtr += elemLength + 1;
    }

    return result;
}
