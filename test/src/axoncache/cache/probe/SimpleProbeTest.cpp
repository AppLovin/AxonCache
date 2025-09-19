// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

#include <string_view>
#include <axoncache/cache/probe/SimpleProbe.h>
#include <doctest/doctest.h>
#include <stdint.h>
#include "axoncache/cache/CacheType.h"

using namespace axoncache;

TEST_CASE( "SimpleProbeTestLog2OfKeyWidth" )
{
    const uint16_t offsetBits = 64U;

    SimpleProbe<1> probe1( offsetBits, 1024UL );
    CHECK( probe1.log2OfKeyWidth() == 0 );

    SimpleProbe<2> probe2( offsetBits, 1024UL );
    CHECK( probe2.log2OfKeyWidth() == 1 );

    SimpleProbe<4> probe4( offsetBits, 1024UL );
    CHECK( probe4.log2OfKeyWidth() == 2 );

    SimpleProbe<8> probe8( offsetBits, 1024UL );
    CHECK( probe8.log2OfKeyWidth() == 3 );

    SimpleProbe<16> probe16( offsetBits, 1024UL );
    CHECK( probe16.log2OfKeyWidth() == 4 );
}

TEST_CASE( "SimpleProbeTestRoundPow2" )
{
    const uint16_t offsetBits = 64U;
    uint32_t collisions;
    SimpleProbe<8> probe( offsetBits, 1024UL );
    std::string_view dummy;

    CHECK( probe.numberOfKeySlots() == 1024UL );
    CHECK( probe.keySlotToPtrOffset( 0UL ) == 0UL );
    CHECK( probe.keySlotToPtrOffset( 83UL ) == ( 83UL * 8UL ) );
    CHECK( probe.calculateKeySpaceSize() == ( 1024UL * 8UL ) );
    CHECK( probe.findKeySlotOffset( dummy, 72UL, nullptr ) == ( 72UL * 8UL ) );
    CHECK( probe.findFreeKeySlotOffset( dummy, 72UL, nullptr, collisions ) == ( 72UL * 8UL ) );
}
