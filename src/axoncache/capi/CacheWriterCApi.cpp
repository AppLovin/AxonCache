// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(cppcoreguidelines-pro-type-cstyle-cast)

#include "axoncache/capi/CacheWriterCApi.h"

#include "CacheInfo.h"

#include <memory>
#include <cstdio>
#include <unistd.h>
#include <cfloat>

#include "axoncache/common/StringUtils.h"
#include "axoncache/common/StringViewUtils.h"
#include "axoncache/common/SharedSettingsProvider.h"

#include <axoncache/CacheGenerator.h>
#include <axoncache/builder/CacheFileBuilder.h>
#include <axoncache/cache/factory/CacheFactory.h>
#include <axoncache/loader/CacheOneTimeLoader.h>
#include <axoncache/cache/BucketChainCache.h>
#include <axoncache/cache/LinearProbeCache.h>
#include <axoncache/cache/LinearProbeDedupCache.h>
#include <axoncache/domain/CacheValue.h>
#include <axoncache/transformer/TypeToString.h>
#include <axoncache/logger/Logger.h>

using CollisionsCounter = std::unordered_map<size_t, size_t>;

namespace
{
const int8_t kStringNoNullType = 127; // Defined in ccache.go

static const char kValueVectorFlag = 1;
static const size_t kNumHashKeys = 100000001;
static const std::string kDestinationFolder = "/var/lib/applovin/datamover";

auto readFile( const std::string & filename ) -> std::string
{
    std::ifstream file( filename, std::ios::in | std::ios::binary );
    if ( !file )
    {
        return {};
    }

    std::ostringstream oss;
    oss << file.rdbuf(); // Read entire file into the stream
    return oss.str();
}

}

using namespace axoncache;

struct CCacheOptions_s
{
    std::string destinationFolder;
    std::string mmapName;
    std::string cacheNameNoExt;
    size_t maxNumberOfKeys;
    size_t maxCacheSize;
    size_t cacheSizeAlertLimit;
    bool acceptOldCaches;
    double maxLoadFactor;
    int cacheType;
    int offsetBits;
};

using CCacheOptions = struct CCacheOptions_s;

class AxonCacheWriter
{
  public:
    AxonCacheWriter() :
        mCacheFileBuilder( nullptr ),
        mCacheType( CacheType::LINEAR_PROBE )
    {
        ;
    }

    ~AxonCacheWriter()
    {
        ;
    }

    void setOptionsFromProperties( const std::string & taskName, const std::string & settingsLocation, CCacheOptions * ccacheOptions )
    {
        axoncache::SharedSettingsProvider settings( settingsLocation );

        ccacheOptions->destinationFolder = settings.getString( "ccache.destination_folder", kDestinationFolder.c_str() );
        ccacheOptions->maxNumberOfKeys = settings.getInt( "ccache.number_of_hash_keys", kNumHashKeys );
        ccacheOptions->cacheType = settings.getInt( "ccache.type", 5 );          // Linear probe dedup typed
        ccacheOptions->offsetBits = settings.getInt( "ccache.offset.bits", 35 ); // Linear probe dedup typed

        const auto & cCacheName = taskName;
        ccacheOptions->mmapName = settings.getString( "ccache.mmap_file", cCacheName.c_str() );
        // Strip the .mmap part to build the cache name without extension
        size_t lastindex = ccacheOptions->mmapName.find_last_of( '.' );
        ccacheOptions->cacheNameNoExt = ccacheOptions->mmapName.substr( 0, lastindex );

        // FIXME: hack this is hard-coded
        ccacheOptions->maxLoadFactor = settings.getDouble( "ccache.max_load_factor", 0.5 );

        std::ostringstream oss;
        oss << "taskname: " << taskName
            << " settingFile: " << settingsLocation
            << " properties file content: \n"
            << readFile( settingsLocation ) << "\n"
            << "ccacheOptions->destinationFolder = " << ccacheOptions->destinationFolder << "\n";
        AL_LOG_INFO( oss.str() );
    }

    int8_t initializeWriter( const std::string & taskName, const std::string & settingsLocation, uint64_t numberOfKeySlots )
    {
        setOptionsFromProperties( taskName, settingsLocation, &mCCacheOptions );

        if ( mCacheInfo.minKeyLength() == 0 )
        {
            mCacheInfo.setMinKeyLength( std::numeric_limits<size_t>::max() );
            mCacheInfo.setMinValueLength( std::numeric_limits<size_t>::max() );
        }

        axoncache::SharedSettingsProvider settings( settingsLocation );

        setCacheType( mCCacheOptions.cacheType );
        auto cacheType = mCacheType;

        auto cacheNameNoExt = mCCacheOptions.cacheNameNoExt;
        auto outputDirectory = mCCacheOptions.destinationFolder;
        auto maxLoadFactor = mCCacheOptions.maxLoadFactor;
        auto offsetBits = mCCacheOptions.offsetBits;

        if ( ( offsetBits < Constants::kMinLinearProbeOffsetBits ) || ( Constants::kMaxLinearProbeOffsetBits < offsetBits ) )
        {
            return 3;
        }

        try
        {
            auto cache = CacheFactory::createCache( offsetBits, numberOfKeySlots, maxLoadFactor, cacheType );

            // make the cache file builder a member variable ; needs to be a pointer or compile errors
            mCacheFileBuilder = std::make_unique<CacheFileBuilder>(
                &settings, outputDirectory, cacheNameNoExt, std::vector<std::string>{ "dummy.dat" }, std::move( cache ) );
        }
        catch ( std::exception & e )
        {
            mLastError = e.what();
            return 1;
        }

        return 0;
    }

    void finalizeWriter()
    {
        mCacheFileBuilder.reset();
    }

    // FIXME: string matching is fragile, we should throw a custom exception when this happens
    bool isOffsetBitsInsertError()
    {
        if ( mLastError.find( "offset bits " ) == std::string::npos )
        {
            return false;
        }
        if ( mLastError.find( std::to_string( mOffsetBits ) ) == std::string::npos )
        {
            return false;
        }
        if ( mLastError.find( "too short" ) == std::string::npos )
        {
            return false;
        }
        return true;
    }

    // FIXME: string matching is fragile, we should throw a custom exception when this happens
    bool isKeySpaceIsFullError()
    {
        return mLastError.find( "keySpace is full" ) != std::string::npos;
    }

    int8_t insertScalarKey( char * key, size_t keySize, char * value, size_t valueSize, int8_t type )
    {
        CacheKeyValue keyValuePair;

        // Scalar/regular keys
        keyValuePair.first = std::string_view{ key, keySize };
        auto valueType = static_cast<CacheValueType>( type );

        // keyValuePair.second = CacheValue( std::string_view{ value, valueSize } );

        if ( valueType == CacheValueType::String )
        {
            keyValuePair.second = CacheValue( std::string_view{ value, valueSize } );
        }
        else if ( valueType == CacheValueType::Double ) // double
        {
            double actualValue = StringUtils::toDouble( std::string_view{ value, valueSize } );
            keyValuePair.second = CacheValue( actualValue );
        }
        else if ( valueType == CacheValueType::Bool ) // bool
        {
            bool actualValue = StringUtils::toBool( std::string_view{ value, valueSize } );
            keyValuePair.second = CacheValue( actualValue );
        }
        else if ( valueType == CacheValueType::Int64 ) // int64
        {
            int64_t actualValue = StringUtils::toLong( std::string_view{ value, valueSize } );
            // This is mysterious comment, that needed to make it work
            keyValuePair.second = CacheValue( actualValue );
        }
        else if ( valueType == CacheValueType::FloatList )
        {
            auto actualValue = axoncache::stringViewToVector<float>( std::string_view{ value, valueSize }, ':', valueSize );
            keyValuePair.second = CacheValue( std::move( actualValue ) );
        }
        else if ( type == kStringNoNullType )
        {
            // Legacy C-Cache behavior: Truncate value at null if exist
            keyValuePair.second = CacheValue( std::string_view{ value, strnlen( value, valueSize ) } );
        }
        else // default string
        {
            keyValuePair.second = CacheValue( std::string_view{ value, valueSize } );
        }

        try
        {
            auto ret = mCacheFileBuilder->consumer()->consumeValue( keyValuePair );
            mCollisionsCounter[ret.second]++;
        }
        catch ( std::exception & e )
        {
            mLastError = e.what();
            if ( isOffsetBitsInsertError() )
            {
                return 2;
            }
            else if ( isKeySpaceIsFullError() )
            {
                return 3;
            }
            else
            {
                return 1;
            }
        }

        return 0;
    }

    int8_t insertVectorKey( char * key, size_t keySize, char * value, size_t valueSize, int8_t keyType )
    {
        CacheKeyValue keyValuePair;

        // Vector keys
        keyValuePair.first = std::string_view{ key + 1, keySize - 1 };

        std::string valueStr( value, valueSize );
        auto valueVec = axoncache::StringUtils::splitStringView( '|', valueStr );
        std::sort( valueVec.begin(), valueVec.end() );

        if ( keyType == kStringNoNullType )
        {
            for ( auto & val : valueVec )
            {
                // Legacy C-Cache behavior: Truncate value at null if exist
                val = std::string_view{ val.data(), strnlen( val.data(), val.size() ) };
            }
        }

        keyValuePair.second = CacheValue( valueVec );

        try
        {
            auto ret = mCacheFileBuilder->consumer()->consumeValue( keyValuePair );
            mCollisionsCounter[ret.second]++;
        }
        catch ( std::exception & e )
        {
            mLastError = e.what();
            if ( isOffsetBitsInsertError() )
            {
                return 2;
            }
            else if ( isKeySpaceIsFullError() )
            {
                return 3;
            }
            else
            {
                return 1;
            }
        }

        return 0;
    }

    int8_t insertKey( char * key, size_t keySize, char * value, size_t valueSize, int8_t keyType )
    {
        if ( key[0] != kValueVectorFlag )
        {
            return insertScalarKey( key, keySize, value, valueSize, keyType );
        }
        else
        {
            return insertVectorKey( key, keySize, value, valueSize, keyType );
        }
    }

    char * insertKeyWithError( char * key, size_t keySize, char * value, size_t valueSize, int8_t keyType, int8_t & errorCode, size_t * errorMsgSize )
    {
        errorCode = insertKey( key, keySize, value, valueSize, keyType );
        if ( errorCode != 0 && !mLastError.empty() )
        {
            if ( errorMsgSize != nullptr )
            {
                *errorMsgSize = mLastError.size();
            }
            return strdup( mLastError.c_str() );
        }
        if ( errorMsgSize != nullptr )
        {
            *errorMsgSize = 0;
        }
        return nullptr;
    }

    int8_t finishCacheCreation()
    {
        mCacheInfo.setTotalKeys( mCacheFileBuilder->cache()->numberOfEntries() );
        mCacheInfo.setMaxCollisionCount( mCacheFileBuilder->cache()->maxCollisions() );

        //
        // Write file to disk
        // CCache V1 does things a bit differently, it would be good to unify the behavior
        //
        auto writer = mCacheFileBuilder->createWriter();
        writer->write();

        // This will trigger a disk IO flush, by deleting the writer which calls flush.
        // It would be cleaner to call endWrite instead, maybe we can do that by
        // doing a dynamic_cast and get the writer object.
        //
        // It we are not doing this, the files might not contains all its content.
        mCacheFileBuilder.reset();

        return 0;
    }

    const char * getCacheVersion()
    {
        return strdup( "v2" );
    }

    const char * getCacheHashFunction()
    {
        return strdup( "xxh3" ); // This info should be available on the axoncache objects too
    }

    uint64_t getCacheUniqueKeys()
    {
        return mCacheInfo.uniqueKeys();
    }

    uint64_t getCacheMaxCollisions()
    {
        return mCacheInfo.maxCollisionCount();
    }

    char * getCollisionsCounter()
    {
        // Sort entries by increasing collision number
        // First extract index and put them in a vector
        std::vector<size_t> vec;
        for ( const auto iter : mCollisionsCounter )
        {
            vec.push_back( iter.first );
        }

        // Sort the index vector
        std::sort( vec.begin(), vec.end() );

        // Scan the sorted index vector and extract associated
        // collision count
        std::vector<std::string> entries;
        for ( const auto & idx : vec )
        {
            std::string entry( "[" );
            entry += std::to_string( idx );
            entry += ",";
            entry += std::to_string( mCollisionsCounter[idx] );
            entry += "]";
            entries.push_back( entry );
        }

        std::string buffer( "[" );
        if ( !entries.empty() )
        {
            buffer += axoncache::StringUtils::join( ',', entries );
        }
        buffer += "]";

        return strdup( buffer.c_str() );
    }

    void setCacheType( int cacheType )
    {
        mCacheType = static_cast<axoncache::CacheType>( cacheType );
    }

    void setOffsetBits( int offsetBits )
    {
        mOffsetBits = offsetBits;
    }

    char * getLastError()
    {
        char * error = strdup( mLastError.c_str() );
        mLastError.clear();
        return error;
    }

    void addDuplicateValue( const std::string & value, int8_t queryType )
    {
        std::string val = value;
        auto valueType = static_cast<CacheValueType>( queryType );
        if ( valueType == CacheValueType::String )
        {
            val.push_back( '\0' );
        }
        else if ( valueType == CacheValueType::FloatList )
        {
            auto values = parseAsFloat( val, ':' );
            val = std::string{ ( const char * )values.data(), sizeof( float ) * values.size() };
        }
        mDuplicateValues.push_back( val );
    }

    int8_t finishAddDuplicateValues()
    {
        if ( mCacheType != axoncache::CacheType::LINEAR_PROBE_DEDUP && mCacheType != axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED )
        {
            return 1;
        }
        try
        {
            // CacheFileBuilder holds cache to consume value
            ( ( LinearProbeDedupCache * )mCacheFileBuilder->cache() )->setDuplicatedValues( mDuplicateValues );
            return 0;
        }
        catch ( std::exception & e )
        {
            return 1;
        }
    }

  private:
    auto parseAsFloat( const std::string & arrayStr, const char delimiter ) -> std::vector<float>
    {
        const auto arrayStrArray = StringUtils::split( delimiter, arrayStr );
        std::vector<float> result;
        result.reserve( arrayStrArray.size() );
        for ( const auto & powerStr : arrayStrArray )
        {
            const auto valueDbl = StringUtils::toDouble( StringUtils::trim( powerStr ) );
            // If we have a valid value
            if ( valueDbl < FLT_MAX && valueDbl > -FLT_MAX )
            {
                result.emplace_back( static_cast<float>( valueDbl ) );
            }
            // Otherwise (we got an invalid spec)
            else
            {
                std::ostringstream oss;
                oss << "value is too large or small to fit in a 32 bits float: " << valueDbl;
                AL_LOG_ERROR( oss.str() );
            }
        }
        return result;
    }

    CacheInfo mCacheInfo;
    std::string mCacheInfoStr;
    std::vector<std::string> mKeyValuePair;

    CollisionsCounter mCollisionsCounter;

    CCacheOptions mCCacheOptions;
    std::unique_ptr<CacheFileBuilder> mCacheFileBuilder;
    std::shared_ptr<BucketChainCache> mReaderBucketChainCache;
    std::shared_ptr<LinearProbeCache> mReaderLinearProbeCache;
    std::shared_ptr<LinearProbeDedupCache> mReaderLinearProbeDedupCache;
    axoncache::CacheType mCacheType{};
    int mOffsetBits{ 35 };
    std::string mLastError;
    std::vector<std::string> mDuplicateValues;

    std::string mConvertedValue;
};

typedef struct _CacheWriterHandle
{
    std::unique_ptr<AxonCacheWriter> src;
} CacheWriterHandle;

CacheWriterHandle * NewCacheWriterHandle()
{
    auto * handle = new CacheWriterHandle(); // NOLINT
    handle->src = std::make_unique<AxonCacheWriter>();

    return handle;
}

void CacheWriter_DeleteCppObject( CacheWriterHandle * handle )
{
    delete handle; // NOLINT
}

int8_t CacheWriter_Initialize( CacheWriterHandle * handle, const char * taskName, const char * settingsLocation, uint64_t numberOfKeySlots )
{
    return handle->src->initializeWriter( taskName, settingsLocation, numberOfKeySlots );
}

void CacheWriter_Finalize( CacheWriterHandle * handle )
{
    handle->src->finalizeWriter();
}

int8_t CacheWriter_InsertKey( CacheWriterHandle * handle, char * key, size_t keySize, char * value, size_t valueSize, int8_t keyType )
{
    return handle->src->insertKey( key, keySize, value, valueSize, keyType );
}

char * CacheWriter_InsertKeyWithError( CacheWriterHandle * handle, char * key, size_t keySize, char * value, size_t valueSize, int8_t keyType, int8_t * errorCode, size_t * errorMsgSize )
{
    int8_t error = 0;
    char * errorMsg = handle->src->insertKeyWithError( key, keySize, value, valueSize, keyType, error, errorMsgSize );
    if ( errorCode != nullptr )
    {
        *errorCode = error;
    }
    return errorMsg;
}

const char * CacheWriter_GetVersion( CacheWriterHandle * handle )
{
    return handle->src->getCacheVersion();
}

const char * CacheWriter_GetHashFunction( CacheWriterHandle * handle )
{
    return handle->src->getCacheHashFunction();
}

uint64_t CacheWriter_GetUniqueKeys( CacheWriterHandle * handle )
{
    return handle->src->getCacheUniqueKeys();
}

uint64_t CacheWriter_GetMaxCollisions( CacheWriterHandle * handle )
{
    return handle->src->getCacheMaxCollisions();
}

char * CacheWriter_GetCollisionsCounter( CacheWriterHandle * handle )
{
    return handle->src->getCollisionsCounter();
}

int8_t CacheWriter_FinishCacheCreation( CacheWriterHandle * handle )
{
    return handle->src->finishCacheCreation();
}

char * CacheWriter_GetLastError( CacheWriterHandle * handle )
{
    return handle->src->getLastError();
}

void CacheWriter_AddDuplicateValue( CacheWriterHandle * handle, const char * value, int8_t keyType )
{
    return handle->src->addDuplicateValue( value, keyType );
}

int8_t CacheWriter_FinishAddDuplicateValues( CacheWriterHandle * handle )
{
    return handle->src->finishAddDuplicateValues();
}

// NOLINTEND(cppcoreguidelines-pro-type-cstyle-cast)
// NOLINTEND(modernize-use-trailing-return-type)
