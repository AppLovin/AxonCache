// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/logger/Logger.h"
#include "axoncache/memory/MmapMemoryHandler.h"
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <string>
#include <sstream>

using namespace axoncache;

MmapMemoryHandler::MmapMemoryHandler( const CacheHeader & header, const std::string & cacheFile, bool isPreloadMemoryEnabled ) :
    mHeaderSize( header.headerSize )
{
    auto mmapResult = loadMmap( header, cacheFile, isPreloadMemoryEnabled );
    mBasePointer = mmapResult.first;
    mBaseSize = mmapResult.second;

    setData( mBasePointer + mHeaderSize );
    setDataSize( mBaseSize - mHeaderSize );
}

MmapMemoryHandler::~MmapMemoryHandler()
{
    if ( mBasePointer != nullptr )
    {
        munmap( mBasePointer, mBaseSize );
    }
}

auto MmapMemoryHandler::allocate( uint64_t newSize ) -> void
{
    if ( newSize > dataSize() )
    {
        throw std::runtime_error( "MmapMemoryHandler does not support resizing" );
    }
}

auto MmapMemoryHandler::resizeToFit( uint64_t /* newSize */ ) -> void
{
    throw std::runtime_error( "MmapMemoryHandler::resizeToFit() not implemented" );
}

auto MmapMemoryHandler::loadMmap( const CacheHeader & header, const std::string & cacheFile, [[maybe_unused]] bool isPreloadMemoryEnabled ) -> std::pair<uint8_t *, size_t>
{
    auto fd = open( cacheFile.c_str(), O_RDONLY ); // NOLINT
    if ( fd == -1 )
    {
        std::ostringstream oss;
        oss << "opening file failed: " << cacheFile
            << " error " << strerror( errno );
        AL_LOG_ERROR( oss.str() );
        return { nullptr, 0 };
    }

    struct stat st{};
    if ( fstat( fd, &st ) != 0 )
    {
        AL_LOG_ERROR( "fstat failed for " + cacheFile );
        close( fd );
        return { nullptr, 0 };
    }

    const auto fileSize = st.st_size;
    if ( fileSize <= header.headerSize )
    {
        AL_LOG_ERROR( "Cache has invalid size " + cacheFile );
        close( fd );
        return { nullptr, 0 };
    }

#if defined( __APPLE__ )
    //osx does not support MAP_POPULATE
    int mmapOptions = MAP_PRIVATE;
#else
    int mmapOptions = MAP_SHARED;

    // MAP_POPULATE prefetches all pages, this means mmap blocks until all the data is loaded into memory
    if ( isPreloadMemoryEnabled )
    {
        mmapOptions |= MAP_POPULATE;
    }
#endif
    auto * result = mmap( nullptr, fileSize, PROT_READ, mmapOptions, fd, 0 );

    if ( result == MAP_FAILED ) // NOLINT
    {
        std::ostringstream oss;
        oss << "mmap failed: " << cacheFile
            << " fd " << fd
            << " error " << strerror( errno );
        AL_LOG_ERROR( oss.str() );
        close( fd );
        return { nullptr, 0 };
    }

    close( fd );

    return { ( uint8_t * )result, fileSize };
}
