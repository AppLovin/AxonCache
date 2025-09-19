// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string_view>
#include <axoncache/cache/probe/LinearProbe.h>
#include <axoncache/memory/MallocMemoryHandler.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include "axoncache/Constants.h"
#include "axoncache/Math.h"
#include "axoncache/memory/MemoryHandler.h"

using namespace axoncache;

TEST_CASE( "LinearProbeTestLog2OfKeyWidth" )
{
    // We only accept KeyWidth == 8
    const uint16_t offsetBits = 35U;
    LinearProbe<8> probe8( offsetBits, 1024UL );
    CHECK( probe8.log2OfKeyWidth() == 3 );
    CHECK( probe8.hashcodeBits() == 29 );
    CHECK( probe8.offsetBits() == 35 );
}

TEST_CASE( "LinearProbeTestInvalidOffsetBits" )
{
    for ( auto offsetBits = 0; offsetBits < Constants::kMinLinearProbeOffsetBits; offsetBits++ )
    {
        CHECK_THROWS_WITH( LinearProbe<8> probe8( offsetBits, 1024UL ), "offset bits must in range of [ 16, 38 ]" );
    }
    for ( auto offsetBits = Constants::kMaxLinearProbeOffsetBits + 1; offsetBits < 65; offsetBits++ )
    {
        CHECK_THROWS_WITH( LinearProbe<8> probe8( offsetBits, 1024UL ), "offset bits must in range of [ 16, 38 ]" );
    }
}

TEST_CASE( "LinearProbeTestBitMasks" )
{
    struct LinearProbeBits_t
    {
        uint16_t hashcodeBits;
        uint16_t offsetBits;
        uint64_t hashcodeMask;
        uint64_t offsetMask;
    } expected[] = {
        { 48, 16, 0xFFFFFFFFFFFF0000, 0x000000000000FFFF },
        { 47, 17, 0xFFFFFFFFFFFE0000, 0x000000000001FFFF },
        { 46, 18, 0xFFFFFFFFFFFC0000, 0x000000000003FFFF },
        { 45, 19, 0xFFFFFFFFFFF80000, 0x000000000007FFFF },
        { 44, 20, 0xFFFFFFFFFFF00000, 0x00000000000FFFFF },
        { 43, 21, 0xFFFFFFFFFFE00000, 0x00000000001FFFFF },
        { 42, 22, 0xFFFFFFFFFFC00000, 0x00000000003FFFFF },
        { 41, 23, 0xFFFFFFFFFF800000, 0x00000000007FFFFF },
        { 40, 24, 0xFFFFFFFFFF000000, 0x0000000000FFFFFF },
        { 39, 25, 0xFFFFFFFFFE000000, 0x0000000001FFFFFF },
        { 38, 26, 0xFFFFFFFFFC000000, 0x0000000003FFFFFF },
        { 37, 27, 0xFFFFFFFFF8000000, 0x0000000007FFFFFF },
        { 36, 28, 0xFFFFFFFFF0000000, 0x000000000FFFFFFF },
        { 35, 29, 0xFFFFFFFFE0000000, 0x000000001FFFFFFF },
        { 34, 30, 0xFFFFFFFFC0000000, 0x000000003FFFFFFF },
        { 33, 31, 0xFFFFFFFF80000000, 0x000000007FFFFFFF },
        { 32, 32, 0xFFFFFFFF00000000, 0x00000000FFFFFFFF },
        { 31, 33, 0xFFFFFFFE00000000, 0x00000001FFFFFFFF },
        { 30, 34, 0xFFFFFFFC00000000, 0x00000003FFFFFFFF },
        { 29, 35, 0xFFFFFFF800000000, 0x00000007FFFFFFFF },
        { 28, 36, 0xFFFFFFF000000000, 0x0000000FFFFFFFFF },
        { 27, 37, 0xFFFFFFE000000000, 0x0000001FFFFFFFFF },
        { 26, 38, 0xFFFFFFC000000000, 0x0000003FFFFFFFFF },
    };

    for ( uint16_t i = 0, offsetBits = Constants::kMinLinearProbeOffsetBits; offsetBits <= Constants::kMaxLinearProbeOffsetBits; i++, offsetBits++ )
    {
        LinearProbe<8> probe8( offsetBits, 1024UL );
        CHECK( expected[i].hashcodeBits == probe8.hashcodeBits() );
        CHECK( expected[i].offsetBits == probe8.offsetBits() );
        CHECK( expected[i].hashcodeMask == probe8.hashcodeMask() );
        CHECK( expected[i].offsetMask == probe8.offsetMask() );
    }
}

TEST_CASE( "LinearProbeTestRoundPow2" )
{
    CHECK( math::roundUpToPowerOfTwo( 0 ) == 1 );
    CHECK( math::roundUpToPowerOfTwo( 1 ) == 1 );
    CHECK( math::roundUpToPowerOfTwo( 2 ) == 2 );
    CHECK( math::roundUpToPowerOfTwo( 3 ) == 4 );
    CHECK( math::roundUpToPowerOfTwo( 13 ) == 16 );
    CHECK( math::roundUpToPowerOfTwo( 16 ) == 16 );
}

TEST_CASE( "LinearProbeTestRoundPow2" )
{
    const uint16_t offsetBits = 35U;
    uint32_t collisions;
    MallocMemoryHandler memory( 1000UL * sizeof( uint64_t ) );
    std::string_view dummy;
    LinearProbe<8> probe( offsetBits, 1024UL );

    CHECK( probe.numberOfKeySlots() == 1024UL );
    CHECK( probe.keySlotToPtrOffset( 0UL ) == 0UL );
    CHECK( probe.keySlotToPtrOffset( 83UL ) == ( 83UL * 8UL ) );
    CHECK( probe.calculateKeySpaceSize() == ( 1024UL * 8UL ) );
    CHECK( probe.findFreeKeySlotOffset( dummy, 72UL, memory.data(), collisions ) == ( 72UL * 8UL ) );
}
