// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/memory/MallocMemoryHandler.h"
#include <cstdlib>

using namespace axoncache;

MallocMemoryHandler::~MallocMemoryHandler()
{
    free( data() );
}

MallocMemoryHandler::MallocMemoryHandler( uint64_t initialCapacity ) :
    mRealDataSize( initialCapacity )
{
    setData( ( uint8_t * )calloc( 1, initialCapacity ) );
}

auto MallocMemoryHandler::allocate( uint64_t newSize ) -> void
{
    if ( data() == nullptr || newSize >= dataSize() )
    {
        if ( data() != nullptr )
        {
            free( data() );
        }

        setData( ( uint8_t * )calloc( 1, newSize ) );
        setDataSize( newSize );
        mRealDataSize = newSize;
    }
}

auto MallocMemoryHandler::resizeToFit( uint64_t newSize ) -> void
{
    if ( newSize >= mRealDataSize )
    {
        auto newSizeBytes = newSize + ( mRealDataSize / 2 ); // increase by 50%

        auto * newValueSpace = ( uint8_t * )std::realloc( ( void * )data(), newSizeBytes );
        setData( newValueSpace );
        mRealDataSize = newSizeBytes;
    }
}
