// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <jni.h>
#include <string>
#include <cstring>
#include "axoncache/capi/CacheWriterCApi.h"
#include "JniHelpers.h"

using axoncache::jni::convertToUtf16;
using axoncache::jni::convertToUtf8;

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeNewCacheWriterHandle
 */
extern "C" JNIEXPORT jlong JNICALL Java_com_applovin_axoncache_CacheWriter_nativeNewCacheWriterHandle( JNIEnv * env, jobject obj )
{
    CacheWriterHandle * handle = NewCacheWriterHandle();
    return reinterpret_cast<jlong>( handle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeInitialize
 */
extern "C" JNIEXPORT jbyte JNICALL Java_com_applovin_axoncache_CacheWriter_nativeInitialize( JNIEnv * env, jobject obj, jlong handle, jstring taskName, jstring settingsLocation, jlong numberOfKeySlots )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    std::string taskNameStr = convertToUtf8( env, taskName );
    std::string settingsLocationStr = convertToUtf8( env, settingsLocation );

    return CacheWriter_Initialize( writerHandle, taskNameStr.c_str(), settingsLocationStr.c_str(), numberOfKeySlots );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeFinalize
 */
extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheWriter_nativeFinalize( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    CacheWriter_Finalize( writerHandle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeDeleteCppObject
 */
extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheWriter_nativeDeleteCppObject( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    CacheWriter_DeleteCppObject( writerHandle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeInsertKey
 */
extern "C" JNIEXPORT jbyte JNICALL Java_com_applovin_axoncache_CacheWriter_nativeInsertKey( JNIEnv * env, jobject obj, jlong handle, jbyteArray key, jint keySize, jbyteArray value, jint valueSize, jbyte keyType )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    jbyte * keyBytes = env->GetByteArrayElements( key, nullptr );
    jbyte * valueBytes = env->GetByteArrayElements( value, nullptr );

    int8_t result = CacheWriter_InsertKey( writerHandle,
                                           reinterpret_cast<char *>( keyBytes ), keySize,
                                           reinterpret_cast<char *>( valueBytes ), valueSize,
                                           keyType );

    env->ReleaseByteArrayElements( key, keyBytes, JNI_ABORT );
    env->ReleaseByteArrayElements( value, valueBytes, JNI_ABORT );
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeAddDuplicateValue
 */
extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheWriter_nativeAddDuplicateValue( JNIEnv * env, jobject obj, jlong handle, jstring value, jbyte keyType )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    std::string valueStr = convertToUtf8( env, value );
    CacheWriter_AddDuplicateValue( writerHandle, valueStr.c_str(), keyType );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeFinishAddDuplicateValues
 */
extern "C" JNIEXPORT jbyte JNICALL Java_com_applovin_axoncache_CacheWriter_nativeFinishAddDuplicateValues( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    return CacheWriter_FinishAddDuplicateValues( writerHandle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeGetVersion
 */
extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheWriter_nativeGetVersion( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    const char * version = CacheWriter_GetVersion( writerHandle );
    if ( version )
    {
        jstring result = env->NewStringUTF( version );
        free( const_cast<char *>( version ) );
        return result;
    }
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeGetHashFunction
 */
extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheWriter_nativeGetHashFunction( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    const char * hashFunc = CacheWriter_GetHashFunction( writerHandle );
    if ( hashFunc )
    {
        jstring result = env->NewStringUTF( hashFunc );
        free( const_cast<char *>( hashFunc ) );
        return result;
    }
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeGetUniqueKeys
 */
extern "C" JNIEXPORT jlong JNICALL Java_com_applovin_axoncache_CacheWriter_nativeGetUniqueKeys( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    return CacheWriter_GetUniqueKeys( writerHandle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeGetMaxCollisions
 */
extern "C" JNIEXPORT jlong JNICALL Java_com_applovin_axoncache_CacheWriter_nativeGetMaxCollisions( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    return CacheWriter_GetMaxCollisions( writerHandle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeGetCollisionsCounter
 */
extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheWriter_nativeGetCollisionsCounter( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    char * collisions = CacheWriter_GetCollisionsCounter( writerHandle );
    if ( collisions )
    {
        jstring result = env->NewStringUTF( collisions );
        free( collisions );
        return result;
    }
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeFinishCacheCreation
 */
extern "C" JNIEXPORT jbyte JNICALL Java_com_applovin_axoncache_CacheWriter_nativeFinishCacheCreation( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    return CacheWriter_FinishCacheCreation( writerHandle );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeSetCacheType
 */
extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheWriter_nativeSetCacheType( JNIEnv * env, jobject obj, jlong handle, jint cacheType )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    CacheWriter_SetCacheType( writerHandle, cacheType );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeSetOffsetBits
 */
extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheWriter_nativeSetOffsetBits( JNIEnv * env, jobject obj, jlong handle, jint offsetBits )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    CacheWriter_SetOffsetBits( writerHandle, offsetBits );
}

/*
 * Class:     com_applovin_axoncache_CacheWriter
 * Method:    nativeGetLastError
 */
extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheWriter_nativeGetLastError( JNIEnv * env, jobject obj, jlong handle )
{
    CacheWriterHandle * writerHandle = reinterpret_cast<CacheWriterHandle *>( handle );
    char * error = CacheWriter_GetLastError( writerHandle );
    if ( error && strlen( error ) > 0 )
    {
        jstring result = env->NewStringUTF( error );
        free( error );
        return result;
    }
    if ( error )
        free( error );
    return nullptr;
}
