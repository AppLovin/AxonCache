// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <ostream>

namespace axoncache
{
enum class CacheType
{
    NONE,
    MAP,
    BUCKET_CHAIN,
    LINEAR_PROBE,
    LINEAR_PROBE_DEDUP,
    LINEAR_PROBE_DEDUP_TYPED,
};
}

[[maybe_unused]] static auto to_string( axoncache::CacheType type ) -> std::string
{
    switch ( type )
    {
        case axoncache::CacheType::NONE:
            return "NONE";
        case axoncache::CacheType::MAP:
            return "MAP";
        case axoncache::CacheType::BUCKET_CHAIN:
            return "BUCKET_CHAIN";
        case axoncache::CacheType::LINEAR_PROBE:
            return "LINEAR_PROBE";
        case axoncache::CacheType::LINEAR_PROBE_DEDUP:
            return "LINEAR_PROBE_DEDUP";
        case axoncache::CacheType::LINEAR_PROBE_DEDUP_TYPED:
            return "LINEAR_PROBE_DEDUP_TYPED";
    }
    return "NONE";
}

[[maybe_unused]] static auto operator<<( std::ostream & os, axoncache::CacheType type ) -> std::ostream &
{
    os << to_string( type );
    return os;
}
