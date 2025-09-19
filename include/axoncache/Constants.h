// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string_view>
#include <string>
#include "axoncache/cache/CacheType.h"
#include <cinttypes>

namespace axoncache::Constants
{
[[maybe_unused]] constexpr auto kSeviceName = "axoncache";
[[maybe_unused]] constexpr auto kDefaultMaxValuesRead = 128UL;
[[maybe_unused]] constexpr std::string_view kCacheFileNameSuffix = ".cache";
[[maybe_unused]] constexpr std::string_view kLatestTimestampFileNameSuffix = ".timestamp.latest";
[[maybe_unused]] constexpr uint16_t kCacheHeaderMagicNumber = 37460;
[[maybe_unused]] constexpr uint16_t kMinLinearProbeOffsetBits = 16; // 65KB
[[maybe_unused]] constexpr uint16_t kMaxLinearProbeOffsetBits = 38; // reference upto 274,877,906,944, i.e. 274GB
[[maybe_unused]] constexpr unsigned long kMaxCacheNameSize = 32;

namespace ProbeStatus
{
[[maybe_unused]] constexpr int64_t AXONCACHE_KEY_NOT_FOUND = -1;
[[maybe_unused]] constexpr int64_t AXONCACHE_KEY_EXISTS = -2;
}

namespace HashFuncId
{
[[maybe_unused]] constexpr uint16_t UNKNOWN = 0;
[[maybe_unused]] constexpr uint16_t XXHASH64 = 1;
[[maybe_unused]] constexpr uint16_t XXH3 = 2;
}

namespace ConfKey
{
[[maybe_unused]] const std::string kLogLocation = "server.log.location";
[[maybe_unused]] const std::string kErrorLogLocation = "server.errorlog.location";
[[maybe_unused]] const std::string kCacheNames = "axoncache.names";
[[maybe_unused]] const std::string kCacheType = "axoncache.type";
[[maybe_unused]] const std::string kOutputDir = "axoncache.output_dir";
[[maybe_unused]] const std::string kInputDir = "axoncache.input_dir";
[[maybe_unused]] const std::string kLoadDir = "axoncache.load_dir";
[[maybe_unused]] const std::string kInputFiles = "axoncache.input_files";
[[maybe_unused]] const std::string kOffsetBits = "axoncache.offset_bits";
[[maybe_unused]] const std::string kKeySlots = "axoncache.key_slots";
[[maybe_unused]] const std::string kCacheUpdateIntervalMs = "axoncache.update_interval_ms";
[[maybe_unused]] const std::string kMaxLoadFactor = "axoncache.max_load_factor"; // < 0.8. preferably 0.5 for linear probe

[[maybe_unused]] const std::string kControlCharLine = "axoncache.control_char.line";
[[maybe_unused]] const std::string kControlCharKeyValue = "axoncache.control_char.key_value";
[[maybe_unused]] const std::string kControlCharVectorType = "axoncache.control_char.vector_type";
[[maybe_unused]] const std::string kControlCharVectorElem = "axoncache.control_char.vector_elem";
}

namespace ConfDefault
{
[[maybe_unused]] constexpr std::string_view kConfigLocation = "/etc/applovin/axoncache/axoncache.properties";
[[maybe_unused]] constexpr std::string_view kLogLocation = "/var/log/applovin/axoncache.log";
[[maybe_unused]] constexpr std::string_view kErrorLogLocation = "/var/log/applovin/axoncache-error.log";
[[maybe_unused]] constexpr std::string_view kCacheNames = "axoncache";
[[maybe_unused]] constexpr std::string_view kOutputDir = "/var/lib/applovin/datamover";
[[maybe_unused]] constexpr uint16_t kLinearProbeOffsetBits = 35;
[[maybe_unused]] constexpr uint16_t kBucketChainOffsetBits = 64;
[[maybe_unused]] constexpr uint64_t kKeySlots = 1024;
[[maybe_unused]] constexpr std::string_view kLoadDir = "/var/lib/applovin/datamover";
[[maybe_unused]] constexpr std::string_view kInputDir = "/var/lib/applovin/datamover";
[[maybe_unused]] constexpr uint32_t kCacheType = static_cast<uint32_t>( CacheType::BUCKET_CHAIN );
[[maybe_unused]] constexpr uint64_t kMemoryCapcityBytes = 1024;
[[maybe_unused]] constexpr uint64_t kCacheUpdateIntervalMs = 300000UL; // 5 mins
[[maybe_unused]] constexpr double kLinearProbeMaxLoadFactor = 0.8;
[[maybe_unused]] constexpr double kMaxLoadFactor = 0.5;

[[maybe_unused]] constexpr char kControlCharLine = '\036';
[[maybe_unused]] constexpr char kControlCharKeyValue = '=';
[[maybe_unused]] constexpr char kControlCharVectorType = 1;
[[maybe_unused]] constexpr char kControlCharVectorElem = '|';
}

namespace Limit
{
[[maybe_unused]] constexpr uint64_t kKeyLength = ( 1 << 16 ) - 1;
[[maybe_unused]] constexpr uint64_t kVectorLength = ( 1 << 16 ) - 1;
[[maybe_unused]] constexpr uint64_t kVectorElementLength = ( 1 << 16 ) - 1;
[[maybe_unused]] constexpr uint64_t kValueLength = ( 1 << 24 ) - 1;
}
}
