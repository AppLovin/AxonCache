// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <filesystem>
#include <string>
#include <memory>
#include <utility>
#include <axoncache/reader/DataFileReader.h>
#ifdef BAZEL_BUILD
#include "doctest/doctest.h"
#else
#include <doctest/doctest.h>
#endif
#include <exception>
#include <iostream>
#include <fstream>
#include "axoncache/common/SharedSettingsProvider.h"
#include "axoncache/cache/CacheType.h"
#include "axoncache/consumer/StringCacheValueConsumer.h"
#include "axoncache/parser/CacheValueParser.h"

using namespace axoncache;

TEST_CASE( "DataFileReaderTest" )
{
    const std::string dataFilename = "data_file_reader_test.txt";
    const auto file1 = std::filesystem::temp_directory_path().string() + std::filesystem::path::preferred_separator + dataFilename;
    std::fstream file1Out( file1, std::ios::out | std::ios::binary );
    file1Out << "hello=world\nthis=is|a|test";
    file1Out.flush();
    file1Out.close();

    try
    {
        SharedSettingsProvider settings( "" );
        auto parser = std::make_unique<CacheValueParser>( &settings );
        DataFileReader reader( { file1 }, std::move( parser ), '\n' );

        StringCacheValueConsumer consumer;
        reader.readValues( &consumer );
        CHECK( consumer.output() == R"("hello":{"type":"String", "value":"world"},
"this":{"type":"String", "value":"is|a|test"})" );
    }
    catch ( std::exception & e )
    {
        std::cerr << e.what() << std::endl;
        CHECK( false );
    }
    std::filesystem::remove( file1 );
}
