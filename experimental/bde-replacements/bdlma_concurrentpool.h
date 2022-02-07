#pragma once

#ifndef SHIM_CONCURRENTPOOL_HPP
#define SHIM_CONCURRENTPOOL_HPP

#include <bsls_ident.h>
BSLS_IDENT("$Id: $")

#include <bdlma_simpool.h>

#include <bdlscm_version.h>

#include <bslmt_mutex.h>

#include <bdlma_infrequentdeleteblocklist.h>

#include <bslma_allocator.h>
#include <bslma_deleterhelper.h>

#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_atomic.h>
#include <bsls_atomicoperations.h>
#include <bsls_blockgrowth.h>
#include <bsls_platform.h>
#include <bsls_types.h>

#include <bsl_cstddef.h>

namespace BloombergLP {
namespace bdlma {

                           // ====================
                           // class ConcurrentPool
                           // ====================

  class PrintConcurrentPool {
  public:
    PrintConcurrentPool() {
      // printMeOnce();
    }
    void printMeOnce()
    {
      static bool printed = false;
      if (!printed) {
	printed = true;
	printf("ConcurrentPool\n");
      }
    }
  };
  
  class ConcurrentPool { // : public PrintConcurrentPool {
    // This class implements a memory pool that allocates and manages memory
    // blocks of some uniform size specified at construction.  This memory pool
    // maintains an internal linked list of free memory blocks, and dispenses
    // one block for each 'allocate' method invocation.  When a memory block is
    // deallocated, it is returned to the free list for potential reuse.
    //
    // This class guarantees thread safety while allocating or releasing
    // memory.

    // PRIVATE TYPES
    struct Link {
        // This 'struct' implements a link data structure that stores the
        // address of the next link, and is used to implement the internal
        // linked list of free memory blocks.  Note that this type is
        // replicated in 'bdlma_concurrentpool.cpp' to provide access to a
        // compatible type from static methods defined in 'bdema_pool.cpp'.

        union {
            bsls::AtomicOperations::AtomicTypes::Int d_refCount;
            bsls::AlignmentUtil::MaxAlignedType      d_dummy;
        };
        Link  *volatile d_next_p;   // pointer to next link
    };

    // DATA
    bsls::Types::size_type d_blockSize;  // size of each allocated memory block
                                         // returned to client

    bsls::Types::size_type d_internalBlockSize;
                                         // actual size of each block
                                         // maintained on free list (contains
                                         // overhead for 'Link')

    int                    d_chunkSize;  // current chunk size (in
                                         // blocks-per-chunk)

    int                    d_maxBlocksPerChunk;
                                         // maximum chunk size (in
                                         // blocks-per-chunk)

    bsls::BlockGrowth::Strategy d_growthStrategy;
                                         // growth strategy of the chunk size

    bsls::AtomicPointer<Link> d_freeList;
                                         // linked list of free memory blocks

    bdlma::InfrequentDeleteBlockList d_blockList;
                                         // memory manager for allocated memory

    bslmt::Mutex      d_mutex;           // protects access to the block list


  SimPool pool;
  
    // PRIVATE MANIPULATORS

  private:
    // NOT IMPLEMENTED
    ConcurrentPool(const ConcurrentPool&);
    ConcurrentPool& operator=(const ConcurrentPool&);

  public:

  void lock() {
    d_mutex.lock();
  }

  void unlock() {
    d_mutex.unlock();
  }
  
    // CREATORS
  explicit ConcurrentPool(bsls::Types::size_type  blockSize,
			      bslma::Allocator *      /* basicAllocator */ = 0) {
    d_blockSize = blockSize;
  }
  ConcurrentPool(bsls::Types::size_type       blockSize,
		     bsls::BlockGrowth::Strategy  /* growthStrategy */,
		     bslma::Allocator  *          /* basicAllocator */ = 0) {
    d_blockSize = blockSize;
  }
  ConcurrentPool(bsls::Types::size_type       blockSize,
		       bsls::BlockGrowth::Strategy  /* growthStrategy */,
		       int                          /* maxBlocksPerChunk */,
		       bslma::Allocator            * /* basicAllocator */ = 0) {
    d_blockSize = blockSize;
  }
        // Create a memory pool that returns blocks of contiguous memory of the
        // specified 'blockSize' (in bytes) for each 'allocate' method
        // invocation.  Optionally specify a 'growthStrategy' used to control
        // the growth of internal memory chunks (from which memory blocks are
        // dispensed).  If 'growthStrategy' is not specified, geometric growth
        // is used.  Optionally specify 'maxBlocksPerChunk' as the maximum
        // chunk size.  If geometric growth is used, the chunk size grows
        // starting at 'blockSize', doubling in size until the size is exactly
        // 'blockSize * maxBlocksPerChunk'.  If constant growth is used, the
        // chunk size is always 'maxBlocksPerChunk'.  If 'maxBlocksPerChunk' is
        // not specified, an implementation-defined value is used.  Optionally
        // specify a 'basicAllocator' used to supply memory.  If
        // 'basicAllocator' is 0, the currently installed default allocator is
        // used.  The behavior is undefined unless '1 <= blockSize' and
        // '1 <= maxBlocksPerChunk'.

  ~ConcurrentPool() {}
        // Destroy this pool, releasing all associated memory back to the
        // underlying allocator.

    // MANIPULATORS
  void *allocate() {
    lock();
    auto ptr = pool.malloc(d_blockSize);
    unlock();
    return ptr;
  }
  
        // Return the address of a contiguous block of memory having the fixed
        // block size specified at construction.

  void deallocate(void *address) {
    lock();
    pool.free(address);
    unlock();
  }
        // Relinquish the memory block at the specified 'address' back to this
        // pool object for reuse.  The behavior is undefined unless 'address'
        // is non-zero, was allocated by this pool, and has not already been
        // deallocated.

    template <class TYPE>
    void deleteObject(const TYPE *object);
        // Destroy the specified 'object' based on its dynamic type and then
        // use this pool to deallocate its memory footprint.  This method has
        // no effect if 'object' is 0.  The behavior is undefined unless
        // 'object', when cast appropriately to 'void *', was allocated using
        // this pool and has not already been deallocated.  Note that
        // 'dynamic_cast<void *>(object)' is applied if 'TYPE' is polymorphic,
        // and 'static_cast<void *>(object)' is applied otherwise.

    template <class TYPE>
    void deleteObjectRaw(const TYPE *object);
        // Destroy the specified 'object' and then use this pool to deallocate
        // its memory footprint.  This method has no effect if 'object' is 0.
        // The behavior is undefined unless 'object' is !not! a secondary base
        // class pointer (i.e., the address is (numerically) the same as when
        // it was originally dispensed by this pool), was allocated using this
        // pool, and has not already been deallocated.

    void release();
        // Relinquish all memory currently allocated via this pool object.

  void reserveCapacity(int /* numBlocks */) {
  }
        // Reserve memory from this pool to satisfy memory requests for at
        // least the specified 'numBlocks' before the pool replenishes.  The
        // behavior is undefined unless '0 <= numBlocks'.

    // ACCESSORS
  bsls::Types::size_type blockSize() const;
        // Return the size (in bytes) of the memory blocks allocated from this
        // pool object.  Note that all blocks dispensed by this pool have the
        // same size.

                                  // Aspects

    bslma::Allocator *allocator() const;
        // Return the allocator used by this object to allocate memory.  Note
        // that this allocator can not be used to deallocate memory
        // allocated through this pool.
};

}  // close package namespace
}  // close enterprise namespace

// Note that the 'new' and 'delete' operators are declared outside the
// 'BloombergLP' namespace so that they do not hide the standard placement
// 'new' and 'delete' operators (i.e.,
// 'void *operator new(bsl::size_t, void *)' and
// 'void operator delete(void *)').
//
// Also note that only the scalar versions of operators 'new' and 'delete' are
// provided, because overloading 'new' (and 'delete') with their array versions
// would cause dangerous ambiguity.  Consider what would have happened had we
// overloaded the array version of 'operator new':
//..
//  void *operator new[](bsl::size_t size, BloombergLP::bdlma::Pool& pool);
//..
// A user of 'bdlma::Pool' may expect to be able to use array 'operator new' as
// follows:
//..
//   new (*pool) my_Type[...];
//..
// The problem is that this expression returns an array that cannot be safely
// deallocated.  On the one hand, there is no syntax in C++ to invoke an
// overloaded 'operator delete'; on the other hand, the pointer returned by
// 'operator new' cannot be passed to the 'deallocate' method directly because
// the pointer is different from the one returned by the 'allocate' method.
// The compiler offsets the value of this pointer by a header, which is used to
// maintain the number of objects in the array (so that 'operator delete' can
// destroy the right number of objects).

// FREE OPERATORS
void *operator new(bsl::size_t size, BloombergLP::bdlma::ConcurrentPool& pool);
    // Return a block of memory of the specified 'size' (in bytes) allocated
    // from the specified 'pool'.  The behavior is undefined unless 'size' is
    // the same or smaller than the 'blockSize' with which 'pool' was
    // constructed.  Note that an object may allocate additional memory

    // internally, requiring the allocator to be passed in as a constructor
    // argument:
    //..
    //  my_Type *newMyType(bdlma::ConcurrentPool *pool,
    //                     bslma::Allocator      *basicAllocator)
    //  {
    //      return new (*pool) my_Type(..., basicAllocator);
    //  }
    //..
    // Also note that the analogous version of 'operator delete' should not be
    // called directly.  Instead, this component provides a static template
    // member function, 'deleteObject', parameterized by 'TYPE':
    //..
    //  void deleteMyType(my_Type *t, bdlma::ConcurrentPool *pool)
    //  {
    //      pool->deleteObject(t);
    //  }
    //..
    // 'deleteObject' performs the following:
    //..
    //  t->~my_Type();
    //  pool->deallocate(t);
    //..

inline
void operator delete(void *address, BloombergLP::bdlma::ConcurrentPool& pool);
    // Use the specified 'pool' to deallocate the memory at the specified
    // 'address'.  The behavior is undefined unless 'address' was allocated
    // using 'pool' and has not already been deallocated.  This operator is
    // supplied solely to allow the compiler to arrange for it to be called in
    // case of an exception.  Client code should not call it; use
    // 'bdlma::ConcurrentPool::deleteObject()' instead.

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

namespace BloombergLP {
namespace bdlma {

                           // --------------------
                           // class ConcurrentPool
                           // --------------------

// MANIPULATORS
template<class TYPE>
inline
void ConcurrentPool::deleteObject(const TYPE *object)
{
    if (nullptr != object)
    {
#ifndef BSLS_PLATFORM_CMP_SUN
        object->~TYPE();
#else
        const_cast<TYPE *>(object)->~TYPE();
#endif
        lock();
        pool.free(const_cast<TYPE *>(object));
        unlock();
    }
}

template<class TYPE>
inline
void ConcurrentPool::deleteObjectRaw(const TYPE *object)
{
    if (nullptr != object)
    {
        void *address = const_cast<TYPE *>(object);

#if defined(BSLS_PLATFORM_CMP_SUN)
        const_cast<TYPE *>(object)->~TYPE();
#else
        object->~TYPE();
#endif

        lock();
        pool.free(const_cast<TYPE *>(object));
        unlock();
    }
}

inline
void ConcurrentPool::release()
{
  lock();
    pool.release();
  unlock();
}

// ACCESSORS
inline
bsls::Types::size_type ConcurrentPool::blockSize() const
{
    return d_blockSize;
}

// Aspects

inline
bslma::Allocator *ConcurrentPool::allocator() const
{
    return d_blockList.allocator();
}

}  // close package namespace
}  // close enterprise namespace


// FREE OPERATORS
inline
void *operator new(bsl::size_t size, BloombergLP::bdlma::ConcurrentPool& pool)
{
#if defined(BSLS_ASSERT_SAFE_IS_USED)
    // gcc-4.8.1 introduced a new warning for unused typedefs, so this typedef
    // should only be present in SAFE mode builds (where it is used).

    typedef BloombergLP::bsls::AlignmentUtil Util;

    BSLS_ASSERT_SAFE(size <= pool.blockSize()
                  && Util::calculateAlignmentFromSize(size)
                       <= Util::calculateAlignmentFromSize(pool.blockSize()));
#endif

    static_cast<void>(size);  // suppress "unused parameter" warnings
    auto ptr = pool.allocate();
    return ptr;
}

inline
void operator delete(void *address, BloombergLP::bdlma::ConcurrentPool& pool)
{
    pool.deallocate(address);
}


#endif
