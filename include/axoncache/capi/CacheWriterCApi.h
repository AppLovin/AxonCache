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
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
    // This struct is a placeholder to be cast to the C++ object type we use internally.
    typedef struct _CacheWriterHandle CacheWriterHandle;

    // Creation/Init/Deletion
    CacheWriterHandle * NewCacheWriterHandle();
    int8_t CacheWriter_Initialize( CacheWriterHandle * handle, const char * taskName, const char * settingsLocation, uint64_t numberOfKeySlots );
    void CacheWriter_Finalize( CacheWriterHandle * handle );
    void CacheWriter_DeleteCppObject( CacheWriterHandle * handle );

    int8_t CacheWriter_InsertKey( CacheWriterHandle * handle, char * key, size_t keySize, char * value, size_t valueSize, int8_t keyType );
    void CacheWriter_AddDuplicateValue( CacheWriterHandle * handle, const char * value, int8_t keyType );
    int8_t CacheWriter_FinishAddDuplicateValues( CacheWriterHandle * handle );

    const char * CacheWriter_GetVersion( CacheWriterHandle * handle );
    const char * CacheWriter_GetHashFunction( CacheWriterHandle * handle );
    uint64_t CacheWriter_GetUniqueKeys( CacheWriterHandle * handle );
    uint64_t CacheWriter_GetMaxCollisions( CacheWriterHandle * handle );
    char * CacheWriter_GetCollisionsCounter( CacheWriterHandle * handle );
    int8_t CacheWriter_FinishCacheCreation( CacheWriterHandle * handle );
    char * CacheWriter_GetLastError( CacheWriterHandle * handle );

#ifdef __cplusplus
}
#endif

// NOLINTEND(modernize-deprecated-headers)
// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
