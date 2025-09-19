// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <cstdint>
#include "axoncache/memory/MemoryHandler.h"
#include "axoncache/Constants.h"

namespace axoncache
{
class MallocMemoryHandler : public MemoryHandler
{
  public:
    MallocMemoryHandler( uint64_t initialCapacity = Constants::ConfDefault::kMemoryCapcityBytes );
    ~MallocMemoryHandler() override;

    MallocMemoryHandler( const MallocMemoryHandler & ) = delete;
    MallocMemoryHandler( MallocMemoryHandler & ) = delete;
    MallocMemoryHandler( MallocMemoryHandler && ) = delete;
    auto operator=( const MallocMemoryHandler & ) -> MallocMemoryHandler & = delete;
    auto operator=( MallocMemoryHandler & ) -> MallocMemoryHandler & = delete;
    auto operator=( MallocMemoryHandler && ) -> MallocMemoryHandler & = delete;

    auto allocate( uint64_t newSize ) -> void override;

  protected:
    auto resizeToFit( uint64_t newSize ) -> void override;

  private:
    uint64_t mRealDataSize{};
};
}
