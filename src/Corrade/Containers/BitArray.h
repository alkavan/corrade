#ifndef Corrade_Containers_BitArray_h
#define Corrade_Containers_BitArray_h
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

/** @file
 * @brief Class @ref Corrade::Containers::BitArray
 * @m_since_latest
 */

#include <cstddef>
#include <type_traits>

#include "Corrade/Tags.h"
#include "Corrade/Containers/Containers.h"
#include "Corrade/Utility/DebugAssert.h"
#include "Corrade/Utility/Move.h"
#include "Corrade/Utility/Utility.h"
#include "Corrade/Utility/visibility.h"

namespace Corrade { namespace Containers {

/**
@brief Bit array
@m_since_latest

Owning container for an array of bits. Eight times more memory-efficient than
@ref Array "Array<bool>"; a lighter alternative to @ref std::vector<bool>.
A non-owning version of this container is a @ref BitArrayView and a
@ref MutableBitArrayView, implemented using a generic @ref BasicBitArrayView.

As with @ref Array itself, the container is non-copyable with a size specified
upfront. At the moment, there's no growing functionality.

@section Containers-BitArray-usage Usage

The following snippet shows using a bit array for marking which mesh vertices
are used by an index buffer:

@snippet Containers.cpp BitArray-usage

The @ref BitArray class provides access and slicing APIs similar to
@ref BasicBitArrayView "BitArrayView", see @ref Containers-BasicBitArrayView-usage "its usage docs"
for details. All @ref BitArray slicing APIs return a (mutable)
@ref BasicBitArrayView "BitArrayView", additionally @ref BitArray instances are also implicitly convertible to it. The only difference is due to the owning
aspect --- mutable access to the data is provided only via non @cpp const @ce
overloads.

@attention Consistently with @ref BitArrayView, because the size represents
    bits and because the class additionally has to store initial offset in the
    first byte, on 32-bit systems the size is limited to 512M bits --- i.e.,
    64 MB of memory.

@subsection Containers-BitArray-usage-initialization Bit array initialization

The following explicit initialization constructors are provided, similarly to
the @ref Array class:

-   @ref BitArray(ValueInitT, std::size_t) zero-initializes the array. This is
    equivalent to @cpp new char[(size + 7)/8]{} @ce.
-   @ref BitArray(DirectInitT, std::size_t, bool) fills the whole array with
    given bit value. This is equivalent to @cpp new char[(size + 7)/8]{value*'\xff', value*'\xff', …} @ce.
-   @ref BitArray(NoInitT, std::size_t) keeps the contents uninitialized.
    Useful when you'll be overwriting the contents anyway. Equivalent to
    @cpp new char[(size + 7)/8] @ce.

Arrays of at most @ref Implementation::SmallMaxBits "184 bits" (88 on 32-bit)
are stored in-place inside the instance instead of being heap-allocated.
@ref data() then points into the instance and is invalidated on move.
@ref release() copies the bits into a @cpp new[] @ce allocation so the
caller can still @cpp delete[] @ce it.

@snippet Containers.cpp BitArray-usage-initialization

<b></b>

@m_class{m-note m-success}

@par Aligned allocations
    Please note that @ref BitArray allocations are by default only aligned to
    @cpp 2*sizeof(void*) @ce. If you need overaligned memory for working with
    SIMD types, use @ref Utility::allocateAligned() instead.

@subsection Containers-BitArray-usage-wrapping Wrapping externally allocated bit arrays

Similarly to @ref Array, by default the class makes all allocations using
@cpp operator new[] @ce and deallocates using @cpp operator delete[] @ce,
with the exception of arrays of at most 184 bits (88 on 32-bit), which are
stored in-place.
It's however also possible to wrap an externally allocated array using
@ref BitArray(void*, std::size_t, std::size_t, Deleter) together with
specifying which function to use for deallocation. In addition to a size in
bits, there's also an offset argument specifying the starting bit in the first
byte, consistently with a @ref BasicBitArrayView "BitArrayView". Often it will
be @cpp 0 @ce however.

For example, properly deallocating a bit array allocated using @ref std::malloc():

@snippet Containers.cpp BitArray-usage-wrapping

@see @ref EnumSet, @ref BigEnumSet
*/
class CORRADE_UTILITY_EXPORT BitArray {
    public:
        typedef void(*Deleter)(char*, std::size_t); /**< @brief Deleter type */

        /**
         * @brief Default constructor
         *
         * Creates a zero-sized array. Move a @ref BitArray with a nonzero size
         * onto the instance to make it useful.
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        /*implicit*/ BitArray(std::nullptr_t = nullptr) noexcept;
        #else
        /* To avoid ambiguity in certain cases of passing 0 to overloads that
           take either a BitArray or std::size_t. See the
           constructZeroNullPointerAmbiguity() test for more info. FFS, zero as
           null pointer was deprecated in C++11 already, why is this still a
           problem?! */
        template<class T, typename std::enable_if<std::is_same<std::nullptr_t, T>::value, int>::type = 0> /*implicit*/ BitArray(T) noexcept: large{} {}
        /*implicit*/ BitArray() noexcept: large{} {}
        #endif

        /**
         * @brief Construct a zero-initialized array
         * @param size      Size in bits
         *
         * If the size is zero, no allocation is done.
         * @see @relativeref{Corrade,ValueInit},
         *      @ref BitArray(NoInitT, std::size_t),
         *      @ref BitArray(DirectInitT, std::size_t, bool), @ref resetAll()
         */
        explicit BitArray(Corrade::ValueInitT, std::size_t size);

        /**
         * @brief Construct an array without initializing its contents
         * @param size      Size in bits
         *
         * The contents are *not* initialized. If the size is zero, no
         * allocation is done. Useful if you will be overwriting all elements
         * later anyway.
         * @see @relativeref{Corrade,NoInit},
         *      @ref BitArray(ValueInitT, std::size_t),
         *      @ref BitArray(DirectInitT, std::size_t, bool), @ref setAll(),
         *      @ref resetAll()
         */
        explicit BitArray(Corrade::NoInitT, std::size_t size);

        /**
         * @brief Construct an array initialized to a particular bit value
         * @param size      Size in bits
         * @param value     Bit value
         *
         * If the size is zero, no allocation is done.
         * @see @relativeref{Corrade,DirectInit},
         *      @ref BitArray(ValueInitT, std::size_t),
         *      @ref BitArray(NoInitT, std::size_t), @ref setAll(),
         *      @ref resetAll()
         */
        explicit BitArray(Corrade::DirectInitT, std::size_t size, bool value);

        /**
         * @brief Take ownership of an external bit array
         * @param data      Data
         * @param offset    Initial bit offset in @p data. Expected to be less
         *      than 8.
         * @param size      Size in bits, excluding @p offset
         * @param deleter   Deleter. Use @cpp nullptr @ce for the standard
         *      @cpp delete[] @ce.
         *
         * The @p deleter will be *unconditionally* called on destruction with
         * @p data and @cpp (offset + size + 7)/8 @ce (i.e., size including the
         * initial offset in bytes) as an argument. In particular, it will be
         * also called if @p data is @cpp nullptr @ce or @p size is @cpp 0 @ce.
         *
         * In case of a moved-out instance, the deleter gets reset to a
         * default-constructed value alongside the array pointer and size. It
         * effectively means @cpp delete[] nullptr @ce gets called when
         * destructing a moved-out instance (which is a no-op).
         * @see @ref Containers-BitArray-usage-wrapping
         */
        explicit BitArray(void* data, std::size_t offset, std::size_t size, Deleter deleter) noexcept;

        /* No ownership-taking overload taking const void* at the moment, as
           (unlike with a String) I don't see a need for such a footgun. */

        /** @brief Copying is not allowed */
        BitArray(const BitArray&) = delete;

        /**
         * @brief Move constructor
         *
         * Resets data pointer, offset, size and deleter of @p other to be
         * equivalent to a default-constructed instance.
         */
        /* MinGW complains loudly if the declaration doesn't also have inline */
        inline BitArray(BitArray&& other) noexcept;

        /**
         * @brief Destructor
         *
         * Calls @ref deleter() on the owned @ref data(). An in-place array
         * has nothing to delete.
         */
        ~BitArray();

        /** @brief Copying is not allowed */
        BitArray& operator=(const BitArray&) = delete;

        /**
         * @brief Move assignment
         *
         * Swaps data pointer, offset, size and deleter of the two instances.
         */
        /* MinGW complains loudly if the declaration doesn't also have inline */
        inline BitArray& operator=(BitArray&& other) noexcept;

        /** @brief Conversion to a view */
        /*implicit*/ operator MutableBitArrayView();
        /*implicit*/ operator BitArrayView() const; /**< @overload */

        /* No bool conversion operator right now, as it's yet unclear what
           semantic should it have -- return false if it's nullptr, if the size
           is zero (or both?). or if all bits are false? */

        /**
         * @brief Array data
         *
         * Use @ref offset() to get location of the first bit pointed to by the
         * array. For an in-place array the returned pointer addresses storage
         * inside the instance and is invalidated by a move.
         */
        char* data() { if(isSmall()) { return small.data; } return large.data; }
        const char* data() const { if(isSmall()) { return small.data; } return large.data; } /**< @overload */

        /**
         * @brief Array deleter
         *
         * If set to @cpp nullptr @ce, the contents are deleted using standard
         * @cpp operator delete[] @ce.
         * @see @ref BitArray(void*, std::size_t, std::size_t, Deleter)
         */
        Deleter deleter() const { return isSmall() ? nullptr : large.deleter; }

        /**
         * @brief Offset in the first byte
         *
         * The returned value is always less than @cpp 8 @ce, and is non-zero
         * only if the array was created with a non-zero @p offset passed to
         * the @ref BitArray(void*, std::size_t, std::size_t, Deleter)
         * constructor.
         * @see @ref size()
         */
        std::size_t offset() const { return isSmall() ? 0 : (large.sizeOffset & 0x07); }

        /**
         * @brief Size in bits
         *
         * @see @ref isEmpty(), @ref offset()
         */
        std::size_t size() const {
            return isSmall() ? small.size - SmallSizeMark : large.sizeOffset >> 3;
        }

        /**
         * @brief Whether the array is empty
         *
         * @see @ref size()
         */
        bool isEmpty() const { return size() == 0; }

        /**
         * @brief Bit at given position
         *
         * Expects that @p i is less than @ref size(). Use @ref set(std::size_t),
         * @ref reset(std::size_t) or @ref set(std::size_t, bool) to set a bit
         * value.
         */
        /* MinGW complains loudly if the declaration doesn't also have inline */
        inline bool operator[](std::size_t i) const;

        /**
         * @brief Set a bit at given position
         *
         * Expects that @p i is less than @ref size().
         * @see @ref operator[](), @ref reset(), @ref set(std::size_t, bool),
         *      @ref setAll()
         */
        /* MinGW complains loudly if the declaration doesn't also have inline */
        inline void set(std::size_t i);

        /**
         * @brief Use @ref setAll(bool) to set all bits or @ref set(std::size_t, bool) to set a single bit to a concrete value
         *
         * Deleted to avoid accidental use of @ref set(std::size_t) with a
         * @cpp bool @ce.
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        void set(bool) = delete;
        #else
        template<class U, typename std::enable_if<std::is_same<U, bool>::value, int>::type = 0> void set(U) = delete;
        #endif

        /**
         * @brief Set all bits
         *
         * You can set just a range of bits by making a @ref slice() first.
         * @see @ref resetAll(), @ref setAll(bool), @ref set(std::size_t)
         */
        void setAll();

        /**
         * @brief Reset a bit at given position
         *
         * Expects that @p i is less than @ref size().
         * @see @ref operator[](), @ref set(), @ref resetAll()
         */
        /* MinGW complains loudly if the declaration doesn't also have inline */
        inline void reset(std::size_t i);

        /**
         * @brief Use @ref setAll(bool) to set all bits or @ref set(std::size_t, bool) to set a single bit to a concrete value
         *
         * Deleted to avoid accidental use of @ref reset(std::size_t) with a
         * @cpp bool @ce.
         */
        #ifdef DOXYGEN_GENERATING_OUTPUT
        void reset(bool) = delete;
        #else
        template<class U, typename std::enable_if<std::is_same<U, bool>::value, int>::type = 0> void reset(U) = delete;
        #endif

        /**
         * @brief Reset all bits
         *
         * You can set just a range of bits by making a @ref slice() first.
         * @see @ref setAll(), @ref reset(std::size_t)
         */
        void resetAll();

        /**
         * @brief Set or reset a bit at given position
         *
         * Expects that @p i is less than @ref size(). For a @p value known at
         * compile time, explicitly calling either @ref set(std::size_t) or
         * @ref reset(std::size_t) is more efficient.
         * @see @ref operator[]()
         */
        /* MinGW complains loudly if the declaration doesn't also have inline */
        inline void set(std::size_t i, bool value);

        /**
         * @brief Set or reset all bits
         *
         * Calls either @ref setAll() or @ref resetAll() based on @p value. You
         * can set or reset just a range of bits by making a @ref slice()
         * first.
         * @see @ref set(std::size_t, bool)
         */
        void setAll(bool value) { value ? setAll() : resetAll(); }

        /**
         * @brief View on a slice
         *
         * Equivalent to @ref BasicBitArrayView::slice().
         */
        MutableBitArrayView slice(std::size_t begin, std::size_t end);
        BitArrayView slice(std::size_t begin, std::size_t end) const; /**< @overload */

        /**
         * @brief View on a slice of given size
         *
         * Equivalent to @ref BasicBitArrayView::sliceSize().
         */
        MutableBitArrayView sliceSize(std::size_t begin, std::size_t size);
        BitArrayView sliceSize(std::size_t begin, std::size_t size) const; /**< @overload */

        /**
         * @brief View on the first @p size bits
         *
         * Equivalent to @ref BasicBitArrayView::prefix().
         */
        MutableBitArrayView prefix(std::size_t size);
        BitArrayView prefix(std::size_t size) const; /**< @overload */

        /**
         * @brief View on the last @p size bits
         *
         * Equivalent to @ref BasicBitArrayView::suffix().
         */
        MutableBitArrayView suffix(std::size_t size);
        BitArrayView suffix(std::size_t size) const; /**< @overload */

        /**
         * @brief View except the first @p size bits
         *
         * Equivalent to @ref BasicBitArrayView::exceptPrefix().
         */
        MutableBitArrayView exceptPrefix(std::size_t size);
        BitArrayView exceptPrefix(std::size_t size) const; /**< @overload */

        /**
         * @brief View except the last @p size bits
         *
         * Equivalent to @ref BasicBitArrayView::exceptSuffix().
         */
        MutableBitArrayView exceptSuffix(std::size_t size);
        BitArrayView exceptSuffix(std::size_t size) const; /**< @overload */

        /**
         * @brief Count of set bits
         *
         * Equivalent to @ref BasicBitArrayView::count().
         */
        std::size_t count() const;

        /**
         * @brief Release data storage
         *
         * Returns the data pointer and resets data pointer, offset, size and
         * deleter to be equivalent to a default-constructed instance. Deleting
         * the returned array is user responsibility --- note the array might
         * have a custom @ref deleter() and so @cpp delete[] @ce might not be
         * always appropriate. An in-place
         * array is copied into a @cpp new[] @ce allocation first;
         * @cpp delete[] @ce is then the right way to free that pointer.
         */
        char* release();

        private:
            enum: std::size_t {
                /* 23 bytes on 64-bit, 11 on 32-bit. Same budget as String SSO,
                   all of it used for bits: 184 / 88. */
                SmallDataSize = sizeof(std::size_t)*3 - 1,
                SmallMaxBits = SmallDataSize*8,
                /* Stored as `size + SmallSizeMark`, so every in-place size has
                   a bit in SmallSizeMask set. 0x40 alone is not enough: a size
                   of 64 encodes as 0x80. Large sizeOffset keeps both bits
                   clear, same reservation as String. */
                SmallSizeMark = 0x40,
                SmallSizeMask = 0xc0
            };

            /* Field order matches String so this size byte and the most
               significant byte of sizeOffset occupy the same address on both
               endians. */
            struct Large {
                #ifdef CORRADE_TARGET_BIG_ENDIAN
                std::size_t sizeOffset;
                char* data;
                void(*deleter)(char*, std::size_t);
                #else
                char* data;
                void(*deleter)(char*, std::size_t);
                std::size_t sizeOffset;
                #endif
            };
            struct Small {
                #ifdef CORRADE_TARGET_BIG_ENDIAN
                unsigned char size;
                char data[SmallDataSize];
                #else
                char data[SmallDataSize];
                unsigned char size;
                #endif
            };
            union {
                Large large{};
                Small small;
            };

            static_assert(sizeof(Large) == sizeof(Small),
                "in-place and heap BitArray representations have to be the same size");
            static_assert(SmallMaxBits + SmallSizeMark <= 255,
                "in-place BitArray size has to fit into one byte");
            static_assert((SmallSizeMark & SmallSizeMask) && ((64 + SmallSizeMark) & SmallSizeMask)
                && ((SmallMaxBits + SmallSizeMark) & SmallSizeMask),
                "in-place sizes must be distinguishable from a large sizeOffset");

            bool isSmall() const { return small.size & SmallSizeMask; }
};

static_assert(sizeof(BitArray) == 3*sizeof(std::size_t),
    "Containers::BitArray in-place storage should fit in the same space as a pointer, deleter and size");

CORRADE_UTILITY_EXPORT Utility::Debug& operator<<(Utility::Debug& debug, const BitArray& value);

inline BitArray::BitArray(BitArray&& other) noexcept {
    if(other.isSmall())
        small = other.small;
    else
        large = other.large;
    /* Clearing only the size byte would leave the small bytes looking
       like a heap pointer. */
    other.large = {};
}

inline BitArray& BitArray::operator=(BitArray&& other) noexcept {
    using Utility::swap;
    if(isSmall() == other.isSmall()) {
        if(isSmall())
            swap(small, other.small);
        else
            swap(large, other.large);
    } else if(isSmall()) {
        auto tmp = small;
        large = other.large;
        other.small = tmp;
    } else {
        auto tmp = large;
        small = other.small;
        other.large = tmp;
    }
    return *this;
}

inline bool BitArray::operator[](std::size_t i) const {
    CORRADE_DEBUG_ASSERT(i < size(),
        "Containers::BitArray::operator[](): index" << i << "out of range for" << size() << "bits", {});
    const char* const d = data();
    const std::size_t off = offset();
    return d[(off + i) >> 3] & (1 << ((off + i) & 0x7));
}

inline void BitArray::set(std::size_t i) {
    CORRADE_DEBUG_ASSERT(i < size(),
        "Containers::BitArray::set(): index" << i << "out of range for" << size() << "bits", );
    char* const d = data();
    const std::size_t off = offset();
    d[(off + i) >> 3] |= (1 << ((off + i) & 0x07));
}

inline void BitArray::reset(std::size_t i) {
    CORRADE_DEBUG_ASSERT(i < size(),
        "Containers::BitArray::reset(): index" << i << "out of range for" << size() << "bits", );
    char* const d = data();
    const std::size_t off = offset();
    d[(off + i) >> 3] &= ~(1 << ((off + i) & 0x07));
}

inline void BitArray::set(std::size_t i, bool value) {
    CORRADE_DEBUG_ASSERT(i < size(),
        "Containers::BitArray::set(): index" << i << "out of range for" << size() << "bits", );
    char* const d = data();
    const std::size_t off = offset();
    char& byte = d[(off + i) >> 3];
    byte ^= (-char(value) ^ byte) & (1 << ((off + i) & 0x07));
}

}}

#endif
