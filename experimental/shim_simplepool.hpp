#pragma once

#ifndef SHIM_SIMPLEPOOL_HPP
#define SHIM_SIMPLEPOOL_HPP

#include <bsls_ident.h>
BSLS_IDENT("$Id: $")

#include "simregion.hpp"

#if defined(BSL_OVERRIDES_STD) && !defined(BOS_STDHDRS_PROLOGUE_IN_EFFECT)
#error "<bslstl_simplepool.h> header can't be included directly in \
BSL_OVERRIDES_STD mode"
#endif

#include <bslscm_version.h>

#include <bslma_allocatortraits.h>

#include <bslmf_movableref.h>

#include <bsls_alignmentfromtype.h>
#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_platform.h>

#include <algorithm>  // swap (C++03)
#include <utility>    // swap (C++17)

namespace BloombergLP {
namespace bslstl {

template <class ALLOCATOR>
struct SHIM_SimplePool_Type {
    // For use only by 'bslstl::SimplePool'.  This 'struct' provides a
    // namespace for a set of types used to define the base-class of a
    // 'SimplePool'.  The parameterized 'ALLOCATOR' is bound to
    // 'MaxAlignedType' to ensure the allocated memory is maximally aligned.

    typedef typename bsl::allocator_traits<ALLOCATOR>::template
            rebind_traits<bsls::AlignmentUtil::MaxAlignedType> AllocatorTraits;
        // Alias for the allocator traits rebound to allocate
        // 'bsls::AlignmentUtil::MaxAlignedType'.

    typedef typename AllocatorTraits::allocator_type AllocatorType;
        // Alias for the allocator type for
        // 'bsls::AlignmentUtil::MaxAlignedType'.
};

template <class VALUE, class ALLOCATOR>
class SimplePool : public SHIM_SimplePool_Type<ALLOCATOR>::AllocatorType {
private:

  static constexpr int SIZE = 8;
  SimRegion * _allocVector { nullptr };
  
    // PRIVATE TYPES
    typedef SHIM_SimplePool_Type<ALLOCATOR> Types;

  int blocksInUse { 0 };
  int blocksAllocated { 0 };
  
  public:
    // TYPES
    typedef VALUE ValueType;
        // Alias for the parameterized type 'VALUE'.

  typedef typename Types::AllocatorType AllocatorType;
        // Alias for the allocator type for a
        // 'bsls::AlignmentUtil::MaxAlignedType'.

  typedef typename Types::AllocatorTraits AllocatorTraits;
        // Alias for the allocator traits for the parameterized
        // 'ALLOCATOR'.
    typedef typename AllocatorTraits::size_type size_type;

  private:
    // NOT IMPLEMENTED
    SimplePool& operator=(bslmf::MovableRef<SimplePool>);
    SimplePool& operator=(const SimplePool&);
    SimplePool(const SimplePool&);

  public:
    // CREATORS
    explicit SimplePool(const ALLOCATOR& allocator);
        // Create a memory pool that returns blocks of contiguous memory of the
        // size of the parameterized 'VALUE' using the specified 'allocator' to
        // supply memory.  The chunk size grows starting with at least
        // 'sizeof(VALUE)', doubling in size up to an implementation defined
        // maximum number of blocks per chunk.

    SimplePool(bslmf::MovableRef<SimplePool> original);
        // Create a memory pool, adopting all outstanding memory allocations
        // associated with the specified 'original' pool, that returns blocks
        // of contiguous memory of the sizeof the paramterized 'VALUE' using
        // the allocator associated with 'original'.  The chunk size is set to
        // that of 'original' and continues to double in size up to an
        // implementation defined maximum number of blocks per chunk.  Note
        // that 'original' is left in a valid but unspecified state.

    ~SimplePool();
        // Destroy this pool, releasing all associated memory back to the
        // underlying allocator.

    // MANIPULATORS
    void adopt(bslmf::MovableRef<SimplePool> pool);
        // Adopt all outstanding memory allocations associated with the
        // specfied memory 'pool'.  The behavior is undefined unless this pool
        // uses the same allocator as that associated with 'pool'.  The
        // behavior is undefined unless this pool is in the default-constructed
        // state.

    AllocatorType& allocator();
        // Return a reference providing modifiable access to the rebound
        // allocator traits for the node-type.  Note that this operation
        // returns a base-class ('AllocatorType') reference to this object.

    VALUE *allocate();
        // Return the address of a block of memory of at least the size of
        // 'VALUE'.  Note that the memory is *not* initialized.

    void deallocate(void *address);
        // Relinquish the memory block at the specified 'address' back to this
        // pool object for reuse.  The behavior is undefined unless 'address'
        // is non-zero, was allocated by this pool, and has not already been
        // deallocated.

    void reserve(size_type numBlocks);
        // Dynamically allocate a new chunk containing the specified
        // 'numBlocks' number of blocks, and add the chunk to the free memory
        // list of this pool.  The additional memory is added irrespective of
        // the amount of free memory when called.  The behavior is undefined
        // unless '0 < numBlocks'.

    void release();
        // Relinquish all memory currently allocated via this pool object.

    void swap(SimplePool& other);
        // Efficiently exchange the memory blocks of this object with those of
        // the specified 'other' object.  This method provides the no-throw
        // exception-safety guarantee.  The behavior is undefined unless
        // 'allocator() == other.allocator()'.

    void quickSwapRetainAllocators(SimplePool& other);
        // Efficiently exchange the memory blocks of this object with those of
        // the specified 'other' object.  This method provides the no-throw
        // exception-safety guarantee.  The behavior is undefined unless
        // 'allocator() == other.allocator()'.

    void quickSwapExchangeAllocators(SimplePool& other);
        // Efficiently exchange the memory blocks and the allocator of this
        // object with those of the specified 'other' object.  This method
        // provides the no-throw exception-safety guarantee.

    // ACCESSORS
    const AllocatorType& allocator() const;
        // Return a reference providing non-modifiable access to the rebound
        // allocator traits for the node-type.  Note that this operation
        // returns a base-class ('AllocatorType') reference to this object.

    bool hasFreeBlocks() const;
        // Return 'true' if this object holds free (currently unused) blocks,
        // and 'false' otherwise.
};

// ============================================================================
//                  TEMPLATE AND INLINE FUNCTION DEFINITIONS
// ============================================================================

// CREATORS
template <class VALUE, class ALLOCATOR>
inline
SimplePool<VALUE, ALLOCATOR>::SimplePool(const ALLOCATOR& allocator)
  : AllocatorType(allocator),
    _allocVector (new SimRegion(SIZE))
{
}

template <class VALUE, class ALLOCATOR>
inline
SimplePool<VALUE, ALLOCATOR>::SimplePool(
                                        bslmf::MovableRef<SimplePool> original)
  : AllocatorType(bslmf::MovableRefUtil::access(original).allocator()),
    _allocVector (new SimRegion(SIZE))
{
    SimplePool& lvalue = original;
}

template <class VALUE, class ALLOCATOR>
inline
SimplePool<VALUE, ALLOCATOR>::~SimplePool()
{
    release();
}

// MANIPULATORS
template <class VALUE, class ALLOCATOR>
inline
void
SimplePool<VALUE, ALLOCATOR>::adopt(bslmf::MovableRef<SimplePool> pool)
{
    BSLS_ASSERT_SAFE(allocator()
                           == bslmf::MovableRefUtil::access(pool).allocator());

    SimplePool& lvalue = pool;
    _allocVector = lvalue._allocVector;
    lvalue._allocVector = new SimRegion(SIZE);
}

template <class VALUE, class ALLOCATOR>
inline
typename SimplePool<VALUE, ALLOCATOR>::AllocatorType&
SimplePool<VALUE, ALLOCATOR>::allocator()
{
    return *this;
}

template <class VALUE, class ALLOCATOR>
inline
VALUE *SimplePool<VALUE, ALLOCATOR>::allocate()
{
  return (VALUE *) _allocVector->allocate(sizeof(VALUE));
}

template <class VALUE, class ALLOCATOR>
inline
void SimplePool<VALUE, ALLOCATOR>::deallocate(void *address)
{
    BSLS_ASSERT_SAFE(address);
    // DISABLE FOR NOW. POSSIBLY FIXME
}

template <class VALUE, class ALLOCATOR>
inline
void SimplePool<VALUE, ALLOCATOR>::swap(SimplePool<VALUE, ALLOCATOR>& other)
{
    BSLS_ASSERT_SAFE(allocator() == other.allocator());

    std::swap(_allocVector, other._allocVector);
}

template <class VALUE, class ALLOCATOR>
inline
void SimplePool<VALUE, ALLOCATOR>::quickSwapRetainAllocators(
                                           SimplePool<VALUE, ALLOCATOR>& other)
{
    swap(other);
}

template <class VALUE, class ALLOCATOR>
inline
void SimplePool<VALUE, ALLOCATOR>::quickSwapExchangeAllocators(
                                           SimplePool<VALUE, ALLOCATOR>& other)
{
    using std::swap;
    swap(this->allocator(), other.allocator());
    swap(_allocVector, other._allocVector);
}

template <class VALUE, class ALLOCATOR>
void SimplePool<VALUE, ALLOCATOR>::reserve(size_type numBlocks)
{
}

template <class VALUE, class ALLOCATOR>
void SimplePool<VALUE, ALLOCATOR>::release()
{
#ifdef BSLS_PLATFORM_HAS_PRAGMA_GCC_DIAGNOSTIC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
  _allocVector->release();
#ifdef BSLS_PLATFORM_HAS_PRAGMA_GCC_DIAGNOSTIC
#pragma GCC diagnostic pop
#endif

}

// ACCESSORS
template <class VALUE, class ALLOCATOR>
inline
const typename SimplePool<VALUE, ALLOCATOR>::AllocatorType&
SimplePool<VALUE, ALLOCATOR>::allocator() const
{
    return *this;
}

template <class VALUE, class ALLOCATOR>
inline
bool SimplePool<VALUE, ALLOCATOR>::hasFreeBlocks() const
{
  return true;
}

}  // close package namespace
}  // close enterprise namespace

#endif // SHIM_SIMPLEPOOL_HPP
