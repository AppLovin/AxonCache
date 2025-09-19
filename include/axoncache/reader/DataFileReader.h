// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once
#include <string>
#include <memory>
#include <stdint.h>
#include <vector>
#include "axoncache/Constants.h"
#include "axoncache/parser/CacheValueParser.h"
#include "axoncache/reader/DataReader.h"
namespace axoncache
{
class CacheValueConsumerBase;
}

namespace axoncache
{
class DataFileReader : public DataReader
{
  public:
    DataFileReader( std::vector<std::string> fileNames, std::unique_ptr<CacheValueParser> parser, char lineDelimiter = Constants::ConfDefault::kControlCharLine );
    ~DataFileReader() override = default;

    DataFileReader( DataFileReader && ) = delete;
    DataFileReader( DataFileReader & ) = delete;
    DataFileReader( const DataFileReader & ) = delete;
    auto operator=( const DataFileReader & ) -> DataFileReader & = delete;
    auto operator=( DataFileReader & ) -> DataFileReader & = delete;
    auto operator=( DataFileReader && ) -> DataFileReader & = delete;

    auto readValues( CacheValueConsumerBase * consumer ) -> uint64_t override;

  private:
    const std::vector<std::string> mFileNames;
    std::unique_ptr<CacheValueParser> mLineParser;
    const char mLineDelimiter;
};
} // namespace axoncache
