// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <filesystem>
#include <string>
#include <memory>
#include <utility>
#include <axoncache/Constants.h>
#include <axoncache/cache/MapCache.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <axoncache/writer/CacheFileWriter.h>
#include <axoncache/writer/detail/GenerateHeader.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <iosfwd>
#include <string_view>
#include "axoncache/cache/CacheType.h"
#include "axoncache/domain/CacheHeader.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

TEST_CASE( "CacheFileWriterTest" )
{
    std::string name = "cache_file_writer_test1";
    auto tmpFile = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + name + std::string{ Constants::kCacheFileNameSuffix };

    // Force file to close by putting the write in it's own scope
    {
        auto memoryHandler = std::make_unique<MallocMemoryHandler>( 1024 * sizeof( uint64_t ) );
        MapCache cache( std::move( memoryHandler ) );

        cache.put( "key1", "value1" );
        cache.put( "this", { "is", "a", "test" } );

        [[maybe_unused]] CacheFileWriter writer( std::filesystem::temp_directory_path(), name, &cache );
        writer.write();
    }

    std::ifstream file( tmpFile, std::ios::binary );

    GenerateHeader headerReader;
    const auto val = headerReader.read( file );

    std::string output;
    while ( file.peek() != EOF )
    {
        output += static_cast<char>( file.get() );
    }

    file.close();

    std::filesystem::remove( tmpFile );

    CHECK( val.first == name );
    CHECK( output == "key1=value1\nthis=is|a|test\n" );
}
