/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2026 Igal Alkon <igal@alkontek.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Corrade/Containers/BitArray.h"
#include "Corrade/TestSuite/Tester.h"

namespace Corrade { namespace Containers { namespace Test { namespace {

/* 64 bits is in-place on both 32- and 64-bit. InPlaceBits is the in-place
   limit: 184 on 64-bit, 88 on 32-bit. 1024 is always on the heap. */
constexpr std::size_t InPlaceBits =
    #ifdef CORRADE_TARGET_32BIT
    88
    #else
    184
    #endif
    ;

struct BitArrayBenchmark: TestSuite::Tester {
    explicit BitArrayBenchmark();

    void constructValueInit64();
    void constructValueInitInPlace();
    void constructValueInit1024();

    void setGet64();
    void setGetInPlace();
    void setGet1024();

    void manyInPlace();
};

constexpr std::size_t Repeats = 1024;

BitArrayBenchmark::BitArrayBenchmark() {
    addBenchmarks({&BitArrayBenchmark::constructValueInit64,
                   &BitArrayBenchmark::constructValueInitInPlace,
                   &BitArrayBenchmark::constructValueInit1024,

                   &BitArrayBenchmark::setGet64,
                   &BitArrayBenchmark::setGetInPlace,
                   &BitArrayBenchmark::setGet1024,

                   &BitArrayBenchmark::manyInPlace}, 50);
}

void BitArrayBenchmark::constructValueInit64() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, 64};
        sum += a.size();
    }
    CORRADE_COMPARE(sum, Repeats*64);
}

void BitArrayBenchmark::constructValueInitInPlace() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, InPlaceBits};
        sum += a.size();
    }
    CORRADE_COMPARE(sum, Repeats*InPlaceBits);
}

void BitArrayBenchmark::constructValueInit1024() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, 1024};
        sum += a.size();
    }
    CORRADE_COMPARE(sum, Repeats*1024);
}

void BitArrayBenchmark::setGet64() {
    BitArray a{Corrade::ValueInit, 64};
    volatile std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        for(std::size_t i = 0; i != 64; ++i)
            a.set(i, static_cast<bool>(i & 1));
        for(std::size_t i = 0; i != 64; ++i)
            sum += a[i];
    }
    CORRADE_COMPARE(static_cast<std::size_t>(sum), Repeats*32);
}

void BitArrayBenchmark::setGetInPlace() {
    BitArray a{Corrade::ValueInit, InPlaceBits};
    volatile std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        for(std::size_t i = 0; i != InPlaceBits; ++i)
            a.set(i, static_cast<bool>(i & 1));
        for(std::size_t i = 0; i != InPlaceBits; ++i)
            sum += a[i];
    }
    CORRADE_COMPARE(static_cast<std::size_t>(sum), Repeats*(InPlaceBits/2));
}

void BitArrayBenchmark::setGet1024() {
    BitArray a{Corrade::ValueInit, 1024};
    volatile std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        for(std::size_t i = 0; i != 1024; ++i)
            a.set(i, static_cast<bool>(i & 1));
        for(std::size_t i = 0; i != 1024; ++i)
            sum += a[i];
    }
    CORRADE_COMPARE(static_cast<std::size_t>(sum), Repeats*512);
}

void BitArrayBenchmark::manyInPlace() {
    /* 8..64 stays in-place on both widths. The last array is the limit. */
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        for(std::size_t bits = 8; bits <= 64; bits += 8) {
            BitArray a{Corrade::DirectInit, bits, true};
            sum += a.size();
        }
        BitArray edge{Corrade::ValueInit, InPlaceBits};
        sum += edge.size();
    }
    CORRADE_COMPARE(sum, Repeats*(8 + 16 + 24 + 32 + 40 + 48 + 56 + 64 + InPlaceBits));
}

}}}}

CORRADE_TEST_MAIN(Corrade::Containers::Test::BitArrayBenchmark)
