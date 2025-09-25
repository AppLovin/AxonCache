// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <axoncache/logger/Logger.h>
#include <axoncache/CacheGenerator.h>
#include <axoncache/cache/CacheType.h>
#include <axoncache/version.h>
#include <axoncache/Constants.h>
#include <axoncache/loader/CacheOneTimeLoader.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/cache/MapCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/writer/CacheFileWriter.h"
#include "axoncache/common/StringViewUtils.h"

#include <cxxopts.hpp>
#include <spdlog/spdlog.h>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <sstream>

auto readMode( axoncache::SharedSettingsProvider * settings, const cxxopts::ParseResult & result ) -> void;
auto writeMode( axoncache::SharedSettingsProvider * settings, const cxxopts::ParseResult & result ) -> void;

auto benchModeUnorderedMap(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void;
auto benchModeAxonCache(
    int numKeys,
    std::vector<std::string> keys,
    std::vector<std::string> vals ) -> void;

enum class Command
{
    GET,
    GETFLOATVECTOR,
    GETFLOATSPAN,
    GETFLOATATINDEX,
    GETBOOL,
    GETVECTOR,
    GETINTEGER,
    GETLONG,
    GETINT64,
    GETDOUBLE
};

void handleGet(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto value = cache->get( key );
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

void handleGetBool(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto value = cache->getBool( key ).first;
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

void handleGetInteger(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto value = static_cast<int32_t>( cache->getInt64( key ).first );
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

void handleGetLong(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto value = cache->getInt64( key ).first;
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

void handleGetInt64(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto value = cache->getInt64( key ).first;
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

void handleGetDouble(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto value = cache->getDouble( key ).first;
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

void handleGetVector(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto vec = cache->getVector( key );
    if ( !quiet )
    {
        for ( auto val : vec )
        {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}

void handleGetFloatVector(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto vec = cache->getFloatVector( key );
    if ( !quiet )
    {
        for ( auto val : vec )
        {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}

void handleGetFloatSpan(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    bool quiet )
{
    const auto vec = cache->getFloatSpan( key );
    if ( !quiet )
    {
        for ( auto val : vec )
        {
            std::cout << val << " ";
        }
        std::cout << "\n";
    }
}

void handleGetFloatAtIndex(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    const std::string & key,
    int index,
    bool quiet )
{
    const auto value = cache->getFloatAtIndex( key, index );
    if ( !quiet )
    {
        std::cout << value << "\n";
    }
}

auto parseLine( const std::string & line ) -> std::optional<std::pair<Command, std::string>>
{
    std::istringstream iss( line );
    std::string command;
    iss >> command; // Read the first token (the command)

    if ( command == "GET" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GET, key );
        }
        else
        {
            AL_LOG_ERROR( "GET command expects a key argument" );
        }
    }
    else if ( command == "GETBOOL" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETBOOL, key );
        }
        else
        {
            AL_LOG_ERROR( "GETBOOL command expects a key argument" );
        }
    }
    else if ( command == "GETVECTOR" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETVECTOR, key );
        }
        else
        {
            AL_LOG_ERROR( "GETVECTOR command expects a key argument" );
        }
    }
    else if ( command == "GETFLOATVECTOR" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETFLOATVECTOR, key );
        }
        else
        {
            AL_LOG_ERROR( "GETFLOATVECTOR command expects a key argument" );
        }
    }
    else if ( command == "GETFLOATSPAN" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETFLOATSPAN, key );
        }
        else
        {
            AL_LOG_ERROR( "GETFLOATSPAN command expects a key argument" );
        }
    }
    else if ( command == "GETINTEGER" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETINTEGER, key );
        }
        else
        {
            AL_LOG_ERROR( "GETINTEGER command expects a key argument" );
        }
    }
    else if ( command == "GETLONG" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETLONG, key );
        }
        else
        {
            AL_LOG_ERROR( "GETLONG command expects a key argument" );
        }
    }
    else if ( command == "GETINT64" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETINT64, key );
        }
        else
        {
            AL_LOG_ERROR( "GETINT64 command expects a key argument" );
        }
    }
    else if ( command == "GETDOUBLE" )
    {
        std::string key;
        iss >> key; // Expect one key argument for GET
        if ( !key.empty() )
        {
            return std::make_pair( Command::GETDOUBLE, key );
        }
        else
        {
            AL_LOG_ERROR( "GETDOUBLE command expects a key argument" );
        }
    }
    else if ( command == "GETFLOATATINDEX" )
    {
        std::string key;
        int index = -1;
        iss >> key >> index; // Expect one key and an index for GET_MANY
        if ( !key.empty() && index >= 0 )
        {
            return std::make_pair( Command::GETFLOATATINDEX, key );
        }
        else
        {
            AL_LOG_ERROR( "GETFLOATATINDEX command expects a key argument and an index" );
        }
    }
    else
    {
        AL_LOG_ERROR( "Error: Unknown command: " + command );
    }

    return std::nullopt;
}

void executeCommand(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    Command command,
    const std::string & key,
    bool quiet,
    bool printKey )
{
    if ( printKey )
    {
        std::cout << key << " ";
    }

    if ( command == Command::GET )
    {
        handleGet( cache, key, quiet );
    }
    else if ( command == Command::GET )
    {
        handleGetBool( cache, key, quiet );
    }
    else if ( command == Command::GETVECTOR )
    {
        handleGetVector( cache, key, quiet );
    }
    else if ( command == Command::GETINTEGER )
    {
        handleGetInteger( cache, key, quiet );
    }
    else if ( command == Command::GETLONG )
    {
        handleGetLong( cache, key, quiet );
    }
    else if ( command == Command::GETINT64 )
    {
        handleGetInt64( cache, key, quiet );
    }
    else if ( command == Command::GETDOUBLE )
    {
        handleGetDouble( cache, key, quiet );
    }
    else if ( command == Command::GETFLOATVECTOR )
    {
        handleGetFloatVector( cache, key, quiet );
    }
    else if ( command == Command::GETFLOATSPAN )
    {
        handleGetFloatSpan( cache, key, quiet );
    }
    else if ( command == Command::GETFLOATATINDEX )
    {
        auto index = 0; // HACK
        handleGetFloatAtIndex( cache, key, index, quiet );
    }
}

// Function to read input from stdin
void parseInputFromStdin(
    const std::shared_ptr<axoncache::LinearProbeDedupCache> & cache,
    bool quiet,
    int repeat,
    bool printKey )
{
    std::vector<std::pair<Command, std::string>> commands;

    std::string line;
    while ( std::getline( std::cin, line ) )
    {
        // Parse each line from stdin
        auto result = parseLine( line );
        if ( result.has_value() )
        {
            commands.push_back( result.value() );
        }
    }

    for ( int idx = 0; idx < repeat; ++idx )
    {
        auto start = std::chrono::high_resolution_clock::now();

        for ( auto && [command, key] : commands )
        {
            executeCommand( cache, command, key, quiet, printKey );
        }

        // Record end time
        auto end = std::chrono::high_resolution_clock::now();

        // Calculate the duration
        std::chrono::duration<double, std::milli> duration = end - start;
        std::chrono::duration<double, std::micro> durationMicros = end - start;
        std::chrono::duration<double> durationSeconds = end - start;

        // Print the duration of the command execution
        auto commandsCount = static_cast<int>( commands.size() );
        auto qps = static_cast<int>( commandsCount / durationSeconds.count() );
        auto avgQuerySpeed = durationMicros.count() / commandsCount;

        std::ostringstream oss;
        oss << "Execution time: " << duration.count() << " ms "
            << "#commands " << commandsCount << " "
            << "qps, " << qps << " "
            << "avg query speed {} " << avgQuerySpeed << " us";
        // Print to stderr so that we can redirect the output to /dev/null
        std::cerr << oss.str() << "\n";
    }
}

auto loadCache( axoncache::SharedSettingsProvider * settings, const std::string & cacheName, const cxxopts::ParseResult & result ) -> void
{
    axoncache::CacheOneTimeLoader loader( settings );
    const auto isPreloadMemoryEnabled = false;

    // if we specify the absolute path, use it:
    auto cacheAbsolutePath = result["abspath"].as<std::string>();
    if ( cacheAbsolutePath.empty() )
    {
        if ( result["latest"].as<bool>() )
        {
            cacheAbsolutePath = loader.getLatestTimestampFullCacheFileName( cacheName );
        }
        else
        {
            cacheAbsolutePath = loader.getFullCacheFileName( cacheName );
        }
    }

    const auto cache = loader.loadAbsolutePath<axoncache::LinearProbeDedupCache>( cacheName, cacheAbsolutePath, isPreloadMemoryEnabled );

    auto quiet = result["quiet"].as<bool>();
    auto repeat = result["repeat"].as<int>();
    auto printKey = result["print_key"].as<bool>();
    parseInputFromStdin( cache, quiet, repeat, printKey );
}

auto readMode( axoncache::SharedSettingsProvider * settings, const cxxopts::ParseResult & result ) -> void
{
    auto cacheName = result["load"].as<std::string>();
    settings->setIfNotSet( axoncache::Constants::ConfKey::kCacheType + "." + cacheName, result["type"].as<std::string>() );
    settings->setIfNotSet( axoncache::Constants::ConfKey::kLoadDir + "." + cacheName, result["load_dir"].as<std::string>() );

    loadCache( settings, cacheName, result );
}

auto writeMode( axoncache::SharedSettingsProvider * settings, const cxxopts::ParseResult & result ) -> void
{
    auto cacheName = result["name"].as<std::string>();

    settings->setIfNotSet( axoncache::Constants::ConfKey::kCacheNames, cacheName ); // command line only supports one cache
    settings->setIfNotSet( axoncache::Constants::ConfKey::kCacheType + "." + cacheName, result["type"].as<std::string>() );
    settings->setIfNotSet( axoncache::Constants::ConfKey::kInputFiles + "." + cacheName, result["input"].as<std::string>() );
    settings->setIfNotSet( axoncache::Constants::ConfKey::kOutputDir + "." + cacheName, result["output_dir"].as<std::string>() );

    axoncache::CacheGenerator alCache( settings );
    alCache.start();
}

auto createMode( axoncache::SharedSettingsProvider * settings, const cxxopts::ParseResult & result ) -> void
{
    auto fileName = result["create"].as<std::string>();
    auto cacheName = result["name"].as<std::string>();
    auto outputDir = result["output_dir"].as<std::string>();
    auto quiet = result["quiet"].as<bool>();
    if ( cacheName.empty() || fileName.empty() || outputDir.empty() )
    {
        std::cerr << "Some parameters missing. Example: --create sample_input_file.txt  --slot 10000 --name test_cache --output_dir /tmp/\n";
        return;
    }
    auto numberOfKeysSlots = result["slot"].as<int>();
    const auto maxLoadFactor = 0.5f;
    const uint16_t offsetBits = 35U;
    auto memoryHandler = std::make_unique<axoncache::MallocMemoryHandler>( numberOfKeysSlots * sizeof( uint64_t ) );
    auto cache = std::make_unique<axoncache::LinearProbeDedupCache>( offsetBits, numberOfKeysSlots, maxLoadFactor, std::move( memoryHandler ), axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED );
    std::ifstream inputFile;
    inputFile.open( fileName, std::ios::binary );
    if ( !inputFile.is_open() )
    {
        std::cerr << "Failed to open file: " << fileName << '\n';
        return;
    }
    std::string line;
    std::map<std::string, std::string> queryIdToType;
    int lineNumber = 0;
    while ( std::getline( inputFile, line, '\n' ) )
    {
        lineNumber++;
        if ( line.empty() )
        {
            continue;
        }
        if ( lineNumber == 1 )
        {
            auto items = axoncache::StringUtils::split( '|', line );
            for ( auto & item : items )
            {
                auto subitems = axoncache::StringUtils::split( '=', item );
                if ( subitems.size() != 2 )
                {
                    std::cerr << "Wrong format in first line of the file: " << fileName << " example: \"992=Bool|267=String|1401=Double|999=StringList|1111=FloatList\"\n";
                    return;
                }
                queryIdToType[axoncache::StringUtils::trim( subitems[0] )] = axoncache::StringUtils::trim( subitems[1] );
            }
        }
        else
        {
            auto items = axoncache::StringUtils::split( '=', line );
            if ( items.size() != 2 )
            {
                std::cerr << "Wrong format in " << lineNumber << "-th line." << fileName << " example: \"111.key=value or 222.key=value1|value2|value3 or 333.key=1.0:2.0\"\n";
                return;
            }
            auto key = axoncache::StringUtils::trim( items[0] );
            auto value = axoncache::StringUtils::trim( items[1] );
            auto queryId = key.substr( 0, key.find( '.' ) );
            auto type = queryIdToType.contains( queryId ) ? queryIdToType[queryId] : "String";
            if ( type == "String" )
            {
                cache->put( key, value );
            }
            else if ( type == "StringList" )
            {
                auto values = axoncache::StringUtils::splitStringView( '|', value );
                cache->put( key, values );
            }
            else if ( type == "Bool" )
            {
                bool boolValue = axoncache::StringUtils::toBool( value );
                cache->put( key, boolValue );
            }
            else if ( type == "Int64" )
            {
                int64_t intValue = axoncache::StringUtils::toLong( value );
                cache->put( key, intValue );
            }
            else if ( type == "Double" )
            {
                double doubleValue = axoncache::StringUtils::toDouble( value );
                cache->put( key, doubleValue );
            }
            else if ( type == "FloatList" )
            {
                auto values = axoncache::stringViewToVector<float>( value, ':', value.size() );
                cache->put( key, values );
            }
            else
            {
                std::cerr << "Unknown type (" << type << ") in " << lineNumber << "th line, skipping\n";
            }
        }
    }
    auto writer = std::make_unique<axoncache::CacheFileWriter>( outputDir, cacheName, cache.get() );
    writer->write();
}

auto main( int argc, char ** argv ) -> int
{
    cxxopts::Options options( *argv, "Generate cache files" );

    // clang-format off
    options.add_options() // try to keep the flags in alphabetical order
        ("a,abspath", "Absolute path to the cache file to load", cxxopts::value<std::string>()->default_value( "" ) )
        ("c,config", "location of the config", cxxopts::value<std::string>()->default_value( axoncache::Constants::ConfDefault::kConfigLocation.data() ) )
        ("d,debug", "Set debug log level")
        ("h,help", "Show help")
        ("i,input", "Comma separated list of files to read from", cxxopts::value<std::string>()->default_value( "" ) )
        ("l,load", "Name of cache to try to load", cxxopts::value<std::string>()->default_value( "" ) )
        ("m,latest", "load cache with latest timestamp" )
        ("n,name", "Name of the cache", cxxopts::value<std::string>()->default_value( "axoncache" ) )
        ("o,output_dir", "Output directory for cache", cxxopts::value<std::string>()->default_value( axoncache::Constants::ConfDefault::kOutputDir.data() ) )
        ("r,load_dir", "location of cache file to load", cxxopts::value<std::string>()->default_value( axoncache::Constants::ConfDefault::kLoadDir.data() ) )
        ("t,type", "Cache type", cxxopts::value<std::string>()->default_value( std::to_string( static_cast<uint32_t>( axoncache::CacheType::BUCKET_CHAIN ) ) ) )
        ("C,repeat", "How many time to repeat running the commands", cxxopts::value<int>()->default_value( "1" ) )
        ("v,version", "Print the current version number")
        ("q,quiet", "Quiet mode")
        ("k,print_key", "Print keys")
        ("g,create", "Create cache from file", cxxopts::value<std::string>()->default_value( "" ))
        ("s,slot", "Number of key slots", cxxopts::value<int>()->default_value( "10000" ))
        ("b,bench", "Run in benchmark mode", cxxopts::value<bool>()->default_value( "false" ));
    // clang-format on

    try
    {
        auto result = options.parse( argc, argv );

        if ( result["help"].as<bool>() )
        {
            std::cout << options.help() << "\n";
            return 0;
        }

        if ( result["version"].as<bool>() )
        {
            std::cout << "AxonCache, version " << AXONCACHE_VERSION << "\n";
            return 0;
        }

        if ( result["input"].count() == 0 && result["load"].count() == 0 && result["abspath"].count() == 0 && result["create"].count() == 0 && result["bench"].count() == 0 )
        {
            std::cerr << "Either input, name, bench, abspath or create is required" << "\n";
            std::cout << options.help() << "\n";
            return 1;
        }

        axoncache::SharedSettingsProvider settings( axoncache::Constants::ConfDefault::kConfigLocation.data() );

        auto alcacheLogger = []( const char * msg, const axoncache::LogLevel & level )
        {
            switch ( level )
            {
                case axoncache::LogLevel::INFO:
                    SPDLOG_INFO( msg );
                    break;
                case axoncache::LogLevel::WARNING:
                    SPDLOG_WARN( msg );
                    break;
                case axoncache::LogLevel::ERROR:
                    SPDLOG_ERROR( msg );
                    break;
            }
        };
        axoncache::Logger::setLogFunction( alcacheLogger );
        spdlog::set_level( spdlog::level::info );

        if ( result["input"].count() > 0 )
        {
            writeMode( &settings, result );
        }
        else if ( result["create"].count() > 0 )
        {
            createMode( &settings, result );
        }
        else if ( result["bench"].count() > 0 )
        {
            const int numKeys = 1 * 1000 * 1000;
            std::vector<std::string> keys;
            std::vector<std::string> vals;
            for ( int idx = 0; idx < numKeys; ++idx )
            {
                keys.emplace_back( "key_" + std::to_string( idx ) );
                vals.emplace_back( "val_" + std::to_string( idx ) );
            }

            benchModeUnorderedMap( numKeys, keys, vals );
            benchModeAxonCache( numKeys, keys, vals );
        }
        else
        {
            readMode( &settings, result );
        }
    }
    catch ( const cxxopts::OptionException & e )
    {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
