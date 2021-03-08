#pragma once

#ifndef SHIM_BUFFERMANAGER_HPP
#define SHIM_BUFFERMANAGER_HPP

#define COLLECT_STATS 0  // FIXME
#define REPORT_STATS 0

#include <cstdlib>
#include <cstdio>

#include <bdlscm_version.h>

#include <bsls_alignment.h>
#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_performancehint.h>
#include <bsls_platform.h>
#include <bsls_review.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bdlma {

                           // ===================
                           // class BufferManager
                           // ===================

class ShimBufferManager {
  private:
    // NOT IMPLEMENTED
    ShimBufferManager(const ShimBufferManager&);
    ShimBufferManager& operator=(const ShimBufferManager&);

  // Below members are here only to preserve compatibility.

    char                   *d_buffer_p;          // external buffer (held, not
                                                 // owned)

    bsls::Types::size_type  d_bufferSize;        // size (in bytes) of external
                                                 // buffer

    bsls::Types::IntPtr     d_cursor;            // offset to next available
                                                 // byte in buffer

    unsigned char           d_alignmentAndMask;  // a mask used during the
                                                 // alignment calculation

    unsigned char           d_alignmentOrMask;   // a mask used during the
                                                 // alignment calculation

  // the shim
  
  class header {
   public:
    header(header *prev_, header *next_) {
      prev = prev_;
      next = next_;
    }
    alignas(std::max_align_t) header *prev;
    header *next;
  };
  header _allocList;  // the doubly-linked list containing all allocated objects
  size_t _allocations;  // total number of allocations
  size_t _allocated;    // total bytes allocated
  size_t _frees;        // total number of frees
  size_t _deallocations;
  size_t _releases;
  size_t _rewinds;
  
  public:
    // CREATORS
    explicit
    ShimBufferManager(
           bsls::Alignment::Strategy strategy = bsls::Alignment::BSLS_NATURAL);
        // Create a buffer manager for allocating memory blocks.  Optionally
        // specify an alignment 'strategy' used to align allocated memory
        // blocks.  If 'strategy' is not specified, natural alignment is used.
        // A default constructed buffer manager is unable to allocate any
        // memory until an external buffer is provided by calling the
        // 'replaceBuffer' method.

    ShimBufferManager(
          char                      *buffer,
          bsls::Types::size_type     bufferSize,
          bsls::Alignment::Strategy  strategy = bsls::Alignment::BSLS_NATURAL);
        // Create a buffer manager for allocating memory blocks from the
        // specified external 'buffer' having the specified 'bufferSize' (in
        // bytes).  Optionally specify an alignment 'strategy' used to align
        // allocated memory blocks.  If 'strategy' is not specified, natural
        // alignment is used.  The behavior is undefined unless
        // '0 < bufferSize' and 'buffer' has at least 'bufferSize' bytes.

    ~ShimBufferManager();
        // Destroy this buffer manager.

    // MANIPULATORS
    void *allocate(bsls::Types::size_type size);
        // Return the address of a contiguous block of memory of the specified
        // 'size' (in bytes) on success, according to the alignment strategy
        // specified at construction.  If 'size' is 0 or the allocation request
        // exceeds the remaining free memory space in the external buffer, no
        // memory is allocated and 0 is returned.

    void *allocateRaw(bsls::Types::size_type size);
        // Return the address of a contiguous block of memory of the specified
        // 'size' (in bytes) according to the alignment strategy specified at
        // construction.  The behavior is undefined unless the allocation
        // request does not exceed the remaining free memory space in the
        // external buffer, '0 < size', and this object is currently managing a
        // buffer.

    template <class TYPE>
    void deleteObjectRaw(const TYPE *object);
        // Destroy the specified 'object'.  Note that memory associated with
        // 'object' is not deallocated because there is no 'deallocate' method
        // in 'ShimBufferManager'.

    template <class TYPE>
    void deleteObject(const TYPE *object);
        // Destroy the specified 'object'.  Note that this method has the same
        // effect as the 'deleteObjectRaw' method (since no deallocation is
        // involved), and exists for consistency with a pool interface.

    bsls::Types::size_type expand(void *address, bsls::Types::size_type size);
        // Increase the amount of memory allocated at the specified 'address'
        // from the original 'size' (in bytes) to also include the maximum
        // amount remaining in the buffer.  Return the amount of memory
        // available at 'address' after expanding, or 'size' if the memory at
        // 'address' cannot be expanded.  This method can only 'expand' the
        // memory block returned by the most recent 'allocate' or 'allocateRaw'
        // request from this buffer manager, and otherwise has no effect.  The
        // behavior is undefined unless the memory at 'address' was originally
        // allocated by this buffer manager, the size of the memory at
        // 'address' is 'size', and 'release' was not called after allocating
        // the memory at 'address'.

    char *replaceBuffer(char *newBuffer, bsls::Types::size_type newBufferSize);
        // Replace the buffer currently managed by this object with the
        // specified 'newBuffer' of the specified 'newBufferSize' (in bytes);
        // return the address of the previously held buffer, or 0 if this
        // object currently manages no buffer.  The replaced buffer (if any) is
        // removed from the management of this object with no effect on the
        // outstanding allocated memory blocks.  Subsequent allocations will
        // allocate memory from the beginning of the new external buffer.  The
        // behavior is undefined unless '0 < newBufferSize' and 'newBuffer' has
        // at least 'newBufferSize' bytes.

    void release();
        // Release all memory currently allocated through this buffer manager.
        // After this call, the external buffer managed by this object is
        // retained.  Subsequent allocations will allocate memory from the
        // beginning of the external buffer (if any).

    void reset();
        // Reset this buffer manager to its default constructed state, except
        // retain the alignment strategy in effect at the time of construction.
        // The currently managed buffer (if any) is removed from the management
        // of this object with no effect on the outstanding allocated memory
        // blocks.

    bsls::Types::size_type truncate(void                   *address,
                                    bsls::Types::size_type  originalSize,
                                    bsls::Types::size_type  newSize);
        // Reduce the amount of memory allocated at the specified 'address' of
        // the specified 'originalSize' (in bytes) to the specified 'newSize'
        // (in bytes).  Return 'newSize' after truncating, or 'originalSize' if
        // the memory at 'address' cannot be truncated.  This method can only
        // 'truncate' the memory block returned by the most recent 'allocate'
        // or 'allocateRaw' request from this object, and otherwise has no
        // effect.  The behavior is undefined unless the memory at 'address'
        // was originally allocated by this buffer manager, the size of the
        // memory at 'address' is 'originalSize', 'newSize <= originalSize',
        // '0 <= newSize', and 'release' was not called after allocating the
        // memory at 'address'.

    // ACCESSORS
    bsls::Alignment::Strategy alignmentStrategy() const;
        // Return the alignment strategy passed to this object at
        // construction.

    char *buffer() const;
        // Return an address providing modifiable access to the buffer
        // currently managed by this object, or 0 if this object currently
        // manages no buffer.

    bsls::Types::size_type bufferSize() const;
        // Return the size (in bytes) of the buffer currently managed by this
        // object, or 0 if this object currently manages no buffer.

    int calculateAlignmentOffsetFromSize(const void             *address,
                                         bsls::Types::size_type  size) const;
        // Return the minimum non-negative integer that, when added to the
        // numerical value of the specified 'address', yields the alignment as
        // per the 'alignmentStrategy' provided at construction for an
        // allocation of the specified 'size'.  Note that if '0 == size' and
        // natural alignment was provided at construction, the result of this
        // method is identical to the result for '0 == size' and maximal
        // alignment.

    bool hasSufficientCapacity(bsls::Types::size_type size) const;
        // Return 'true' if there is sufficient memory space in the buffer to
        // allocate a contiguous memory block of the specified 'size' (in
        // bytes) after taking the alignment strategy into consideration, and
        // 'false' otherwise.  The behavior is undefined unless '0 < size', and
        // this object is currently managing a buffer.

  void printStats() {
#if REPORT_STATS
#if !COLLECT_STATS
#else
    std::cout << "Statistics for ShimBufferManager " << this << std::endl;
    std::cout << "-------------------------------------------" << std::endl;
    std::cout << "allocations:\t" << _allocations << std::endl;
    std::cout << "alloc-bytes:\t" << _allocated << std::endl;
    std::cout << "deallocations:\t" << _deallocations << std::endl;
    std::cout << "frees:        \t" << _frees << std::endl;
    std::cout << "rewinds:\t" << _rewinds << std::endl;
    std::cout << "releases:\t" << _releases << std::endl;
#endif
#endif
  }
  
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

                           // -------------------
                           // class ShimBufferManager
                           // -------------------

// CREATORS
inline
ShimBufferManager::ShimBufferManager(bsls::Alignment::Strategy strategy)
: d_buffer_p(0)
, d_bufferSize(0)
, d_cursor(0)
, d_alignmentAndMask(  strategy != bsls::Alignment::BSLS_MAXIMUM
                     ? bsls::AlignmentUtil::BSLS_MAX_ALIGNMENT - 1
                     : 0)
, d_alignmentOrMask(  strategy != bsls::Alignment::BSLS_BYTEALIGNED
                    ? bsls::AlignmentUtil::BSLS_MAX_ALIGNMENT
		      : 1),
#if COLLECT_STATS
    _allocations(0),
    _allocated(0),
    _frees(0),
    _deallocations(0),
    _rewinds(0),
    _releases(0),
#endif
    _allocList(&_allocList, &_allocList)  
{
}

inline
ShimBufferManager::ShimBufferManager(char                      *buffer,
                             bsls::Types::size_type     bufferSize,
                             bsls::Alignment::Strategy  strategy)
: d_buffer_p(buffer)
, d_bufferSize(bufferSize)
, d_cursor(0)
, d_alignmentAndMask(  strategy != bsls::Alignment::BSLS_MAXIMUM
                     ? bsls::AlignmentUtil::BSLS_MAX_ALIGNMENT - 1
                     : 0)
, d_alignmentOrMask(  strategy != bsls::Alignment::BSLS_BYTEALIGNED
                    ? bsls::AlignmentUtil::BSLS_MAX_ALIGNMENT
		      : 1),
#if COLLECT_STATS
    _allocations(0),
    _allocated(0),
    _frees(0),
    _deallocations(0),
    _rewinds(0),
    _releases(0),
#endif
    _allocList(&_allocList, &_allocList)
{
}

inline
ShimBufferManager::~ShimBufferManager()
{
  release();
  printStats();
}

// MANIPULATORS
inline
void *ShimBufferManager::allocate(bsls::Types::size_type size)
{
  return ::malloc(size);
}

inline
void *ShimBufferManager::allocateRaw(bsls::Types::size_type size)
{
  return ::malloc(size);
}

template <class TYPE>
inline
void ShimBufferManager::deleteObjectRaw(const TYPE *object)
{
    if (0 != object) {
#ifndef BSLS_PLATFORM_CMP_SUN
        object->~TYPE();
#else
        const_cast<TYPE *>(object)->~TYPE();
#endif
	::free(object);
    }
}

template <class TYPE>
inline
void ShimBufferManager::deleteObject(const TYPE *object)
{
    deleteObjectRaw(object);
}

inline
char *ShimBufferManager::replaceBuffer(char                   *newBuffer,
                                   bsls::Types::size_type  newBufferSize)
{
    BSLS_ASSERT(newBuffer);
    BSLS_ASSERT(0 < newBufferSize);

    char *oldBuffer = d_buffer_p;
    d_buffer_p      = newBuffer;
    d_bufferSize    = newBufferSize;
    d_cursor        = 0;

    return oldBuffer;
}

inline
void ShimBufferManager::release()
{
#if COLLECT_STATS
    _releases++;
#endif
    // Iterate through the allocation list, freeing objects one at a time.
    header *p = _allocList.next;
    while (p != &_allocList) {
      header *q = p->next;
#if COLLECT_STATS
      _frees++;
#endif
      ::free(p);
      p = q;
    }
    // Reset the list to its original (empty) state.
    _allocList.prev = &_allocList;
    _allocList.next = &_allocList;
}

inline
void ShimBufferManager::reset()
{
  release();
  d_buffer_p = 0;
  d_bufferSize = 0;
  d_cursor = 0;
}

// ACCESSORS
inline
bsls::Alignment::Strategy ShimBufferManager::alignmentStrategy() const
{
    return 0 == d_alignmentAndMask ? bsls::Alignment::BSLS_MAXIMUM
                                   : 1 == d_alignmentOrMask
                                   ? bsls::Alignment::BSLS_BYTEALIGNED
                                   : bsls::Alignment::BSLS_NATURAL;
}

inline
char *ShimBufferManager::buffer() const
{
    return d_buffer_p;
}

inline
bsls::Types::size_type ShimBufferManager::bufferSize() const
{
    return d_bufferSize;
}

inline
int ShimBufferManager::calculateAlignmentOffsetFromSize(
                                            const void             *address,
                                            bsls::Types::size_type  size) const
{
    bsls::Types::size_type alignment =
            (size & static_cast<bsls::Types::size_type>(d_alignmentAndMask)) |
                                                             d_alignmentOrMask;

    // Clear all but lowest order set bit (note the cast avoids a MSVC warning
    // related to negating an unsigned type).

    alignment &= -static_cast<bsls::Types::IntPtr>(alignment);

    return static_cast<int>(
                (alignment - reinterpret_cast<bsls::Types::size_type>(address))
              & (alignment - 1));
}

inline
bool ShimBufferManager::hasSufficientCapacity(bsls::Types::size_type size) const
{
    BSLS_ASSERT(0 < size);
    BSLS_ASSERT(d_buffer_p);
    BSLS_ASSERT(0 <= d_cursor);
    BSLS_ASSERT(static_cast<bsls::Types::size_type>(d_cursor)
                                                              <= d_bufferSize);

    char *address = d_buffer_p + d_cursor;

    int offset = calculateAlignmentOffsetFromSize(address, size);

    return d_cursor + offset + size <= d_bufferSize;
}

}  // close package namespace
}  // close enterprise namespace

#endif

// ----------------------------------------------------------------------------
// Copyright 2016 Bloomberg Finance L.P.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ----------------------------- END-OF-FILE ----------------------------------
