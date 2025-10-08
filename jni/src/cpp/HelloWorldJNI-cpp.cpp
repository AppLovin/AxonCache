#include <jni.h>

#include <string>
#include <axoncache/capi/CacheReaderCApi.h>

#include "utf8.h"

namespace
{
auto convertToUtf8( JNIEnv * env, jstring str ) -> std::string
{
    try
    {
        const jchar * chars = env->GetStringChars( str, nullptr );
        jsize len = env->GetStringLength( str );
        std::u16string utf16str( reinterpret_cast<const char16_t *>( chars ), len );
        env->ReleaseStringChars( str, chars );

        std::string utf8result = utf8::utf16to8( utf16str );
        return utf8result;
    }
    catch ( const std::exception & /* utfcpp_ex */ )
    {
        return "";
    }
}

auto convertToUtf16( JNIEnv * env, const char * chars, int len ) -> jstring
{
    try
    {
        std::string_view utf8str{ chars, static_cast<size_t>( len ) };
        std::u16string utf16result = utf8::utf8to16( utf8str );
        return env->NewString( reinterpret_cast<const jchar *>( utf16result.data() ), utf16result.size() );
    }
    catch ( const std::exception & /* utfcpp_ex */ )
    {
        return nullptr;
    }
}
}

JNIEXPORT jstring JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_quietReturnString( JNIEnv * env, jobject thisObject, jstring inputName )
{
    std::string name = convertToUtf8( env, inputName );
    std::string greeting = "Hello " + name + ", from C++ !!";
    return convertToUtf16( env, greeting.c_str(), greeting.size() );
}

JNIEXPORT jint JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_quietReturnInt( JNIEnv * env, jobject thisObject, jint number )
{
    return number + 1;
}

//
// Get pointer field straight from `JavaClass`
//
jfieldID getPtrFieldId( JNIEnv * env, jobject obj )
{
    static jfieldID ptrFieldId = 0;

    if ( !ptrFieldId )
    {
        auto classObj = env->GetObjectClass( obj );
        ptrFieldId = env->GetFieldID( classObj, "objPtr", "J" );
        env->DeleteLocalRef( classObj );
    }

    return ptrFieldId;
}

JNIEXPORT void JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_createHandle(
    JNIEnv * env,
    jobject thisObject )
{
    // Create a new object, and store it in this object.
    auto * handle = NewCacheReaderHandle();
    env->SetLongField( thisObject, getPtrFieldId( env, thisObject ), ( jlong )handle );
}

JNIEXPORT void JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_updateHandle(
    JNIEnv * env,
    jobject thisObject,
    jstring taskName,
    jstring destinationFolder,
    jstring timestamp )
{
    auto taskNameStr = convertToUtf8( env, taskName );
    auto destinationFolderStr = convertToUtf8( env, destinationFolder );
    auto timestampStr = convertToUtf8( env, timestamp );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );

    int isPreloadMemoryEnabled = 0;
    int8_t errorCode = CacheReader_Initialize(
        handle,
        taskNameStr.c_str(),
        destinationFolderStr.c_str(),
        timestampStr.c_str(),
        isPreloadMemoryEnabled );

    if ( errorCode != 0 )
    {
        std::string errorMsg( "init error code: " );
        errorMsg += std::to_string( errorCode );
        env->ThrowNew( env->FindClass( "java/lang/Exception" ), errorMsg.c_str() );
    }
}

JNIEXPORT void JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_initialize(
    JNIEnv * env,
    jobject thisObject,
    jstring taskName,
    jstring destinationFolder,
    jstring timestamp )
{
    // Create a new object, and store it in this object.
    auto * handle = NewCacheReaderHandle();
    env->SetLongField( thisObject, getPtrFieldId( env, thisObject ), ( jlong )handle );

    auto taskNameStr = convertToUtf8( env, taskName );
    auto destinationFolderStr = convertToUtf8( env, destinationFolder );
    auto timestampStr = convertToUtf8( env, timestamp );

    int isPreloadMemoryEnabled = 0;
    int8_t errorCode = CacheReader_Initialize(
        handle,
        taskNameStr.c_str(),
        destinationFolderStr.c_str(),
        timestampStr.c_str(),
        isPreloadMemoryEnabled );

    if ( errorCode != 0 )
    {
        std::string errorMsg( "init error code: " );
        errorMsg += std::to_string( errorCode );
        env->ThrowNew( env->FindClass( "java/lang/Exception" ), errorMsg.c_str() );
    }
}

JNIEXPORT void JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_nonIdempotentCleanup( JNIEnv * env, jobject thisObject )
{
    auto handle =
        ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );

    CacheReader_DeleteCppObject( handle );
}

JNIEXPORT jstring JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getKey( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int size = 0;
    int isExists = 0;
    auto val = CacheReader_GetKey( handle, ( char * )str.c_str(), str.size(), &isExists, &size );
    if ( isExists == 0 )
    {
        return nullptr; // missing key
    }

    if ( isExists == 1 && val == nullptr )
    {
        return env->NewStringUTF( "" ); // empty key, aka existing key with an empty value
    }

    auto javaStr = convertToUtf16( env, val, size );
    if ( val != nullptr )
    {
        free( val );
    }
    return javaStr;
}

JNIEXPORT jboolean JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_containsKey( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );

    int size = 0;
    auto val = CacheReader_ContainsKey( handle, ( char * )str.c_str(), str.size() );
    return val ? 1 : 0;
}

JNIEXPORT jobject JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getLong( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int isExist = 0;
    auto val = CacheReader_GetLong( handle, ( char * )str.c_str(), str.size(), &isExist, 0 );
    if ( !isExist )
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

    jobject longObj = env->NewObject( longClass, longConstructor, val );
    return longObj;
}

JNIEXPORT jobject JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getInteger( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int isExist = 0;
    auto val = CacheReader_GetInteger( handle, ( char * )str.c_str(), str.size(), &isExist, 0 );
    if ( !isExist )
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

    jobject intObj = env->NewObject( intClass, intConstructor, val );

    return intObj;
}

JNIEXPORT jobject JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getDouble( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int isExist = 0;
    auto val = CacheReader_GetDouble( handle, ( char * )str.c_str(), str.size(), &isExist, 0. );
    if ( !isExist )
    {
        return nullptr;
    }
    jclass doubleClass = env->FindClass( "java/lang/Double" );
    if ( doubleClass == nullptr )
    {
        return nullptr;
    }

    jmethodID initDouble = env->GetMethodID( doubleClass, "<init>", "(D)V" );
    if ( initDouble == nullptr )
    {
        return nullptr;
    }

    jobject doubleObject = env->NewObject( doubleClass, initDouble, val );
    return doubleObject;
}

JNIEXPORT jobject JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getBool( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int isExist = 0;
    auto val = CacheReader_GetBool( handle, ( char * )str.c_str(), str.size(), &isExist, false );
    if ( !isExist )
    {
        return nullptr;
    }

    jclass booleanClass = env->FindClass( "java/lang/Boolean" );
    if ( booleanClass == nullptr )
    {
        return nullptr;
    }

    jmethodID booleanConstructor = env->GetMethodID( booleanClass, "<init>", "(Z)V" );
    if ( booleanConstructor == nullptr )
    {
        return nullptr;
    }

    jobject booleanObject = env->NewObject( booleanClass, booleanConstructor, val != 0 );
    return booleanObject;
}

JNIEXPORT jobjectArray JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getVector( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int vectorSize = 0;
    int * valueSizes = nullptr;
    auto values = CacheReader_GetVector( handle, ( char * )str.c_str(), str.size(), &vectorSize, &valueSizes );
    if ( values == nullptr )
    {
        if ( valueSizes != nullptr )
        {
            free( valueSizes );
        }
        return nullptr;
    }
    jobjectArray returnObj = env->NewObjectArray( vectorSize, env->FindClass( "java/lang/String" ), env->NewStringUTF( "" ) );
    if ( returnObj != nullptr )
    {
        for ( int i = 0; i < vectorSize; i++ )
        {
            env->SetObjectArrayElement( returnObj, i, convertToUtf16( env, values[i], valueSizes[i] ) );
        }
    }
    for ( int i = 0; i < vectorSize; i++ )
    {
        if ( values[i] != nullptr )
        {
            free( values[i] );
        }
    }
    free( values );
    free( valueSizes );
    return returnObj;
}

JNIEXPORT jfloatArray JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getFloatVector( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );
    int vectorSize = 0;
    auto values = CacheReader_GetFloatVector( handle, ( char * )str.c_str(), str.size(), &vectorSize );
    if ( values == nullptr )
    {
        return nullptr;
    }
    jfloatArray returnObj = env->NewFloatArray( vectorSize );
    if ( returnObj != nullptr )
    {
        env->SetFloatArrayRegion( returnObj, 0, vectorSize, values );
    }
    if ( values != nullptr )
    {
        free( values );
    }
    return returnObj;
}

JNIEXPORT jstring JNICALL Java_com_applovin_jalcache_AlCacheJniWrapper_getKeyType( JNIEnv * env, jobject thisObject, jstring key )
{
    auto str = convertToUtf8( env, key );
    auto handle = ( CacheReaderHandle * )env->GetLongField( thisObject, getPtrFieldId( env, thisObject ) );

    int size = 0;
    auto val = CacheReader_GetKeyType( handle, ( char * )str.c_str(), str.size(), &size );
    if ( val == nullptr )
    {
        return nullptr;
    }

    auto javaStr = convertToUtf16( env, val, size );
    if ( val != nullptr )
    {
        free( val );
    }
    return javaStr;
}
