// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/consumer/CacheValueConsumer.h"
#include <stdexcept>
#include <string>

using namespace axoncache;

CacheValueConsumer::CacheValueConsumer( CacheBase * cache ) :
    mCache( cache )
{
}

auto CacheValueConsumer::consumeValue( CacheKeyValue keyValuePair ) -> PutStats
{
    switch ( keyValuePair.second.type() )
    {
        case CacheValueType::String:
            return cache()->put( keyValuePair.first, keyValuePair.second.asString() );

        case CacheValueType::StringList:
            return cache()->put( keyValuePair.first, keyValuePair.second.asStringList() );

        case CacheValueType::Bool:
        {
            auto value = keyValuePair.second.asBool();
            return cache()->put( keyValuePair.first, value );
        }

        case CacheValueType::Double:
        {
            auto value = keyValuePair.second.asDouble();
            return cache()->put( keyValuePair.first, value );
        }

        case CacheValueType::Int64:
        {
            auto value = keyValuePair.second.asInt64();
            return cache()->put( keyValuePair.first, value );
        }

        case CacheValueType::FloatList:
            return cache()->put( keyValuePair.first, keyValuePair.second.asFloatList() );

        default:
            break;
    }

    throw std::runtime_error( "Unknown CacheValueType : " + to_string( keyValuePair.second.type() ) );
    return PutStats();
}
