// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#pragma once

#include <cstdlib>
#include <map>

namespace axoncache
{

//statistics of the cache and input data
class CacheInfo // NOLINT
{
  public:
    CacheInfo() = default;
    ~CacheInfo() = default;

    inline void setHashTableSize( size_t hashTableSize )
    {
        mHashTableSize = hashTableSize;
    }

    inline void setCacheSize( size_t cacheSize )
    {
        mCacheSize = cacheSize;
    }

    inline void setKeyspaceSize( size_t keyspaceSize )
    {
        mKeyspaceSize = keyspaceSize;
    }

    inline void setKeyIndexUsed( size_t keyIndexUsed )
    {
        mKeyIndexUsed = keyIndexUsed;
    }

    inline void setTotalKeys( size_t totalKeys )
    {
        mTotalKeys = totalKeys;
    }

    inline void setTotalKeyLength( size_t totalKeyLength )
    {
        mTotalKeyLength = totalKeyLength;
    }

    inline void setTotalValueLength( size_t totalValueLength )
    {
        mTotalValueLength = totalValueLength;
    }

    inline void setUniqueKeys( size_t uniqueKeys )
    {
        mUniqueKeys = uniqueKeys;
    }

    inline void setMaxCollisionCount( size_t maxCollisionCount )
    {
        mMaxCollisionCount = maxCollisionCount;
    }

    inline void setMaxKeyLength( size_t maxKeyLength )
    {
        mMaxKeyLength = maxKeyLength;
    }

    inline void setMaxValueLength( size_t maxValueLength )
    {
        mMaxValueLength = maxValueLength;
    }

    inline void setMinKeyLength( size_t minKeyLength )
    {
        mMinKeyLength = minKeyLength;
    }

    inline void setMinValueLength( size_t minValueLength )
    {
        mMinValueLength = minValueLength;
    }

    inline void setVersion( float version )
    {
        mVersion = version;
    }

    inline size_t totalKeys() const
    {
        return mTotalKeys;
    }

    inline size_t uniqueKeys() const
    {
        return mUniqueKeys;
    }

    inline size_t totalKeyLength() const
    {
        return mTotalKeyLength;
    }

    inline size_t totalValueLength() const
    {
        return mTotalValueLength;
    }

    inline size_t maxKeyLength() const
    {
        return mMaxKeyLength;
    }

    inline size_t minKeyLength() const
    {
        return mMinKeyLength;
    }

    inline size_t maxValueLength() const
    {
        return mMaxValueLength;
    }

    inline size_t minValueLength() const
    {
        return mMinValueLength;
    }

    inline size_t maxCollisionCount() const
    {
        return mMaxCollisionCount;
    }

  private:
    size_t mHashTableSize;
    size_t mCacheSize;
    size_t mKeyspaceSize;
    size_t mKeyIndexUsed;
    size_t mTotalKeys;

    size_t mTotalKeyLength;
    size_t mTotalValueLength;
    size_t mUniqueKeys;
    size_t mMaxCollisionCount;
    size_t mMaxKeyLength;
    size_t mMaxValueLength;
    size_t mMinKeyLength;
    size_t mMinValueLength;

    float mVersion;

    std::map<std::string, size_t> mMalformKeyCounts;
};

}
