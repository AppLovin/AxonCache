// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/reader/DataFileReader.h"
#include <utility>
#include <fstream>
#include <stdexcept>
#include "axoncache/logger/Logger.h"
#include "axoncache/consumer/CacheValueConsumerBase.h"

using namespace axoncache;

DataFileReader::DataFileReader( std::vector<std::string> fileNames, std::unique_ptr<CacheValueParser> parser, char lineDelimiter ) :
    mFileNames( std::move( fileNames ) ), mLineParser( std::move( parser ) ), mLineDelimiter( lineDelimiter )
{
}

auto DataFileReader::readValues( CacheValueConsumerBase * consumer ) -> uint64_t
{
    AL_LOG_INFO( "Number of input files: " + std::to_string( mFileNames.size() ) );
    auto linesRead = 0UL;
    for ( const auto & fileName : mFileNames )
    {
        AL_LOG_INFO( "Reading file: " + fileName );
        std::ifstream dataFile;
        std::string line;

        dataFile.open( fileName, std::ios::binary );

        if ( !dataFile.is_open() )
        {
            AL_LOG_ERROR( "Failed to open file: " + fileName );
            throw std::runtime_error( "Failed to open file" );
        }

        while ( std::getline( dataFile, line, mLineDelimiter ) )
        {
            consumer->consumeValue( mLineParser->parseValue( line.data(), line.size() ) );
            linesRead++;

            if ( linesRead % 100000UL == 0UL )
            {
                AL_LOG_INFO( "Lines read: " + std::to_string( linesRead ) );
            }
        }

        AL_LOG_INFO( "Total read lines: " + std::to_string( linesRead ) );
        dataFile.close();
    }

    return linesRead;
}
