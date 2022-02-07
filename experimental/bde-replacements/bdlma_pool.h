#pragma once

#ifndef SHIM_POOL_HPP
#define SHIM_POOL_HPP

#include <bsls_ident.h>
BSLS_IDENT("$Id: $")

#include <bdlma_simpool.h>

#include <bdlscm_version.h>

#include <bdlma_infrequentdeleteblocklist.h>

#include <bslma_allocator.h>
#include <bslma_deleterhelper.h>

#include <bsls_alignmentutil.h>
#include <bsls_assert.h>
#include <bsls_blockgrowth.h>
#include <bsls_types.h>

#include <bsl_cstddef.h>

namespace BloombergLP {
namespace bdlma {

                                // ==========
                                // class Pool
                                // ==========
  class PrintPool {
  public:
    PrintPool() {
      //      printMeOnce();
    }
    void printMeOnce()
    {
      static bool printed = false;
      if (!printed) {
	printed = true;
	printf("Pool\n");
      }
    }
  };
  

  class Pool { // : public PrintPool {
    SimPool * _allocVector { nullptr };

    // DATA
    bsls::Types::size_type  d_blockSize;          // size (in bytes) of each
                                                  // allocated memory block
                                                  // returned to client


    int                     d_chunkSize;          // current chunk size (in
                                                  // blocks-per-chunk)

    int                     d_maxBlocksPerChunk;  // maximum chunk size (in
                                                  // blocks-per-chunk)

    bsls::BlockGrowth::Strategy
                            d_growthStrategy;     // growth strategy of the
                                                  // chunk size

    InfrequentDeleteBlockList
                            d_blockList;          // memory manager for
                                                  // allocated memory

  private:
    // NOT IMPLEMENTED
    Pool(const Pool&);
    Pool& operator=(const Pool&);

  public:
    // CREATORS
    explicit
    Pool(bsls::Types::size_type  blockSize,
         bslma::Allocator       *basicAllocator = 0);
    Pool(bsls::Types::size_type       blockSize,
         bsls::BlockGrowth::Strategy  growthStrategy,
         bslma::Allocator            *basicAllocator = 0);
    Pool(bsls::Types::size_type       blockSize,
         bsls::BlockGrowth::Strategy  growthStrategy,
         int                          maxBlocksPerChunk,
         bslma::Allocator            *basicAllocator = 0);
        // Create a memory pool that returns blocks of contiguous memory of the
        // specified 'blockSize' (in bytes) for each 'allocate' method
        // invocation.  Optionally specify a 'growthStrategy' used to control
        // the growth of internal memory chunks (from which memory blocks are
        // dispensed).  If 'growthStrategy' is not specified, geometric growth
        // is used.  Optionally specify 'maxBlocksPerChunk' as the maximum
        // chunk size if 'growthStrategy' is specified.  If geometric growth is
        // used, the chunk size grows starting at 'blockSize', doubling in size
        // until the size is exactly 'blockSize * maxBlocksPerChunk'.  If
        // constant growth is used, the chunk size is always
        // 'blockSize * maxBlocksPerChunk'.  If 'maxBlocksPerChunk' is not
        // specified, an implementation-defined value is used.  Optionally
        // specify a 'basicAllocator' used to supply memory.  If
        // 'basicAllocator' is 0, the currently installed default allocator is
        // used.  The behavior is undefined unless '1 <= blockSize' and
        // '1 <= maxBlocksPerChunk'.

    ~Pool();
        // Destroy this pool, releasing all associated memory back to the
        // underlying allocator.

    // MANIPULATORS
    void *allocate();
        // Return the address of a contiguous block of maximally-aligned memory
        // having the fixed block size specified at construction.

    void deallocate(void *address);
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

    void reserveCapacity(int numBlocks);
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
void *operator new(bsl::size_t size, BloombergLP::bdlma::Pool& pool);
    // Return a block of memory of the specified 'size' (in bytes) allocated
    // from the specified 'pool'.  The behavior is undefined unless 'size' is
    // the same or smaller than the 'blockSize' with which 'pool' was
    // constructed.  Note that an object may allocate additional memory
    // internally, requiring the allocator to be passed in as a constructor
    // argument:
    //..
    //  my_Type *newMyType(bdlma::Pool *pool, bslma::Allocator *basicAllocator)
    //  {
    //      return new (*pool) my_Type(..., basicAllocator);
    //  }
    //..
    // Also note that the analogous version of 'operator delete' should not be
    // called directly.  Instead, this component provides a static template
    // member function, 'deleteObject', parameterized by 'TYPE':
    //..
    //  void deleteMyType(my_Type *t, bdlma::Pool *pool)
    //  {
    //      pool->deleteObject(t);
    //  }
    //..
    // 'deleteObject' performs the following:
    //..
    //  t->~my_Type();
    //  pool->deallocate(t);
    //..

void operator delete(void *address, BloombergLP::bdlma::Pool& pool);
    // Use the specified 'pool' to deallocate the memory at the specified
    // 'address'.  The behavior is undefined unless 'address' is non-zero, was
    // allocated using 'pool', and has not already been deallocated.  Note that
    // this operator is supplied solely to allow the compiler to arrange for it
    // to be called in the case of an exception.

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

namespace BloombergLP {
namespace bdlma {

                                // ----------
                                // class Pool
                                // ----------
inline Pool::Pool(bsls::Types::size_type blockSize, bslma::Allocator *basicAllocator)
: _allocVector(new SimPool)
, d_blockSize(blockSize)
, d_growthStrategy(bsls::BlockGrowth::BSLS_GEOMETRIC)
, d_blockList(basicAllocator)
{
}

inline Pool::Pool(bsls::Types::size_type       blockSize,
           bsls::BlockGrowth::Strategy  growthStrategy,
           bslma::Allocator            *basicAllocator)
: _allocVector(new SimPool)
, d_blockSize(blockSize)
, d_growthStrategy(growthStrategy)
, d_blockList(basicAllocator)
{
    BSLS_ASSERT(1 <= blockSize);
}

inline Pool::Pool(bsls::Types::size_type       blockSize,
           bsls::BlockGrowth::Strategy  growthStrategy,
           int                          /*maxBlocksPerChunk*/,
           bslma::Allocator            *basicAllocator)
: _allocVector(new SimPool)
, d_blockSize(blockSize)
, d_growthStrategy(growthStrategy)
, d_blockList(basicAllocator)
{
    BSLS_ASSERT(1 <= blockSize);
    // BSLS_ASSERT(1 <= maxBlocksPerChunk);
}

inline Pool::~Pool()
{
}

// MANIPULATORS
inline void Pool::reserveCapacity(int /*numBlocks*/)
{
    // BSLS_ASSERT(0 <= numBlocks);
}

// MANIPULATORS
inline
void *Pool::allocate()
{
    return (char *) _allocVector->malloc(d_blockSize);
}

inline
void Pool::deallocate(void *address)
{
    _allocVector->free(address);
}

template <class TYPE>
inline
void Pool::deleteObject(const TYPE *object)
{
    bslma::DeleterHelper::deleteObject(object, this);
}

template <class TYPE>
inline
void Pool::deleteObjectRaw(const TYPE *object)
{
    bslma::DeleterHelper::deleteObjectRaw(object, this);
}

inline
void Pool::release()
{
    _allocVector->release();
}

// ACCESSORS
inline
bsls::Types::size_type Pool::blockSize() const
{
    return d_blockSize;
}

// Aspects

inline
bslma::Allocator *Pool::allocator() const
{
    return d_blockList.allocator();
}

}  // close package namespace
}  // close enterprise namespace

// FREE OPERATORS
inline
void *operator new(bsl::size_t size, BloombergLP::bdlma::Pool& pool)
{
    using namespace BloombergLP;

    BSLS_ASSERT_SAFE(size <= pool.blockSize() &&
        bsls::AlignmentUtil::calculateAlignmentFromSize(size)
         <= bsls::AlignmentUtil::calculateAlignmentFromSize(pool.blockSize()));

    static_cast<void>(size);  // suppress "unused parameter" warnings
    return pool.allocate();
}

inline
void operator delete(void *address, BloombergLP::bdlma::Pool& pool)
{
    BSLS_ASSERT_SAFE(address);

    pool.deallocate(address);
}

#endif
