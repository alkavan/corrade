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

#include <utility>
#include <new>

#include "Corrade/Containers/BitArray.h"
#include "Corrade/TestSuite/Tester.h"

namespace Corrade { namespace Containers { namespace Test { namespace {

/* Minimal reimplementation of the pre-optimization BitArray constructors
   (always heap-allocate for size > 0). Used only so the benchmark can
   compare the two strategies in the same binary. */
struct BitArrayHeapOnly {
    char* data;
    std::size_t size;

    BitArrayHeapOnly(Corrade::ValueInitT, std::size_t s):
        data{s ? new char[(s + 7) >> 3]{} : nullptr}, size{s} {}
    BitArrayHeapOnly(Corrade::DirectInitT, std::size_t s, bool value):
        data{s ? new char[(s + 7) >> 3] : nullptr}, size{s}
    {
        if(data && value)
            for(std::size_t i = 0; i != (s + 7) >> 3; ++i)
                data[i] = '\xff';
    }
    BitArrayHeapOnly(BitArrayHeapOnly&& other) noexcept:
        data{other.data}, size{other.size}
    {
        other.data = nullptr;
        other.size = 0;
    }
    BitArrayHeapOnly& operator=(BitArrayHeapOnly&& other) noexcept {
        using std::swap;
        swap(data, other.data);
        swap(size, other.size);
        return *this;
    }
    ~BitArrayHeapOnly() { delete[] data; }

    BitArrayHeapOnly(const BitArrayHeapOnly&) = delete;
    BitArrayHeapOnly& operator=(const BitArrayHeapOnly&) = delete;
};

struct BitArrayBenchmark: TestSuite::Tester {
    explicit BitArrayBenchmark();

    void constructValueInitSmall();
    void constructValueInitSmallHeapOnly();
    void constructValueInitLarge();
    void constructDirectInitSmall();
    void constructDirectInitSmallHeapOnly();

    void moveConstructSmall();
    void moveConstructSmallHeapOnly();
    void moveAssignSmall();
    void moveAssignSmallHeapOnly();

    void manyTinyArrays();
    void manyTinyArraysHeapOnly();
};

constexpr std::size_t Repeats = 1024;

BitArrayBenchmark::BitArrayBenchmark() {
    addBenchmarks({&BitArrayBenchmark::constructValueInitSmall,
                   &BitArrayBenchmark::constructValueInitSmallHeapOnly,
                   &BitArrayBenchmark::constructValueInitLarge,
                   &BitArrayBenchmark::constructDirectInitSmall,
                   &BitArrayBenchmark::constructDirectInitSmallHeapOnly,

                   &BitArrayBenchmark::moveConstructSmall,
                   &BitArrayBenchmark::moveConstructSmallHeapOnly,
                   &BitArrayBenchmark::moveAssignSmall,
                   &BitArrayBenchmark::moveAssignSmallHeapOnly,

                   &BitArrayBenchmark::manyTinyArrays,
                   &BitArrayBenchmark::manyTinyArraysHeapOnly}, 50);
}

void BitArrayBenchmark::constructValueInitSmall() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, 8};
        sum += a.size();
    }
    CORRADE_COMPARE(sum, Repeats*8);
}

void BitArrayBenchmark::constructValueInitSmallHeapOnly() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArrayHeapOnly a{Corrade::ValueInit, 8};
        sum += a.size;
    }
    CORRADE_COMPARE(sum, Repeats*8);
}

void BitArrayBenchmark::constructValueInitLarge() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, 64};
        sum += a.size();
    }
    CORRADE_COMPARE(sum, Repeats*64);
}

void BitArrayBenchmark::constructDirectInitSmall() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::DirectInit, 5, true};
        sum += a[0];
    }
    CORRADE_COMPARE(sum, Repeats);
}

void BitArrayBenchmark::constructDirectInitSmallHeapOnly() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArrayHeapOnly a{Corrade::DirectInit, 5, true};
        sum += a.size;
    }
    CORRADE_COMPARE(sum, Repeats*5);
}

void BitArrayBenchmark::moveConstructSmall() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, 8};
        BitArray b{std::move(a)};
        sum += b.size();
    }
    CORRADE_COMPARE(sum, Repeats*8);
}

void BitArrayBenchmark::moveConstructSmallHeapOnly() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArrayHeapOnly a{Corrade::ValueInit, 8};
        BitArrayHeapOnly b{std::move(a)};
        sum += b.size;
    }
    CORRADE_COMPARE(sum, Repeats*8);
}

void BitArrayBenchmark::moveAssignSmall() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArray a{Corrade::ValueInit, 8};
        BitArray b{Corrade::ValueInit, 4};
        b = std::move(a);
        sum += b.size();
    }
    CORRADE_COMPARE(sum, Repeats*8);
}

void BitArrayBenchmark::moveAssignSmallHeapOnly() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        BitArrayHeapOnly a{Corrade::ValueInit, 8};
        BitArrayHeapOnly b{Corrade::ValueInit, 4};
        b = std::move(a);
        sum += b.size;
    }
    CORRADE_COMPARE(sum, Repeats*8);
}

void BitArrayBenchmark::manyTinyArrays() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        for(std::size_t i = 1; i <= 8; ++i) {
            BitArray a{Corrade::DirectInit, i, i & 1};
            sum += a.size();
        }
    }
    CORRADE_COMPARE(sum, Repeats*(1+2+3+4+5+6+7+8));
}

void BitArrayBenchmark::manyTinyArraysHeapOnly() {
    std::size_t sum = 0;
    CORRADE_BENCHMARK(Repeats) {
        for(std::size_t i = 1; i <= 8; ++i) {
            BitArrayHeapOnly a{Corrade::DirectInit, i, i & 1};
            sum += a.size;
        }
    }
    CORRADE_COMPARE(sum, Repeats*(1+2+3+4+5+6+7+8));
}

}}}}

CORRADE_TEST_MAIN(Corrade::Containers::Test::BitArrayBenchmark)
