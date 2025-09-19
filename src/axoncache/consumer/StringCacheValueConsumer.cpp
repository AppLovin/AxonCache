// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/consumer/StringCacheValueConsumer.h"
#include <string_view>

using namespace axoncache;

auto StringCacheValueConsumer::consumeValue( CacheKeyValue keyValuePair ) -> PutStats
{
    if ( !mOutput.empty() )
    {
        mOutput.append( ",\n" );
    }

    mOutput.append( "\"" )
        .append( keyValuePair.first )
        .append( "\":" )
        .append( keyValuePair.second.toDebugString() );

    return PutStats();
}
