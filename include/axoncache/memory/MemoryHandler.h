// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <cstdint>

namespace axoncache
{
class MemoryHandler
{
  public:
    MemoryHandler() = default;
    virtual ~MemoryHandler() = default;

    MemoryHandler( const MemoryHandler & ) = delete;
    MemoryHandler( MemoryHandler & ) = delete;
    MemoryHandler( MemoryHandler && ) = delete;
    auto operator=( const MemoryHandler & ) -> MemoryHandler & = delete;
    auto operator=( MemoryHandler & ) -> MemoryHandler & = delete;
    auto operator=( MemoryHandler && ) -> MemoryHandler & = delete;

    [[nodiscard]] inline uint8_t * data() const
    {
        return mData;
    }

    [[nodiscard]] inline uint64_t dataSize() const
    {
        return mDataSize;
    }

    virtual auto allocate( uint64_t newSize ) -> void = 0;
    virtual auto grow( uint64_t growByBytes ) -> uint8_t *;

  protected:
    auto setData( uint8_t * data ) -> void;

    auto setDataSize( uint64_t dataSize ) -> void;

    virtual auto resizeToFit( uint64_t newSize ) -> void = 0;

  private:
    uint8_t * mData{};
    uint64_t mDataSize{};
};
}
