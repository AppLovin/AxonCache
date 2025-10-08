// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <jni.h>
#include <string>
#include <cstring>
#include "axoncache/capi/CacheReaderCApi.h"
#include "JniHelpers.h"

using axoncache::jni::convertToUtf8;
using axoncache::jni::convertToUtf16;

extern "C" {

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeNewCacheReaderHandle
 */
JNIEXPORT jlong JNICALL Java_com_applovin_axoncache_CacheReader_nativeNewCacheReaderHandle
  (JNIEnv* env, jobject obj) {
    CacheReaderHandle* handle = NewCacheReaderHandle();
    return reinterpret_cast<jlong>(handle);
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeInitialize
 */
JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeInitialize
  (JNIEnv* env, jobject obj, jlong handle, jstring taskName, jstring destinationFolder, jstring timestamp, jint isPreloadMemoryEnabled) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    std::string taskNameStr = convertToUtf8(env, taskName);
    std::string destinationFolderStr = convertToUtf8(env, destinationFolder);
    std::string timestampStr = convertToUtf8(env, timestamp);
    
    return CacheReader_Initialize(readerHandle, taskNameStr.c_str(), destinationFolderStr.c_str(), timestampStr.c_str(), isPreloadMemoryEnabled);
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeFinalize
 */
JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheReader_nativeFinalize
  (JNIEnv* env, jobject obj, jlong handle) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    CacheReader_Finalize(readerHandle);
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeDeleteCppObject
 */
JNIEXPORT void JNICALL Java_com_applovin_axoncache_CacheReader_nativeDeleteCppObject
  (JNIEnv* env, jobject obj, jlong handle) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    CacheReader_DeleteCppObject(readerHandle);
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeContainsKey
 */
JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeContainsKey
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int result = CacheReader_ContainsKey(readerHandle, reinterpret_cast<char*>(keyBytes), keySize);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetKey
 */
JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetKey
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int isExist = 0;
    int valueSize = 0;
    char* value = CacheReader_GetKey(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &isExist, &valueSize);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    
    if (isExist && value) {
        jstring result = convertToUtf16(env, value, valueSize);
        free(value);
        return result;
    }
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetLong
 */
JNIEXPORT jlong JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetLong
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize, jlong defaultValue) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int isExist = 0;
    int64_t result = CacheReader_GetLong(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &isExist, defaultValue);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetInteger
 */
JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetInteger
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize, jint defaultValue) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int isExist = 0;
    int result = CacheReader_GetInteger(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &isExist, defaultValue);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetDouble
 */
JNIEXPORT jdouble JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetDouble
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize, jdouble defaultValue) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int isExist = 0;
    double result = CacheReader_GetDouble(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &isExist, defaultValue);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetBool
 */
JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetBool
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize, jint defaultValue) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int isExist = 0;
    int result = CacheReader_GetBool(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &isExist, defaultValue);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetVector
 */
JNIEXPORT jobjectArray JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetVector
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int vectorSize = 0;
    int* valueSizes = nullptr;
    char** values = CacheReader_GetVector(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &vectorSize, &valueSizes);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    
    if (values && vectorSize > 0) {
        jclass stringClass = env->FindClass("java/lang/String");
        jobjectArray result = env->NewObjectArray(vectorSize, stringClass, nullptr);
        
        for (int i = 0; i < vectorSize; i++) {
            if (values[i]) {
                jstring str = convertToUtf16(env, values[i], valueSizes[i]);
                env->SetObjectArrayElement(result, i, str);
                env->DeleteLocalRef(str);
                free(values[i]);
            }
        }
        
        free(values);
        if (valueSizes) free(valueSizes);
        return result;
    }
    
    if (valueSizes) free(valueSizes);
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetFloatVector
 */
JNIEXPORT jfloatArray JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetFloatVector
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int vectorSize = 0;
    float* values = CacheReader_GetFloatVector(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &vectorSize);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    
    if (values && vectorSize > 0) {
        jfloatArray result = env->NewFloatArray(vectorSize);
        env->SetFloatArrayRegion(result, 0, vectorSize, values);
        free(values);
        return result;
    }
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetVectorKeySize
 */
JNIEXPORT jint JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetVectorKeySize
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int result = CacheReader_GetVectorKeySize(readerHandle, reinterpret_cast<char*>(keyBytes), keySize);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    return result;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetVectorKey
 */
JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetVectorKey
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize, jint index) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int valueSize = 0;
    char* value = CacheReader_GetVectorKey(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, index, &valueSize);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    
    if (value) {
        jstring result = convertToUtf16(env, value, valueSize);
        free(value);
        return result;
    }
    return nullptr;
}

/*
 * Class:     com_applovin_axoncache_CacheReader
 * Method:    nativeGetKeyType
 */
JNIEXPORT jstring JNICALL Java_com_applovin_axoncache_CacheReader_nativeGetKeyType
  (JNIEnv* env, jobject obj, jlong handle, jbyteArray key, jint keySize) {
    CacheReaderHandle* readerHandle = reinterpret_cast<CacheReaderHandle*>(handle);
    jbyte* keyBytes = env->GetByteArrayElements(key, nullptr);
    int valueSize = 0;
    char* value = CacheReader_GetKeyType(readerHandle, reinterpret_cast<char*>(keyBytes), keySize, &valueSize);
    env->ReleaseByteArrayElements(key, keyBytes, JNI_ABORT);
    
    if (value) {
        jstring result = convertToUtf16(env, value, valueSize);
        free(value);
        return result;
    }
    return nullptr;
}

} // extern "C"
