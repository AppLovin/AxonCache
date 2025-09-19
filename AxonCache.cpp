// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//
// Almagedon build (terms comes from SQLite which can be built the same way)
//

#include "src/axoncache/CacheGenerator.cpp"
#include "src/axoncache/builder/CacheBuilder.cpp"
#include "src/axoncache/builder/CacheFileBuilder.cpp"
#include "src/axoncache/cache/base/HashedCacheBase.cpp"
#include "src/axoncache/cache/base/MapCacheBase.cpp"
#include "src/axoncache/cache/factory/CacheFactory.cpp"
#include "src/axoncache/cache/hasher/Xxh3Hasher.cpp"
#include "src/axoncache/cache/LinearProbeDedupCache.cpp"
#include "src/axoncache/cache/probe/SimpleProbe.cpp"
#include "src/axoncache/cache/value/ChainedValue.cpp"
#include "src/axoncache/cache/value/LinearProbeValue.cpp"
#include "src/axoncache/capi/CacheReaderCApi.cpp"
#include "src/axoncache/capi/CacheWriterCApi.cpp"
#include "src/axoncache/common/xxhash.c"
#include "src/axoncache/consumer/CacheValueConsumer.cpp"
#include "src/axoncache/consumer/CacheValueConsumerBase.cpp"
#include "src/axoncache/consumer/StringCacheValueConsumer.cpp"
#include "src/axoncache/domain/CacheValue.cpp"
#include "src/axoncache/loader/CacheLoader.cpp"
#include "src/axoncache/loader/CacheOneTimeLoader.cpp"
#include "src/axoncache/logger/Logger.cpp"
#include "src/axoncache/memory/MallocMemoryHandler.cpp"
#include "src/axoncache/memory/MemoryHandler.cpp"
#include "src/axoncache/memory/MmapMemoryHandler.cpp"
#include "src/axoncache/parser/CacheValueParser.cpp"
#include "src/axoncache/reader/DataFileReader.cpp"
#include "src/axoncache/reader/DataReader.cpp"
#include "src/axoncache/transformer/StringListToString.cpp"
#include "src/axoncache/transformer/StringViewToNullTerminatedString.cpp"
#include "src/axoncache/transformer/TypeToString.cpp"
#include "src/axoncache/writer/CacheFileWriter.cpp"
#include "src/axoncache/writer/CacheWriter.cpp"
#include "src/axoncache/writer/detail/GenerateHeader.cpp"

#ifdef STANDALONE_BUILD_TEST
#include "include/axoncache/capi/CacheReader.h"

int main()
{
    auto * handle = NewCacheReaderHandle();
    // fast_cache.1700516370759.cache
    int errorCode = CacheReader_Initialize( handle,
                                            "fast_cache", "test_data", "1700516370759" );
    std::cout << "Error code: " << errorCode << std::endl;

    int size = 0;
    char * key = ( char * )"267.foo";
    auto keySize = strlen( key );
    auto * val = CacheReader_GetKey( handle, ( char * )key, keySize, &size );
    std::cout << "Key: " << key << " Value: " << val << std::endl;
    free( val );

    return 0;
}
#endif
