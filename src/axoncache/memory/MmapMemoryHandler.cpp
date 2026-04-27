// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/logger/Logger.h"
#include "axoncache/memory/MmapMemoryHandler.h"
#ifdef HAVE_LIBNUMA
#include <numa.h>
#include <numaif.h>
#endif
#include <cerrno>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

using namespace axoncache;

MmapMemoryHandler::MmapMemoryHandler( const CacheHeader & header, const std::string & cacheFile, bool isPreloadMemoryEnabled, bool isNumaInterleaveEnabled ) :
    mHeaderSize( header.headerSize )
{
    auto mmapResult = loadMmap( header, cacheFile, isPreloadMemoryEnabled, isNumaInterleaveEnabled );
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

auto MmapMemoryHandler::loadMmap( const CacheHeader & header, const std::string & cacheFile, [[maybe_unused]] bool isPreloadMemoryEnabled, [[maybe_unused]] bool isNumaInterleaveEnabled ) -> std::pair<uint8_t *, size_t>
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

#if !defined( __APPLE__ )
#ifdef HAVE_LIBNUMA
    if ( isNumaInterleaveEnabled && numa_available() >= 0 )
    {
        struct bitmask * nodes = numa_get_mems_allowed();
        if ( mbind( result, fileSize, MPOL_INTERLEAVE, nodes->maskp, nodes->size + 1, MPOL_MF_STRICT ) != 0 )
        {
            AL_LOG_ERROR( "mbind MPOL_INTERLEAVE failed: " + std::string( strerror( errno ) ) );
        }
        numa_free_nodemask( nodes );
    }
#endif
    if ( isPreloadMemoryEnabled )
    {
        madvise( result, fileSize, MADV_WILLNEED );
        logResidency();
    }
#endif

    return { static_cast<uint8_t *>( result ), fileSize };
}

auto MmapMemoryHandler::logResidency() const -> void
{
    if ( mBasePointer == nullptr )
    {
        return;
    }

    const long pageSize = sysconf( _SC_PAGESIZE );
    const size_t pageCount = ( mBaseSize + static_cast<size_t>( pageSize ) - 1 ) / static_cast<size_t>( pageSize );

    std::vector<unsigned char> vec( pageCount, 0 );
#ifdef __APPLE__
    if ( mincore( mBasePointer, mBaseSize, reinterpret_cast<char *>( vec.data() ) ) != 0 )
#else
    if ( mincore( mBasePointer, mBaseSize, vec.data() ) != 0 )
#endif
    {
        AL_LOG_ERROR( "mincore failed: " + std::string( strerror( errno ) ) );
        return;
    }

    size_t residentPages = 0;
    for ( const auto byte : vec )
    {
        if ( byte & 1 )
        {
            ++residentPages;
        }
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision( 2 );
    oss << "mmap residency: " << residentPages << " / " << pageCount
        << " pages (" << ( 100.0 * residentPages / pageCount ) << "%)";
    AL_LOG_INFO( oss.str() );
}
