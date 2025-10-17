// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <jni.h>
#include <string>
#include <string_view>
#include <utf8.h>

namespace axoncache::jni
{

inline auto convertToUtf8( JNIEnv * env, jstring jstr ) -> std::string
{
    if ( !jstr )
    {
        return "";
    }

    try
    {
        const jchar * chars = env->GetStringChars( jstr, nullptr );
        if ( !chars )
        {
            return "";
        }

        jsize len = env->GetStringLength( jstr );
        std::u16string utf16str( reinterpret_cast<const char16_t *>( chars ), len );
        env->ReleaseStringChars( jstr, chars );

        std::string utf8result;
        utf8::utf16to8( utf16str.begin(), utf16str.end(), std::back_inserter( utf8result ) );
        return utf8result;
    }
    catch ( const std::exception & /* utfcpp_ex */ )
    {
        return "";
    }
}

inline auto convertToUtf16( JNIEnv * env, std::string_view str ) -> jstring
{
    if ( str.empty() )
    {
        return env->NewStringUTF( "" );
    }

    try
    {
        std::u16string utf16result;
        utf8::utf8to16( str.begin(), str.end(), std::back_inserter( utf16result ) );
        return env->NewString( reinterpret_cast<const jchar *>( utf16result.data() ),
                               static_cast<jsize>( utf16result.size() ) );
    }
    catch ( const std::exception & /* utfcpp_ex */ )
    {
        return nullptr;
    }
}

inline auto convertToUtf16( JNIEnv * env, const char * chars, int len ) -> jstring
{
    if ( !chars || len <= 0 )
    {
        return nullptr;
    }

    return convertToUtf16( env, std::string_view{ chars, static_cast<size_t>( len ) } );
}

inline auto convertToUtf16( JNIEnv * env, const char * chars ) -> jstring
{
    if ( !chars )
    {
        return nullptr;
    }

    return convertToUtf16( env, std::string_view{ chars } );
}

} // namespace axoncache::jni
