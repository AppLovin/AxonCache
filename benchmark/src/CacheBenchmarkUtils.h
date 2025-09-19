// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <map>
#include <vector>

namespace axoncache::benchmark_utils
{
static auto gen_random( const int len ) -> std::string
{
    std::string tmp_s;
    tmp_s.reserve( len );

    for ( int i = 0; i < len; ++i )
    {
        tmp_s += ( char )( rand() % 255 ); // NOLINT
    }

    return tmp_s;
}

static std::string gen_alpha_numeric_random( const int len )
{
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve( len );

    for ( int i = 0; i < len; ++i )
    {
        tmp_s += alphanum[rand() % ( sizeof( alphanum ) - 1 )];
    }

    return tmp_s;
}

[[maybe_unused]] static auto gen_random_str_map_alpha_numeric( const uint64_t numberOfElements )
{
    std::map<std::string, std::string> strs;
    while ( strs.size() < numberOfElements )
    {
        std::string key;
        do
        {
            key = gen_alpha_numeric_random( ( rand() % 10 ) + 10 );
        } while ( strs.find( key ) != strs.end() );

        strs[key] = gen_alpha_numeric_random( ( rand() % 10 ) + 10 ); // NOLINT
    }

    return strs;
}

[[maybe_unused]] static auto gen_random_str_map( const uint64_t numberOfElements )
{
    std::map<std::string, std::string> strs;
    while ( strs.size() < numberOfElements )
    {
        std::string key;
        do
        {
            key = gen_random( ( rand() % 10 ) + 10 );
        } while ( strs.find( key ) != strs.end() );

        strs[key] = gen_random( ( rand() % 10 ) + 10 ); // NOLINT
    }

    return strs;
}

[[maybe_unused]] static auto gen_random_str_vec_map_alpha_numeric( const uint64_t numberOfElements )
{
    std::map<std::string, std::vector<std::string>> strs;
    while ( strs.size() < numberOfElements )
    {
        std::string key;
        do
        {
            key = gen_alpha_numeric_random( ( rand() % 10 ) + 10 );
        } while ( strs.find( key ) != strs.end() );

        std::vector<std::string> vec;
        for ( auto numberOfVals = rand() % 10; numberOfVals > 0; --numberOfVals )
        {
            vec.emplace_back( gen_alpha_numeric_random( ( rand() % 10 ) + 10 ) );
        }
        strs[key] = vec; // NOLINT
    }

    return strs;
}

[[maybe_unused]] static auto gen_random_str_vec_map( const uint64_t numberOfElements )
{
    std::map<std::string, std::vector<std::string>> strs;
    while ( strs.size() < numberOfElements )
    {
        std::string key;
        do
        {
            key = gen_random( ( rand() % 10 ) + 10 );
        } while ( strs.find( key ) != strs.end() );

        std::vector<std::string> vec;
        for ( auto numberOfVals = rand() % 10; numberOfVals > 0; --numberOfVals )
        {
            vec.emplace_back( gen_random( ( rand() % 10 ) + 10 ) );
        }
        strs[key] = vec; // NOLINT
    }

    return strs;
}
}
