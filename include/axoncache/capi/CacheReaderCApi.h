// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

//
// Inspired from Datamover go binding
//
// Use functions with extern C linkage like we do for Datamover to be more C compatible
// and avoid odd compilation errors in java jni code.
//
// Some clang-tidy warnings are C++ only, so disable them
//
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)
// NOLINTBEGIN(modernize-deprecated-headers)
//

#pragma once

#include <stdlib.h>
#include <stdint.h> // for uint64_t

#ifdef __cplusplus
extern "C"
{
#endif
    // This struct is a placeholder to be cast to the C++ object type we use internally.
    typedef struct _CacheReaderHandle CacheReaderHandle;

    // Creation/Init/Deletion
    CacheReaderHandle * NewCacheReaderHandle();
    int CacheReader_Initialize( CacheReaderHandle * handle, const char * taskName, const char * destinationFolder, const char * timestamp, int isPreloadMemoryEnabled );
    void CacheReader_Finalize( CacheReaderHandle * handle );
    void CacheReader_DeleteCppObject( CacheReaderHandle * handle );

    // ccache creation (needed ?)
    void CacheReader_SetCacheType( CacheReaderHandle * handle, int cacheType );

    // Query
    int CacheReader_GetVectorKeySize( CacheReaderHandle * handle, char * key, size_t keySize );
    int CacheReader_ContainsKey( CacheReaderHandle * handle, char * key, size_t keySize );

    int64_t CacheReader_GetLong( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int64_t defaultValue );
    int CacheReader_GetInteger( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int defaultValue );
    double CacheReader_GetDouble( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, double defaultValue );
    int CacheReader_GetBool( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int defaultValue );

    // Warning for following APIs
    // Caller need to free return ptr and valueSizes ptr
    char ** CacheReader_GetVector( CacheReaderHandle * handle, char * key, size_t keySize, int * vectorSize, int ** valueSizes );
    float * CacheReader_GetFloatVector( CacheReaderHandle * handle, char * key, size_t keySize, int * vectorSize );

    char * CacheReader_GetKey( CacheReaderHandle * handle, char * key, size_t keySize, int * isExist, int * valueSize );
    char * CacheReader_GetVectorKey( CacheReaderHandle * handle, char * key, size_t keySize, int32_t index, int * valueSize );
    char * CacheReader_GetKeyType( CacheReaderHandle * handle, char * key, size_t keySize, int * valueSize );

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-deprecated-headers)
// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
