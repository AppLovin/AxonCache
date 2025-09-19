// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <cstdint>
#include <ostream>
#include <variant>
#include <vector>

namespace axoncache
{

enum class CacheValueType : uint8_t
{
    String = 0,
    StringList = 1,
    Bool = 2,
    Int64 = 3,
    Double = 4,
    Int = 5,
    Float = 6,
    FloatList = 7
};

using VariantType = std::variant<std::string_view, std::vector<std::string_view>, bool, int32_t, float, double, int64_t, std::vector<float>>;

class CacheValue
{
  public:
    CacheValue() = default;
    explicit CacheValue( std::string_view value );
    explicit CacheValue( std::vector<std::string_view> value );
    explicit CacheValue( bool & value );
    explicit CacheValue( int64_t & value );
    explicit CacheValue( double & value );
    explicit CacheValue( std::vector<float> value );

    [[nodiscard]] auto type() const -> CacheValueType;

    [[nodiscard]] auto asString() const -> const std::string_view &;

    [[nodiscard]] auto asStringList() const -> const std::vector<std::string_view> &;

    [[nodiscard]] auto asBool() const -> const bool &;
    [[nodiscard]] auto asInt() const -> const int32_t &;
    [[nodiscard]] auto asFloat() const -> const float &;
    [[nodiscard]] auto asDouble() const -> const double &;
    [[nodiscard]] auto asInt64() const -> const int64_t &;
    [[nodiscard]] auto asFloatList() const -> const std::vector<float> &;

    [[nodiscard]] auto operator==( const CacheValue & rhs ) const -> bool;

    [[nodiscard]] auto operator!=( const CacheValue & rhs ) const -> bool
    {
        return !( *this == rhs );
    }

    [[nodiscard]] auto toDebugString() const -> std::string;

  private:
    CacheValueType mType{};
    VariantType mValue;
};

using CacheKeyValue = std::pair<std::string_view, CacheValue>;

} // namespace axoncache

[[maybe_unused]] static auto to_string( axoncache::CacheValueType type ) -> std::string
{
    switch ( type )
    {
        case axoncache::CacheValueType::String:
            return "String";
        case axoncache::CacheValueType::StringList:
            return "StringList";
        case axoncache::CacheValueType::Bool:
            return "Bool";
        case axoncache::CacheValueType::Int:
            return "Int";
        case axoncache::CacheValueType::Float:
            return "Float";
        case axoncache::CacheValueType::Double:
            return "Double";
        case axoncache::CacheValueType::Int64:
            return "Int64";
        case axoncache::CacheValueType::FloatList:
            return "FloatList";
    }
    return "None";
}

[[maybe_unused]] static auto operator<<( std::ostream & oss, axoncache::CacheValueType type ) -> std::ostream &
{
    oss << to_string( type );
    return oss;
}

[[maybe_unused]] static auto operator<<( std::ostream & oss, const axoncache::CacheValue & value ) -> std::ostream &
{
    oss << value.toDebugString();
    return oss;
}

[[maybe_unused]] static auto operator<<( std::ostream & oss, const axoncache::CacheKeyValue & keyValue ) -> std::ostream &
{
    oss << "{\"" << keyValue.first << "\":" << keyValue.second.toDebugString() << "}";
    return oss;
}
