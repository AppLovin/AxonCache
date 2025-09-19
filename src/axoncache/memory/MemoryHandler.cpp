// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

auto MemoryHandler::setData( uint8_t * data ) -> void
{
    mData = data;
}

auto MemoryHandler::setDataSize( uint64_t dataSize ) -> void
{
    mDataSize = dataSize;
}

auto MemoryHandler::grow( uint64_t growByBytes ) -> uint8_t *
{
    resizeToFit( mDataSize + growByBytes );
    auto oldOffset = mDataSize;
    mDataSize += growByBytes;
    return mData + oldOffset;
}
