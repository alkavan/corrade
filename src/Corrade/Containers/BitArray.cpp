/*
    This file is part of Corrade.

    Copyright © 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014, 2015, 2016,
                2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025, 2026
              Vladimír Vondruš <mosra@centrum.cz>

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

#include "BitArray.h"

#include <cstring>

#include "Corrade/Containers/StridedBitArrayView.h"
#include "Corrade/Utility/Assert.h"
#include "Corrade/Utility/Debug.h"

namespace Corrade { namespace Containers {

BitArray::BitArray(Corrade::ValueInitT, std::size_t size)
    #if defined(CORRADE_TARGET_32BIT) && defined(CORRADE_GRACEFUL_ASSERT)
    /* If the assert fails, don't try to delete a random pointer. Doing
       this only on 32-bit keeps a 64-bit NoInit array actually
       uninitialized. */
    : large{}
    #endif
{
    #ifdef CORRADE_TARGET_32BIT
    /* It makes little sense to check the size constraint on 64-bit, if 64-bit
       code happens to go over then it's got bigger problems than this
       assert. Two further bits are reserved for the in-place flag, so this
       is tighter than BitArrayView. */
    CORRADE_ASSERT(size < std::size_t{1} << (sizeof(std::size_t)*8 - 5),
        "Containers::BitArray: size expected to be smaller than 2^" << Utility::Debug::nospace << (sizeof(std::size_t)*8 - 5) << "bits, got" << size, );
    #endif

    /* Size 0 stays a null large instance so data() == nullptr still means
       empty. Anything that fits in the spare bytes stays in-place. */
    if(!size)
        large = {};
    else if(size <= SmallMaxBits) {
        std::memset(small.data, '\0', sizeof(small.data));
        small.size = static_cast<unsigned char>(size + SmallSizeMark);
    } else {
        large.data = new char[(size + 7) >> 3]{};
        large.sizeOffset = size << 3;
        large.deleter = {};
    }
}

BitArray::BitArray(Corrade::NoInitT, std::size_t size)
    #if defined(CORRADE_TARGET_32BIT) && defined(CORRADE_GRACEFUL_ASSERT)
    : large{} /* If the assert fails, don't try to delete a random pointer */
    #endif
{
    #ifdef CORRADE_TARGET_32BIT
    /* It makes little sense to check the size constraint on 64-bit, if 64-bit
       code happens to go over then it's got bigger problems than this
       assert. Two further bits are reserved for the in-place flag, so this
       is tighter than BitArrayView. */
    CORRADE_ASSERT(size < std::size_t{1} << (sizeof(std::size_t)*8 - 5),
        "Containers::BitArray: size expected to be smaller than 2^" << Utility::Debug::nospace << (sizeof(std::size_t)*8 - 5) << "bits, got" << size, );
    #endif

    if(!size)
        large = {};
    else if(size <= SmallMaxBits) {
        /* Data stays uninitialized. Only the size byte has to be written
           for the destructor to recognize the in-place representation. */
        small.size = static_cast<unsigned char>(size + SmallSizeMark);
    } else {
        large.data = new char[(size + 7) >> 3];
        large.sizeOffset = size << 3;
        large.deleter = {};
    }
}

BitArray::BitArray(Corrade::DirectInitT, const std::size_t size, const bool value)
    #if defined(CORRADE_TARGET_32BIT) && defined(CORRADE_GRACEFUL_ASSERT)
    : large{} /* If the assert fails, don't try to delete a random pointer */
    #endif
{
    #ifdef CORRADE_TARGET_32BIT
    /* It makes little sense to check the size constraint on 64-bit, if 64-bit
       code happens to go over then it's got bigger problems than this
       assert. Two further bits are reserved for the in-place flag, so this
       is tighter than BitArrayView. */
    CORRADE_ASSERT(size < std::size_t{1} << (sizeof(std::size_t)*8 - 5),
        "Containers::BitArray: size expected to be smaller than 2^" << Utility::Debug::nospace << (sizeof(std::size_t)*8 - 5) << "bits, got" << size, );
    #endif
    if(!size)
        large = {};
    else if(size <= SmallMaxBits) {
        std::memset(small.data, value ? '\xff' : '\0', sizeof(small.data));
        small.size = static_cast<unsigned char>(size + SmallSizeMark);
    } else if(!value) {
        large.data = new char[(size + 7) >> 3]{};
        large.sizeOffset = size << 3;
        large.deleter = {};
    } else {
        large.data = new char[(size + 7) >> 3];
        std::memset(large.data, 0xff, (size + 7) >> 3);
        large.sizeOffset = size << 3;
        large.deleter = {};
    }
}

BitArray::BitArray(void* data, const std::size_t offset, const std::size_t size, const Deleter deleter) noexcept
    #ifdef CORRADE_GRACEFUL_ASSERT
    /* Zero first so a failed assert does not free the caller's pointer.
       Members are assigned below rather than in the initializer list
       because their order depends on endianness. */
    : large{}
    #endif
{
    CORRADE_ASSERT(offset < 8,
        "Containers::BitArray: offset expected to be smaller than 8 bits, got" << offset, );
    #ifdef CORRADE_TARGET_32BIT
    /* It makes little sense to check the size constraint on 64-bit, if 64-bit
       code happens to go over then it's got bigger problems than this
       assert. Two further bits are reserved for the in-place flag, so this
       is tighter than BitArrayView. */
    CORRADE_ASSERT(size < std::size_t{1} << (sizeof(std::size_t)*8 - 5),
        "Containers::BitArray: size expected to be smaller than 2^"
        << Utility::Debug::nospace << (sizeof(std::size_t)*8 - 5)
        << "bits, got" << size, );
    #endif
    large.data = static_cast<char*>(data);
    large.deleter = deleter;
    large.sizeOffset = size << 3 | offset;
}

BitArray::~BitArray() {
    /* In-place data lives in the object; nothing was allocated. */
    if(isSmall())
        return;

    if(large.deleter) {
        /* The deleter gets amount of bits spanned by all bits including the
           initial offset -- if offset is 0 and size is 0, it gets 0, but if
           offset is 7 and size 0, then it gets 1. */
        large.deleter(large.data, ((large.sizeOffset >> 3) + (large.sizeOffset & 0x07) + 7) >> 3);
    } else delete[] large.data;
}

char* BitArray::release() {
    if(isSmall()) {
        const std::size_t bytes = (size() + 7) >> 3;
        const auto out = new char[bytes ? bytes : 1];
        if(bytes)
            std::memcpy(out, small.data, bytes);
        large = {};
        return out;
    }

    char* const out = large.data;
    large = {};
    return out;
}

BitArray::operator MutableBitArrayView() {
    return MutableBitArrayView{data(), isSmall() ? (size() << 3) : large.sizeOffset};
}

BitArray::operator BitArrayView() const {
    return BitArrayView{data(), isSmall() ? (size() << 3) : large.sizeOffset};
}

void BitArray::setAll() {
    MutableBitArrayView{*this}.setAll();
}

void BitArray::resetAll() {
    MutableBitArrayView{*this}.resetAll();
}

MutableBitArrayView BitArray::slice(const std::size_t begin, const std::size_t end) {
    return MutableBitArrayView{*this}.slice(begin, end);
}
BitArrayView BitArray::slice(const std::size_t begin, const std::size_t end) const {
    return BitArrayView{*this}.slice(begin, end);
}

MutableBitArrayView BitArray::sliceSize(const std::size_t begin, const std::size_t size) {
    return MutableBitArrayView{*this}.sliceSize(begin, size);
}
BitArrayView BitArray::sliceSize(const std::size_t begin, const std::size_t size) const {
    return BitArrayView{*this}.sliceSize(begin, size);
}

MutableBitArrayView BitArray::prefix(const std::size_t size) {
    return MutableBitArrayView{*this}.prefix(size);
}
BitArrayView BitArray::prefix(const std::size_t size) const {
    return BitArrayView{*this}.prefix(size);
}

MutableBitArrayView BitArray::suffix(const std::size_t size) {
    return MutableBitArrayView{*this}.suffix(size);
}
BitArrayView BitArray::suffix(const std::size_t size) const {
    return BitArrayView{*this}.suffix(size);
}

MutableBitArrayView BitArray::exceptPrefix(const std::size_t size) {
    return MutableBitArrayView{*this}.exceptPrefix(size);
}
BitArrayView BitArray::exceptPrefix(const std::size_t size) const {
    return BitArrayView{*this}.exceptPrefix(size);
}

MutableBitArrayView BitArray::exceptSuffix(const std::size_t size) {
    return MutableBitArrayView{*this}.exceptSuffix(size);
}
BitArrayView BitArray::exceptSuffix(const std::size_t size) const {
    return BitArrayView{*this}.exceptSuffix(size);
}

std::size_t BitArray::count() const {
    return BitArrayView{*this}.count();
}

Utility::Debug& operator<<(Utility::Debug& debug, const BitArray& value) {
    return operator<<(debug, BitArrayView{value});
}

Utility::Debug& operator<<(Utility::Debug& debug, const StridedBitArrayView1D& value) {
    debug << "{" << Utility::Debug::nospace;

    for(std::size_t i = 0, iMax = value.size(); i != iMax; ++i) {
        if(i && i % 8 == 0)
            debug << ",";

        debug << (value[i] ? "1" : "0") << Utility::Debug::nospace;
    }

    return debug << "}";
}

}}
