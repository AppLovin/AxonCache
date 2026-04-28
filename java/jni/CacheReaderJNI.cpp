// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <jni.h>
#include <string>
#include <cstring>
#include "axoncache/capi/CacheReaderCApi.h"
#include "JniHelpers.h"

using axoncache::jni::convertToUtf16;
using axoncache::jni::convertToUtf8;

extern "C" JNIEXPORT jlong JNICALL Java_com_applovin_axoncache_CacheReader_nativeNewCacheReaderHandle( JNIEnv * env, jobject obj )
{
    CacheReaderHandle * handle = NewCacheReaderHandle();
    return reinterpret_cast<jlong>( handle );
}

extern "C" JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeInitialize( JNIEnv * env, jobject obj, jlong handle, jstring taskName, jstring destinationFolder, jstring timestamp, jint isPreloadMemoryEnabled )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string taskNameStr = convertToUtf8( env, taskName );
    std::string destinationFolderStr = convertToUtf8( env, destinationFolder );
    std::string timestampStr = convertToUtf8( env, timestamp );

    return CacheReader_Initialize( readerHandle, taskNameStr.c_str(), destinationFolderStr.c_str(), timestampStr.c_str(), isPreloadMemoryEnabled );
}

extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheReader_nativeFinalize( JNIEnv * env, jobject obj, jlong handle )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    CacheReader_Finalize( readerHandle );
}

extern "C" JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheReader_nativeDeleteCppObject( JNIEnv * env, jobject obj, jlong handle )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    CacheReader_DeleteCppObject( readerHandle );
}

extern "C" JNIEXPORT jboolean JNICALL Java_com_applovin_axoncache_CacheReader_nativeContainsKey( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int hasKey = CacheReader_ContainsKey( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size() );
    return hasKey != 0;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetKey( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int isExist = 0;
    int valueSize = 0;
    char * value = CacheReader_GetKey( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &isExist, &valueSize );

    if ( isExist == 0 )
    {
        return nullptr; // Key not found
    }

    if ( isExist == 1 && value == nullptr )
    {
        return env->NewStringUTF( "" ); // Empty value
    }

    jstring result = convertToUtf16( env, value, valueSize );
    if ( value != nullptr )
    {
        free( value );
    }
    return result;
}

extern "C" JNIEXPORT jobject JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetLong( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int isExist = 0;
    int64_t result = CacheReader_GetLong( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &isExist, 0 );

    if ( isExist == 0 )
    {
        return nullptr;
    }

    jclass longClass = env->FindClass( "java/lang/Long" );
    if ( longClass == nullptr )
    {
        return nullptr;
    }

    jmethodID longConstructor = env->GetMethodID( longClass, "<init>", "(J)V" );
    if ( longConstructor == nullptr )
    {
        return nullptr;
    }

    return env->NewObject( longClass, longConstructor, result );
}

extern "C" JNIEXPORT jobject JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetInteger( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int isExist = 0;
    int result = CacheReader_GetInteger( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &isExist, 0 );

    if ( isExist == 0 )
    {
        return nullptr;
    }

    jclass intClass = env->FindClass( "java/lang/Integer" );
    if ( intClass == nullptr )
    {
        return nullptr;
    }

    jmethodID intConstructor = env->GetMethodID( intClass, "<init>", "(I)V" );
    if ( intConstructor == nullptr )
    {
        return nullptr;
    }

    return env->NewObject( intClass, intConstructor, result );
}

extern "C" JNIEXPORT jobject JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetDouble( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int isExist = 0;
    double result = CacheReader_GetDouble( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &isExist, 0.0 );

    if ( isExist == 0 )
    {
        return nullptr;
    }

    jclass doubleClass = env->FindClass( "java/lang/Double" );
    if ( doubleClass == nullptr )
    {
        return nullptr;
    }

    jmethodID doubleConstructor = env->GetMethodID( doubleClass, "<init>", "(D)V" );
    if ( doubleConstructor == nullptr )
    {
        return nullptr;
    }

    return env->NewObject( doubleClass, doubleConstructor, result );
}

extern "C" JNIEXPORT jobject JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetBool( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int isExist = 0;
    int result = CacheReader_GetBool( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &isExist, 0 );

    if ( isExist == 0 )
    {
        return nullptr;
    }

    jclass boolClass = env->FindClass( "java/lang/Boolean" );
    if ( boolClass == nullptr )
    {
        return nullptr;
    }

    jmethodID boolConstructor = env->GetMethodID( boolClass, "<init>", "(Z)V" );
    if ( boolConstructor == nullptr )
    {
        return nullptr;
    }

    return env->NewObject( boolClass, boolConstructor, result != 0 );
}

extern "C" JNIEXPORT jobjectArray JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetVector( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int vectorSize = 0;
    int * valueSizes = nullptr;
    char ** values = CacheReader_GetVector( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &vectorSize, &valueSizes );

    if ( values && vectorSize > 0 )
    {
        jclass stringClass = env->FindClass( "java/lang/String" );
        jobjectArray result = env->NewObjectArray( vectorSize, stringClass, nullptr );

        for ( int i = 0; i < vectorSize; i++ )
        {
            if ( values[i] )
            {
                jstring str = convertToUtf16( env, values[i], valueSizes[i] );
                env->SetObjectArrayElement( result, i, str );
                env->DeleteLocalRef( str );
                free( values[i] );
            }
        }

        free( values );
        if ( valueSizes )
            free( valueSizes );
        return result;
    }

    if ( values )
        free( values );
    if ( valueSizes )
        free( valueSizes );
    return nullptr;
}

extern "C" JNIEXPORT jfloatArray JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetFloatVector( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int vectorSize = 0;
    float * values = CacheReader_GetFloatVector( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &vectorSize );

    if ( values && vectorSize > 0 )
    {
        jfloatArray result = env->NewFloatArray( vectorSize );
        env->SetFloatArrayRegion( result, 0, vectorSize, values );
        free( values );
        return result;
    }

    if ( values )
        free( values );
    return nullptr;
}

extern "C" JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetVectorKeySize( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    return CacheReader_GetVectorKeySize( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size() );
}

extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetVectorKey( JNIEnv * env, jobject obj, jlong handle, jstring key, jint index )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int valueSize = 0;
    char * value = CacheReader_GetVectorKey( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), index, &valueSize );

    if ( value )
    {
        jstring result = convertToUtf16( env, value, valueSize );
        free( value );
        return result;
    }
    return nullptr;
}

extern "C" JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetKeyType( JNIEnv * env, jobject obj, jlong handle, jstring key )
{
    CacheReaderHandle * readerHandle = reinterpret_cast<CacheReaderHandle *>( handle );
    std::string keyStr = convertToUtf8( env, key );

    int valueSize = 0;
    char * value = CacheReader_GetKeyType( readerHandle, const_cast<char *>( keyStr.c_str() ), keyStr.size(), &valueSize );

    if ( value )
    {
        jstring result = convertToUtf16( env, value, valueSize );
        free( value );
        return result;
    }
    return nullptr;
}
