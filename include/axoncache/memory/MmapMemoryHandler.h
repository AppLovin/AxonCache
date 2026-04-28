// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <utility>
#include <stddef.h>
#include <stdint.h>
#include "axoncache/domain/CacheHeader.h"
#include "axoncache/memory/MemoryHandler.h"

namespace axoncache
{
class MmapMemoryHandler : public MemoryHandler
{
  public:
    MmapMemoryHandler( const CacheHeader & header, const std::string & cacheFile, bool isPreloadMemoryEnabled = false );
    ~MmapMemoryHandler() override;

    MmapMemoryHandler( const MmapMemoryHandler & ) = delete;
    MmapMemoryHandler( MmapMemoryHandler & ) = delete;
    MmapMemoryHandler( MmapMemoryHandler && ) = delete;
    auto operator=( const MmapMemoryHandler & ) -> MmapMemoryHandler & = delete;
    auto operator=( MmapMemoryHandler & ) -> MmapMemoryHandler & = delete;
    auto operator=( MmapMemoryHandler && ) -> MmapMemoryHandler & = delete;

    auto allocate( uint64_t newSize ) -> void override;

  protected:
    auto resizeToFit( uint64_t newSize ) -> void override;

  private:
    static auto loadMmap( const CacheHeader & header, const std::string & cacheFile, bool isPreloadMemoryEnabled ) -> std::pair<uint8_t *, size_t>;

    uint8_t * mBasePointer{};
    uint64_t mBaseSize{};
    uint64_t mHeaderSize{};
};
}
