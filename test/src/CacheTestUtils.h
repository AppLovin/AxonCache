// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <filesystem>

namespace axoncache::test_utils
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

[[maybe_unused]] static auto gen_random_str_map_alpha_numeric( const uint64_t numberOfElements, const uint64_t numberOfValues )
{
    if ( numberOfValues == 0UL )
    {
        return gen_random_str_map_alpha_numeric( numberOfElements );
    }
    std::vector<std::string> values;
    while ( values.size() < numberOfValues )
    {
        values.emplace_back( gen_alpha_numeric_random( ( rand() % 10 ) + 10 ) );
    }
    std::map<std::string, std::string> strs;
    while ( strs.size() < numberOfElements )
    {
        std::string key;
        do
        {
            key = gen_alpha_numeric_random( ( rand() % 10 ) + 10 );
        } while ( strs.find( key ) != strs.end() );

        strs[key] = values[rand() % numberOfValues]; // NOLINT
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

[[maybe_unused]] static auto gen_random_str_vec_map_alpha_numeric( const uint64_t numberOfElements, const uint64_t numberOfValues )
{
    if ( numberOfValues == 0UL )
    {
        return gen_random_str_vec_map_alpha_numeric( numberOfElements );
    }

    std::vector<std::vector<std::string>> values;
    while ( values.size() < numberOfValues )
    {
        std::vector<std::string> vec;
        for ( auto numberOfVals = rand() % 10; numberOfVals > 0; --numberOfVals )
        {
            vec.emplace_back( gen_alpha_numeric_random( ( rand() % 10 ) + 10 ) );
        }
        values.emplace_back( vec );
    }

    std::map<std::string, std::vector<std::string>> strs;
    while ( strs.size() < numberOfElements )
    {
        std::string key;
        do
        {
            key = gen_alpha_numeric_random( ( rand() % 10 ) + 10 );
        } while ( strs.find( key ) != strs.end() );

        strs[key] = values[rand() % numberOfValues]; // NOLINT
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

[[maybe_unused]] static auto gen_random_str_vec_map( const uint64_t numberOfElements, uint32_t numberOfVals )
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
        while ( numberOfVals != 0 )
        {
            numberOfVals--;
            vec.emplace_back( gen_random( ( rand() % 10 ) + 10 ) );
        }
        strs[key] = vec; // NOLINT
    }

    return strs;
}

[[maybe_unused]] static auto gen_random_bool_map( const uint64_t numberOfElements, std::set<std::string> & keys )
{
    std::map<std::string, bool> retVal;
    while ( retVal.size() < numberOfElements )
    {
        std::string key = gen_random( ( rand() % 10 ) + 10 );
        bool value = rand() & 1; // NOLINT
        if ( keys.insert( key ).second )
        {
            retVal[key] = value;
        }
    }

    return retVal;
}

[[maybe_unused]] static auto gen_random_int_map( const uint64_t numberOfElements, std::set<std::string> & keys )
{
    std::map<std::string, int32_t> retVal;
    while ( retVal.size() < numberOfElements )
    {
        std::string key = gen_random( ( rand() % 10 ) + 10 );
        int32_t value = rand(); // NOLINT
        if ( keys.insert( key ).second )
        {
            retVal[key] = value;
        }
    }

    return retVal;
}

[[maybe_unused]] static auto gen_random_double_map( const uint64_t numberOfElements, std::set<std::string> & keys )
{
    std::map<std::string, double> retVal;
    while ( retVal.size() < numberOfElements )
    {
        std::string key = gen_random( ( rand() % 10 ) + 10 );
        double value = ( rand() / ( double )RAND_MAX - 0.5 ) * ( rand() % 1000 ); // NOLINT
        if ( keys.insert( key ).second )
        {
            retVal[key] = value;
        }
    }

    return retVal;
}

[[maybe_unused]] static auto gen_random_int64_map( const uint64_t numberOfElements, std::set<std::string> & keys )
{
    std::map<std::string, int64_t> retVal;
    while ( retVal.size() < numberOfElements )
    {
        std::string key = gen_random( ( rand() % 10 ) + 10 );
        int64_t value = rand(); // NOLINT
        if ( keys.insert( key ).second )
        {
            retVal[key] = value;
        }
    }

    return retVal;
}

[[maybe_unused]] static auto resolveResourcePath( const char * relativePath, const char * currentFile ) -> std::string
{
    const std::string sourcePath( currentFile );
    const auto & parentPath = sourcePath.substr( 0, sourcePath.find_last_of( '/' ) );
    auto resourcePath = parentPath + "/" + relativePath;

    std::filesystem::path path( resourcePath );

    if ( !std::filesystem::exists( path ) )
    {
        throw std::runtime_error( "File not found: " + resourcePath );
    }

    return resourcePath;
}

}
