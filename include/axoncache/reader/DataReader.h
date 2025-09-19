// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <cstdint>
namespace axoncache
{
class CacheValueConsumerBase;
}

namespace axoncache
{
class DataReader
{
  public:
    DataReader();
    DataReader( DataReader && ) = delete;
    DataReader( DataReader & ) = delete;
    DataReader( const DataReader & ) = delete;
    auto operator=( const DataReader & ) -> DataReader & = delete;
    auto operator=( DataReader & ) -> DataReader & = delete;
    auto operator=( DataReader && ) -> DataReader & = delete;
    virtual ~DataReader() = default;
    virtual auto readValues( CacheValueConsumerBase * consumer ) -> uint64_t = 0;
};

} // namespace axoncache
