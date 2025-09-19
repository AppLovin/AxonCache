// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/domain/CacheValue.h"
#include <utility>
#include <stdexcept>

using namespace axoncache;

CacheValue::CacheValue( std::string_view value ) :
    mType{ CacheValueType::String }, mValue( value )
{
}
CacheValue::CacheValue( std::vector<std::string_view> value ) :
    mType{ CacheValueType::StringList }, mValue( std::move( value ) )
{
}

CacheValue::CacheValue( bool & value ) :
    mType{ CacheValueType::Bool }, mValue( value )
{
}
CacheValue::CacheValue( double & value ) :
    mType{ CacheValueType::Double }, mValue( value )
{
}
CacheValue::CacheValue( int64_t & value ) :
    mType{ CacheValueType::Int64 }, mValue( value )
{
}
CacheValue::CacheValue( std::vector<float> value ) :
    mType{ CacheValueType::FloatList }, mValue( std::move( value ) )
{
}

auto CacheValue::type() const -> CacheValueType
{
    return mType;
}

auto CacheValue::asString() const -> const std::string_view &
{
    return std::get<std::string_view>( mValue );
}

auto CacheValue::asStringList() const -> const std::vector<std::string_view> &
{
    return std::get<std::vector<std::string_view>>( mValue );
}

auto CacheValue::asBool() const -> const bool &
{
    return std::get<bool>( mValue );
}

auto CacheValue::asInt() const -> const int32_t &
{
    return std::get<int32_t>( mValue );
}

auto CacheValue::asFloat() const -> const float &
{
    return std::get<float>( mValue );
}

auto CacheValue::asDouble() const -> const double &
{
    return std::get<double>( mValue );
}

auto CacheValue::asInt64() const -> const int64_t &
{
    return std::get<int64_t>( mValue );
}

auto CacheValue::asFloatList() const -> const std::vector<float> &
{
    return std::get<std::vector<float>>( mValue );
}

auto CacheValue::toDebugString() const -> std::string
{
    std::string valueStr;
    std::string prefix;
    switch ( type() )
    {
        case CacheValueType::String:
            valueStr = "\"" + std::string{ std::get<std::string_view>( mValue ) } + "\"";
            break;
        case CacheValueType::StringList:
            valueStr += "[";
            for ( const auto & value : std::get<std::vector<std::string_view>>( mValue ) )
            {
                valueStr += prefix;
                valueStr += "\"";
                valueStr += std::string{ value };
                valueStr += "\"";
                prefix = ", ";
            }
            valueStr += "]";
            break;
        case CacheValueType::Bool:
            valueStr = asBool() ? "true" : "false";
            break;
        case CacheValueType::Int:
            valueStr = std::to_string( asInt() );
            break;
        case CacheValueType::Float:
            valueStr = std::to_string( asFloat() );
            break;
        case CacheValueType::Double:
            valueStr = std::to_string( asDouble() );
            break;
        case CacheValueType::Int64:
            valueStr = std::to_string( asInt64() );
            break;
        case CacheValueType::FloatList:
            valueStr += "[";
            for ( const auto & value : std::get<std::vector<float>>( mValue ) )
            {
                valueStr += prefix;
                valueStr += std::to_string( value );
                prefix = ", ";
            }
            valueStr += "]";
            break;
        default:
            valueStr = "null";
            break;
    }
    return R"({"type":")" + to_string( type() ) + R"(", "value":)" + valueStr + "}";
}

auto CacheValue::operator==( const CacheValue & rhs ) const -> bool
{
    if ( type() != rhs.type() )
    {
        return false;
    }

    switch ( type() )
    {
        case CacheValueType::String:
            return asString() == rhs.asString();
        case CacheValueType::StringList:
            return asStringList() == rhs.asStringList();
        case CacheValueType::Bool:
            return asBool() == rhs.asBool();
        case CacheValueType::Int:
            return asInt() == rhs.asInt();
        case CacheValueType::Float:
            return asFloat() == rhs.asFloat();
        case CacheValueType::Double:
            return asDouble() == rhs.asDouble();
        case CacheValueType::Int64:
            return asInt64() == rhs.asInt64();
        case CacheValueType::FloatList:
            return asFloatList() == rhs.asFloatList();
        default:
            break;
    }
    throw std::runtime_error( "Unknown CacheValueType = " + to_string( type() ) );
    return false;
}
