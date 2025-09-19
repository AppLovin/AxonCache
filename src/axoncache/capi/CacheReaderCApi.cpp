// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(cppcoreguidelines-pro-type-cstyle-cast)

#include "axoncache/capi/CacheReaderCApi.h"
#include <memory>
#include <cstdio>
#include <span>
#include <atomic>

#include <axoncache/CacheGenerator.h>
#include <axoncache/loader/CacheOneTimeLoader.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/cache/BucketChainCache.h>
#include "axoncache/common/SharedSettingsProvider.h"

using namespace axoncache;

namespace
{
// Caller need to free return ptr and valueSizes ptr for these helper functions
char * convertToPointer( std::string_view value, int * valueSize )
{
    if ( value.empty() )
    {
        *valueSize = 0;
        return nullptr;
    }
    *valueSize = static_cast<int>( value.size() );
    char * ptr = ( char * )malloc( ( value.size() + 1 ) * sizeof( char ) );
    std::memcpy( ptr, value.data(), value.size() * sizeof( char ) );
    ptr[value.size()] = '\0';
    return ptr;
}

float * convertToPointer( std::span<const float> values, int * vectorSize )
{
    if ( values.empty() )
    {
        *vectorSize = 0;
        return nullptr;
    }
    *vectorSize = static_cast<int>( values.size() );
    float * ptr = ( float * )malloc( values.size() * sizeof( float ) );
    std::memcpy( ptr, values.data(), values.size() * sizeof( float ) );
    return ptr;
}

char ** convertToPointer( std::vector<std::string_view> values, int * vectorSize, int ** valueSizes )
{
    if ( values.empty() )
    {
        *vectorSize = 0;
        *valueSizes = nullptr;
        return nullptr;
    }
    *vectorSize = static_cast<int>( values.size() );
    int * sizes = ( int * )malloc( values.size() * sizeof( int ) );
    char ** ptr = ( char ** )malloc( values.size() * sizeof( char * ) );
    for ( size_t index = 0; index < values.size(); index++ )
    {
        sizes[index] = static_cast<int>( values[index].size() );
        ptr[index] = ( char * )malloc( ( values[index].size() + 1 ) * sizeof( char ) );
        std::memcpy( ptr[index], values[index].data(), values[index].size() * sizeof( char ) );
        ptr[index][values[index].size()] = '\0';
    }
    *valueSizes = sizes;
    return ptr;
}
}

class CacheReader // NOLINT
{
  public:
    CacheReader() = default;
    virtual ~CacheReader() = default;

    int initializeReader( const std::string & taskName, const std::string & destinationFolder, const std::string & timestamp, bool isPreloadMemoryEnabled )
    {
        const axoncache::SharedSettingsProvider settings( "" );
        axoncache::CacheOneTimeLoader loader( &settings );

        // The decompressed file name is flipped, the extension comes last
        const size_t lastindex = taskName.find_last_of( '.' );
        auto cacheNameNoExt = taskName.substr( 0, lastindex );

        std::string cacheName;
        cacheName += cacheNameNoExt;
        cacheName += ".";
        cacheName += timestamp;
        cacheName += ".cache";

        std::string cacheAbsolutePath( destinationFolder );
        cacheAbsolutePath += "/";
        cacheAbsolutePath += cacheName;

        // To keep a copy of the file in case of errors
        // Use ALFileUtils::copyFile( cacheAbsolutePath, "/tmp/test.cache" );

        CacheHeader info;
        std::ifstream inputFile( cacheAbsolutePath );
        if ( !inputFile.good() )
        {
            return 1;
        }

        inputFile.read( reinterpret_cast<char *>( &info ), sizeof( CacheHeader ) ); // NOLINT
        if ( !inputFile.good() )
        {
            return 1;
        }

        mCacheType = static_cast<axoncache::CacheType>( info.cacheType );

        try
        {
            switch ( mCacheType )
            {
                case axoncache::CacheType::LINEAR_PROBE:
                {
                    auto cache = loader.loadAbsolutePath<axoncache::LinearProbeCache>( cacheName, cacheAbsolutePath, isPreloadMemoryEnabled );
                    std::atomic_store( &mReaderLinearProbeCache, cache );
                }
                break;

                case axoncache::CacheType::LINEAR_PROBE_DEDUP:
                case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
                {
                    auto cache = loader.loadAbsolutePath<axoncache::LinearProbeDedupCache>( cacheName, cacheAbsolutePath, isPreloadMemoryEnabled );
                    std::atomic_store( &mReaderLinearProbeDedupCache, cache );
                }
                break;

                case axoncache::CacheType::BUCKET_CHAIN:
                {
                    auto cache = loader.loadAbsolutePath<axoncache::BucketChainCache>( cacheName, cacheAbsolutePath, isPreloadMemoryEnabled );
                    std::atomic_store( &mReaderBucketChainCache, cache );
                }
                break;

                case axoncache::CacheType::MAP:
                case axoncache::CacheType::NONE:
                    break;
            }
        }
        catch ( std::exception & e )
        {
            fprintf( stderr, "Error initializing reader %s\n", e.what() ); // NOLINT
            return 1;
        }

        return 0;
    }

    void finalizeReader()
    {
    }

    int containsKey( char * key, size_t keySize )
    {
        if ( key == nullptr )
        {
            return 0;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return 0;
                }
                return cache->contains( std::string_view{ key, keySize } ) ? 1 : 0;
            }

            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                const auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return 0;
                }
                return cache->contains( std::string_view{ key, keySize } ) ? 1 : 0;
            }

            case axoncache::CacheType::BUCKET_CHAIN:
            {
                const auto cache = std::atomic_load( &mReaderBucketChainCache );
                if ( cache == nullptr )
                {
                    return 0;
                }
                return cache->contains( std::string_view{ key, keySize } ) ? 1 : 0;
            }

            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                return 0;
        }
    }

    char * getKey( char * key, size_t keySize, int * isExist, int * valueSize )
    {
        *isExist = 0;
        *valueSize = 0;
        if ( key == nullptr )
        {
            return nullptr;
        }

        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                auto result = cache->getString( std::string_view{ key, keySize } );
                *isExist = result.second ? 1 : 0;
                return convertToPointer( result.first, valueSize );
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                auto result = cache->getString( std::string_view{ key, keySize } );
                *isExist = result.second ? 1 : 0;
                return convertToPointer( result.first, valueSize );
            }

            case axoncache::CacheType::BUCKET_CHAIN:
            {
                auto cache = std::atomic_load( &mReaderBucketChainCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                auto result = cache->getString( std::string_view{ key, keySize } );
                *isExist = result.second ? 1 : 0;
                return convertToPointer( result.first, valueSize );
            }

            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                return nullptr;
        }
    }

    char * getVectorKeyItem( char * key, size_t keySize, int index, int * valueSize )
    {
        *valueSize = 0;
        if ( key == nullptr )
        {
            return nullptr;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                const auto items = cache->getVector( std::string_view{ key, keySize } );
                if ( index < static_cast<int>( items.size() ) )
                {
                    return convertToPointer( items[index], valueSize );
                }
            }
            break;
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                const auto items = cache->getVector( std::string_view{ key, keySize } );
                if ( index < static_cast<int>( items.size() ) )
                {
                    return convertToPointer( items[index], valueSize );
                }
            }
            break;
            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                break;
        }
        return nullptr;
    }

    size_t getVectorKeySize( char * key, size_t keySize )
    {
        if ( key == nullptr )
        {
            return 0;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return 0;
                }
                return cache->getVector( std::string_view{ key, keySize } ).size();
            }

            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return 0;
                }
                return cache->getVector( std::string_view{ key, keySize } ).size();
            }

            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                return 0;
        }
    }

    int64_t getLong( char * key, size_t keySize, int * isExist, int64_t defaultValue )
    {
        *isExist = 0;
        if ( key == nullptr )
        {
            return defaultValue;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getInt64( std::string_view{ key, keySize }, defaultValue );
                *isExist = result.second ? 1 : 0;
                return result.first;
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getInt64( std::string_view{ key, keySize }, defaultValue );
                *isExist = result.second ? 1 : 0;
                return result.first;
            }
            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
            {
                return int64_t{}; // Not supported
            }
        }
    }

    int getInteger( char * key, size_t keySize, int * isExist, int defaultValue )
    {
        *isExist = 0;
        if ( key == nullptr )
        {
            return defaultValue;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getInt64( std::string_view{ key, keySize }, defaultValue );
                *isExist = result.second ? 1 : 0;
                return static_cast<int>( result.first );
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getInt64( std::string_view{ key, keySize }, defaultValue );
                *isExist = result.second ? 1 : 0;
                return static_cast<int>( result.first );
            }
            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
            {
                return int{}; // Not supported
            }
        }
    }

    double getDouble( char * key, size_t keySize, int * isExist, double defaultValue )
    {
        *isExist = 0;
        if ( key == nullptr )
        {
            return defaultValue;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getDouble( std::string_view{ key, keySize }, defaultValue );
                *isExist = result.second ? 1 : 0;
                return result.first;
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getDouble( std::string_view{ key, keySize }, defaultValue );
                *isExist = result.second ? 1 : 0;
                return result.first;
            }
            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
            {
                return double{}; // Not supported
            }
        }
    }

    int getBool( char * key, size_t keySize, int * isExist, int defaultValue )
    {
        *isExist = 0;
        if ( key == nullptr )
        {
            return defaultValue;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getBool( std::string_view{ key, keySize }, defaultValue != 0 );
                *isExist = result.second ? 1 : 0;
                return result.first ? 1 : 0;
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return defaultValue;
                }
                auto result = cache->getBool( std::string_view{ key, keySize }, defaultValue != 0 );
                *isExist = result.second ? 1 : 0;
                return result.first ? 1 : 0;
            }
            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
            {
                return 0; // Not supported
            }
        }
    }

    char ** getVector( char * key, size_t keySize, int * vectorSize, int ** valueSizes )
    {
        *vectorSize = 0;
        *valueSizes = nullptr;
        if ( key == nullptr )
        {
            return nullptr;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                return convertToPointer( cache->getVector( std::string_view{ key, keySize } ), vectorSize, valueSizes );
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                return convertToPointer( cache->getVector( std::string_view{ key, keySize } ), vectorSize, valueSizes );
            }
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                return nullptr;
        }
    }

    float * getFloatVector( char * key, size_t keySize, int * vectorSize )
    {
        *vectorSize = 0;
        if ( key == nullptr )
        {
            return nullptr;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                return convertToPointer( cache->getFloatSpan( std::string_view{ key, keySize } ), vectorSize );
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                return convertToPointer( cache->getFloatSpan( std::string_view{ key, keySize } ), vectorSize );
            }
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                return nullptr;
        }
    }

    char * getKeyType( char * key, size_t keySize, int * valueSize )
    {
        *valueSize = 0;
        if ( key == nullptr )
        {
            return nullptr;
        }
        switch ( mCacheType )
        {
            case axoncache::CacheType::LINEAR_PROBE:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                return convertToPointer( cache->getKeyType( std::string_view{ key, keySize } ), valueSize );
            }
            case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            {
                auto cache = std::atomic_load( &mReaderLinearProbeDedupCache );
                if ( cache == nullptr )
                {
                    return nullptr;
                }
                return convertToPointer( cache->getKeyType( std::string_view{ key, keySize } ), valueSize );
            }
            case axoncache::CacheType::BUCKET_CHAIN:
            case axoncache::CacheType::MAP:
            case axoncache::CacheType::NONE:
            default:
                return nullptr;
        }
    }

  private:
    std::shared_ptr<LinearProbeCache> mReaderLinearProbeCache;
    std::shared_ptr<LinearProbeDedupCache> mReaderLinearProbeDedupCache;
    std::shared_ptr<BucketChainCache> mReaderBucketChainCache;
    axoncache::CacheType mCacheType{ CacheType::LINEAR_PROBE_DEDUP };
};

typedef struct _CacheReaderHandle
{
    std::unique_ptr<CacheReader> src;
} CacheReaderHandle;

CacheReaderHandle * NewCacheReaderHandle()
{
    auto * handle = new CacheReaderHandle(); // NOLINT
    handle->src = std::make_unique<CacheReader>();

    return handle;
}

void CacheReader_DeleteCppObject( CacheReaderHandle * handle )
{
    delete handle; // NOLINT
}

int CacheReader_Initialize( CacheReaderHandle * handle, const char * taskName, const char * destinationFolder, const char * timestamp, int isPreloadMemoryEnabled )
{
    return handle->src->initializeReader( taskName, destinationFolder, timestamp, isPreloadMemoryEnabled );
}

void CacheReader_Finalize( CacheReaderHandle * handle )
{
    handle->src->finalizeReader();
}

char * CacheReader_GetKey( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int * valueSize )
{
    return handle->src->getKey( key, keySize, isExist, valueSize );
}

char * CacheReader_GetVectorKey( CacheReaderHandle * handle, char * key, size_t keySize, int32_t index, int * valueSize )
{
    return handle->src->getVectorKeyItem( key, keySize, index, valueSize );
}

int CacheReader_GetVectorKeySize( CacheReaderHandle * handle, char * key, size_t keySize )
{
    return static_cast<int>( handle->src->getVectorKeySize( key, keySize ) );
}

int CacheReader_ContainsKey( CacheReaderHandle * handle, char * key, size_t keySize )
{
    return handle->src->containsKey( key, keySize );
}

int64_t CacheReader_GetLong( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int64_t defaultValue )
{
    return handle->src->getLong( key, keySize, isExist, defaultValue );
}

int CacheReader_GetInteger( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int defaultValue )
{
    return handle->src->getInteger( key, keySize, isExist, defaultValue );
}

double CacheReader_GetDouble( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, double defaultValue )
{
    return handle->src->getDouble( key, keySize, isExist, defaultValue );
}

int CacheReader_GetBool( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int defaultValue )
{
    return handle->src->getBool( key, keySize, isExist, defaultValue );
}

char ** CacheReader_GetVector( CacheReaderHandle * handle, char * key, size_t keySize, int * vectorSize, int ** valueSizes )
{
    return handle->src->getVector( key, keySize, vectorSize, valueSizes );
}

float * CacheReader_GetFloatVector( CacheReaderHandle * handle, char * key, size_t keySize, int * vectorSize )
{
    return handle->src->getFloatVector( key, keySize, vectorSize );
}

char * CacheReader_GetKeyType( CacheReaderHandle * handle, char * key, size_t keySize, int * valueSize )
{
    return handle->src->getKeyType( key, keySize, valueSize );
}

// NOLINTEND(cppcoreguidelines-pro-type-cstyle-cast)
// NOLINTEND(modernize-use-trailing-return-type)
