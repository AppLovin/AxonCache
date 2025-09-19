// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include "axoncache/cache/base/HashedCacheBase.h"
#include "axoncache/cache/hasher/Xxh3Hasher.h"
#include "axoncache/cache/probe/LinearProbe.h"
#include "axoncache/cache/value/LinearProbeValue.h"
#include "axoncache/cache/probe/SimpleProbe.h"
#include "axoncache/cache/value/ChainedValue.h"
#include "axoncache/logger/Logger.h"
#include "axoncache/common/StringUtils.h"
#include "axoncache/common/StringViewUtils.h"
#include <sstream>

using namespace axoncache;

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getString( std::string_view key, std::string_view defaultValue ) const -> std::pair<std::string_view, bool>
{
    bool isExist = false;
    auto str = getInternal( key, CacheValueType::String, &isExist );
    if ( !isExist )
    {
        return std::make_pair( defaultValue, false );
    }
    else
    {
        return std::make_pair( StringViewToNullTerminatedString::trimExtraNullTerminator( str ), true );
    }
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getBool( std::string_view key, bool defaultValue ) const -> std::pair<bool, bool>
{
    const auto [str, type] = getWithType( key );
    if ( str.empty() )
    {
        return std::make_pair( defaultValue, false );
    }
    if ( type == CacheValueType::Bool )
    {
        return std::make_pair( transform<bool>( str ), true );
    }
    else if ( type == CacheValueType::Int64 )
    {
        // Bool are passed in as integers in populate now
        return std::make_pair( transform<int64_t>( str ) != 0LL, true );
    }
    else if ( type == CacheValueType::String )
    {
        return std::make_pair( StringUtils::toBool( str ), true );
    }
    else
    {
        std::ostringstream oss;
        oss << "Type mismatch for key " << key
            << " expected " << to_string( CacheValueType::Bool )
            << " type in cache was " << to_string( type );
        AL_LOG_ERROR( oss.str() );

        return std::make_pair( defaultValue, false );
    }
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getInt64( std::string_view key, int64_t defaultValue ) const -> std::pair<int64_t, bool>
{
    const auto [str, type] = getWithType( key );
    if ( str.empty() )
    {
        return std::make_pair( defaultValue, false );
    }
    if ( type == CacheValueType::Int64 )
    {
        return std::make_pair( transform<int64_t>( str ), true );
    }
    else if ( type == CacheValueType::String )
    {

        return std::make_pair( StringUtils::toLong( str ), true );
    }
    else
    {
        std::ostringstream oss;
        oss << "Type mismatch for key " << key
            << " expected " << to_string( CacheValueType::Int64 )
            << " type in cache was " << to_string( type );
        AL_LOG_ERROR( oss.str() );

        return std::make_pair( defaultValue, false );
    }
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getDouble( std::string_view key, double defaultValue ) const -> std::pair<double, bool>
{
    const auto [str, type] = getWithType( key );
    if ( str.empty() )
    {
        return std::make_pair( defaultValue, false );
    }
    if ( type == CacheValueType::Double )
    {
        return std::make_pair( transform<double>( str ), true );
    }
    else if ( type == CacheValueType::String )
    {
        return std::make_pair( StringUtils::toDouble( str ), true );
    }
    else
    {
        std::ostringstream oss;
        oss << "Type mismatch for key " << key
            << " expected " << to_string( CacheValueType::Float )
            << " type in cache was " << to_string( type );
        AL_LOG_ERROR( oss.str() );

        return std::make_pair( defaultValue, false );
    }
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t>
{
    if ( this->mHeader.numberOfEntries >= this->mMaxNumberOfEntries ) // linear requires more empty slots for efficient get operations
    {
        std::ostringstream oss;
        oss << "keySpace is full, numOfEntries=" << this->mHeader.numberOfEntries
            << " numberOfKeySlots=" << this->numberOfKeySlots()
            << " maxLoadFactor=" << mHeader.maxLoadFactor;
        AL_LOG_ERROR( oss.str() );

        throw std::runtime_error( "keySpace is full" );
    }

    uint32_t collisions = 0;
    auto hashcode = HashAlgo::hash( key );
    auto keySlotOffset = this->mProbe.findFreeKeySlotOffset( key, hashcode, this->mKeySpacePtr, collisions );
    if ( keySlotOffset != Constants::ProbeStatus::AXONCACHE_KEY_EXISTS )
    {
        collisions = std::max( collisions, this->mValueMgr.add( keySlotOffset, key, hashcode, static_cast<uint8_t>( type ), value, this->mutableMemoryHandler() ) );
        this->mHeader.maxCollisions = std::max( collisions, this->mHeader.maxCollisions );
        ++( this->mHeader.numberOfEntries );
        // Data ptr could have changed, so update the pointer
        this->updateKeySpacePtr();
        return std::make_pair( true, collisions );
    }

    return std::make_pair( false, collisions );
}
template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getFloatVector( std::string_view key ) const -> std::vector<float>
{
    const auto [value, type] = getWithTypeInternal( key );
    if ( !value.empty() )
    {
        switch ( type )
        {
            case CacheValueType::String:
                return axoncache::stringViewToVector<float>( value, ':', value.size() );
            case CacheValueType::FloatList:
                return transform<std::vector<float>>( value );
            default:
            {
                std::ostringstream oss;
                oss << "Type mismatch for key " << key
                    << " expected " << to_string( CacheValueType::FloatList )
                    << " type in cache was " << to_string( type );
                AL_LOG_ERROR( oss.str() );
            }
        }
    }
    return std::vector<float>{};
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getFloatSpan( std::string_view key ) const -> std::span<const float>
{
    const auto [value, type] = getWithTypeInternal( key );
    if ( !value.empty() )
    {
        switch ( type )
        {
            case CacheValueType::String:
                throw std::runtime_error( "Cache value type is string, please use getFloatVector instead to explicitly convert it to a float vector" );
            case CacheValueType::FloatList:
                return transform<std::span<const float>>( value );
            default:
            {
                std::ostringstream oss;
                oss << "Type mismatch for key " << key
                    << " expected " << to_string( CacheValueType::FloatList )
                    << " type in cache was " << to_string( type );
                AL_LOG_ERROR( oss.str() );
            }
        }
    }
    return std::span<const float>{};
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::readKey( std::string_view key ) -> std::string_view
{
    const auto [value, type] = getWithTypeInternal( key );
    if ( value.empty() )
    {
        return {};
    }
    switch ( type )
    {
        case CacheValueType::String:
            return StringViewToNullTerminatedString::trimExtraNullTerminator( value );
        case CacheValueType::StringList:
        {
            auto values = StringListToString::transform( value );
            if ( values.size() == 1U )
            {
                return values[0];
            }

            std::ostringstream oss;
            oss << "key : " << key
                << " contains StringList with " << values.size() << " elements";
            AL_LOG_ERROR( oss.str() );

            break;
        }
        default:
        {
            std::ostringstream oss;
            oss << "key : " << key
                << " with type " << to_string( type )
                << " is not stored as String, or StringList type";
            AL_LOG_ERROR( oss.str() );
        }
    }
    return {};
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::readKeys( std::string_view key ) -> std::vector<std::string_view>
{
    const auto [value, type] = getWithTypeInternal( key );
    if ( value.empty() )
    {
        return {};
    }
    switch ( type )
    {
        case CacheValueType::String:
            return std::vector<std::string_view>( 1, StringViewToNullTerminatedString::trimExtraNullTerminator( value ) );
        case CacheValueType::StringList:
            return StringListToString::transform( value );
        default:
        {
            std::ostringstream oss;
            oss << "key : " << key
                << " with type " << to_string( type )
                << " is not stored as String, or StringList type";
            AL_LOG_ERROR( oss.str() );
        }
    }
    return {};
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getFloatAtIndices( std::string_view key, const std::vector<int32_t> & indices ) const -> std::vector<float>
{
    std::vector<float> result( indices.size(), 0.f );
    const auto [value, type] = getWithTypeInternal( key );
    if ( !value.empty() )
    {
        switch ( type )
        {
            case CacheValueType::String:
            {
                auto values = axoncache::stringViewToVector<float>( value, ':', value.size() );
                for ( size_t at = 0U; at < indices.size(); at++ )
                {
                    if ( 0 <= indices[at] && indices[at] < static_cast<int32_t>( values.size() ) )
                    {
                        result[at] = values[indices[at]];
                    }
                }
                return result;
            }
            case CacheValueType::FloatList:
            {
                for ( size_t at = 0U; at < indices.size(); at++ )
                {
                    size_t offset = indices[at] * sizeof( float );
                    if ( 0 <= indices[at] && offset < value.size() )
                    {
                        result[at] = *( float * )( value.data() + offset );
                    }
                }
                return result;
            }
            default:
            {
                std::ostringstream oss;
                oss << "Type mismatch for key " << key
                    << " expected " << to_string( CacheValueType::FloatList )
                    << " type in cache was " << to_string( type );
                AL_LOG_ERROR( oss.str() );

                return result;
            }
        }
    }

    return result;
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getFloatAtIndex( std::string_view key, int32_t index ) const -> float
{
    const auto [value, type] = getWithTypeInternal( key );
    if ( !value.empty() )
    {
        switch ( type )
        {
            case CacheValueType::String:
            {
                auto values = axoncache::stringViewToVector<float>( value, ':', value.size() );
                return ( 0 <= index && index < static_cast<int32_t>( values.size() ) ) ? values[index] : 0.f;
            };
            case CacheValueType::FloatList:
            {
                size_t offset = index * sizeof( float );
                return ( 0 <= index && offset < value.size() ) ? *( float * )( value.data() + offset ) : 0.f;
            }
            default:
            {
                std::ostringstream oss;
                oss << "Type mismatch for key " << key
                    << " expected " << to_string( CacheValueType::FloatList )
                    << " type in cache was " << to_string( type );
                AL_LOG_ERROR( oss.str() );

                return 0.f;
            }
        }
    }
    return 0.f;
}

template<typename HashAlgo, typename Probe, typename ValueMgr, CacheType CacheTypeVal>
auto HashedCacheBase<HashAlgo, Probe, ValueMgr, CacheTypeVal>::getKeyType( std::string_view key ) const -> std::string
{
    const auto [value, type] = getWithTypeInternal( key );
    if ( value.empty() )
    {
        return {};
    }

    auto strType = to_string( type );
    if ( strType == "None" )
    {
        std::ostringstream oss;
        oss << "Unknown type " << static_cast<int>( type )
            << " key " << key;
        AL_LOG_ERROR( oss.str() );

        return {};
    }
    return strType;
}

// Explicitly instantiate the template for the types we use
// that way we can keep out StringUtils from the header class
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getString( std::string_view, std::string_view ) const -> std::pair<std::string_view, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getBool( std::string_view, bool ) const -> std::pair<bool, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getInt64( std::string_view, int64_t ) const -> std::pair<int64_t, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getDouble( std::string_view, double ) const -> std::pair<double, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getFloatVector( std::string_view key ) const -> std::vector<float>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getFloatSpan( std::string_view key ) const -> std::span<const float>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    readKey( std::string_view key ) -> std::string_view;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    readKeys( std::string_view key ) -> std::vector<std::string_view>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getFloatAtIndices( std::string_view key, const std::vector<int32_t> & indices ) const -> std::vector<float>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getFloatAtIndex( std::string_view key, int32_t index ) const -> float;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    getKeyType( std::string_view key ) const -> std::string;

template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )2>::
    getString( std::string_view, std::string_view ) const -> std::pair<std::string_view, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getBool( std::string_view, bool ) const -> std::pair<bool, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getInt64( std::string_view, int64_t ) const -> std::pair<int64_t, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getDouble( std::string_view, double ) const -> std::pair<double, bool>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getFloatVector( std::string_view key ) const -> std::vector<float>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getFloatSpan( std::string_view key ) const -> std::span<const float>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    readKey( std::string_view key ) -> std::string_view;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    readKeys( std::string_view key ) -> std::vector<std::string_view>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getFloatAtIndices( std::string_view key, const std::vector<int32_t> & indices ) const -> std::vector<float>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getFloatAtIndex( std::string_view key, int32_t index ) const -> float;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    getKeyType( std::string_view key ) const -> std::string;

template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::LinearProbe<8u>, axoncache::LinearProbeValue, ( axoncache::CacheType )3>::
    putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )3>::
    putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t>;
template auto axoncache::HashedCacheBase<axoncache::Xxh3Hasher, axoncache::SimpleProbe<8u>, axoncache::ChainedValue, ( axoncache::CacheType )2>::
    putInternal( std::string_view key, CacheValueType type, std::string_view value ) -> std::pair<bool, uint32_t>;
