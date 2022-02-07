#pragma once

#ifndef SHIM_MULTIPOOL_HPP
#define SHIM_MULTIPOOL_HPP

#include <bsls_ident.h>
BSLS_IDENT("$Id: $")

#include <bdlma_simpool.h>

#include <bdlscm_version.h>

#include <bdlma_blocklist.h>
#include <bdlma_pool.h>

#include <bslma_allocator.h>
#include <bslma_deleterhelper.h>

#include <bsls_alignmentutil.h>
#include <bsls_blockgrowth.h>
#include <bsls_types.h>

namespace BloombergLP {
namespace bdlma {

                             // ===============
                             // class Multipool
                             // ===============

  class PrintMultipool {
  public:
    PrintMultipool() {
      // printMeOnce();
    }
    void printMeOnce()
    {
      static bool printed = false;
      if (!printed) {
	printed = true;
	printf("Multipool\n");
      }
    }
  };
  
  class Multipool { // : public PrintMultipool {


    SimPool pool;

    // DATA
    int                     d_numPools;      // number of memory pools

    bslma::Allocator       *d_allocator_p;   // holds (but does not own)
                                             // allocator

  private:
    // PRIVATE MANIPULATORS
    void initialize(bsls::BlockGrowth::Strategy growthStrategy,
                    int                         maxBlocksPerChunk);
    void initialize(const bsls::BlockGrowth::Strategy *growthStrategyArray,
                    int                                maxBlocksPerChunk);
    void initialize(bsls::BlockGrowth::Strategy  growthStrategy,
                    const int                   *maxBlocksPerChunkArray);
    void initialize(const bsls::BlockGrowth::Strategy *growthStrategyArray,
                    const int                         *maxBlocksPerChunkArray);
        // Initialize this multipool with the specified 'growthStrategy[Array]'
        // and 'maxBlocksPerChunk[Array]'.  If an array is used, each
        // individual 'bdlma::Pool' maintained by this multipool is initialized
        // with the corresponding growth strategy or max blocks per chunk entry
        // within the array.

    // PRIVATE ACCESSORS

  private:
    // NOT IMPLEMENTED
    Multipool(const Multipool&);
    Multipool& operator=(const Multipool&);

  public:
    // CREATORS
    explicit
    Multipool(bslma::Allocator *basicAllocator = 0)
    : pool()
    , d_numPools(10)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))

    {}
    explicit
    Multipool(int numPools, bslma::Allocator *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
    explicit
    Multipool(bsls::BlockGrowth::Strategy  growthStrategy,
              bslma::Allocator            *basicAllocator = 0)
    : pool()
    , d_numPools(10)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
    Multipool(int                          numPools,
              bsls::BlockGrowth::Strategy  growthStrategy,
              bslma::Allocator            *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
    Multipool(int                          numPools,
              bsls::BlockGrowth::Strategy  growthStrategy,
              int                          maxBlocksPerChunk,
              bslma::Allocator            *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
        // Create a multipool memory manager.  Optionally specify 'numPools',
        // indicating the number of internally created 'bdlma::Pool' objects;
        // the block size of the first pool is 8 bytes, with the block size of
        // each additional pool successively doubling.  If 'numPools' is not
        // specified, an implementation-defined number of pools 'N' -- covering
        // memory blocks ranging in size from '2^3 = 8' to '2^(N+2)' -- are
        // created.  Optionally specify a 'growthStrategy' indicating whether
        // the number of blocks allocated at once for every internally created
        // 'bdlma::Pool' should be either fixed or grow geometrically, starting
        // with 1.  If 'growthStrategy' is not specified, the allocation
        // strategy for each internally created 'bdlma::Pool' object is
        // geometric, starting from 1.  If 'numPools' and 'growthStrategy' are
        // specified, optionally specify a 'maxBlocksPerChunk', indicating the
        // maximum number of blocks to be allocated at once when a pool must be
        // replenished.  If 'maxBlocksPerChunk' is not specified, an
        // implementation-defined value is used.  Optionally specify a
        // 'basicAllocator' used to supply memory.  If 'basicAllocator' is 0,
        // the currently installed default allocator is used.  Memory
        // allocation (and deallocation) requests will be satisfied using the
        // internally maintained pool managing memory blocks of the smallest
        // size not less than the requested size, or directly from the
        // underlying allocator (supplied at construction), if no internal pool
        // managing memory blocks of sufficient size exists.  The behavior is
        // undefined unless '1 <= numPools' and '1 <= maxBlocksPerChunk'.  Note
        // that, on platforms where
        // '8 < bsls::AlignmentUtil::BSLS_MAX_ALIGNMENT', excess memory may be
        // allocated for pools managing smaller blocks.  Also note that
        // 'maxBlocksPerChunk' need not be an integral power of 2; if geometric
        // growth would exceed the maximum value, the chunk size is capped at
        // that value.

    Multipool(int                                numPools,
              const bsls::BlockGrowth::Strategy *growthStrategyArray,
              bslma::Allocator                  *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
    Multipool(int                                numPools,
              const bsls::BlockGrowth::Strategy *growthStrategyArray,
              int                                maxBlocksPerChunk,
              bslma::Allocator                  *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
    Multipool(int                          numPools,
              bsls::BlockGrowth::Strategy  growthStrategy,
              const int                   *maxBlocksPerChunkArray,
              bslma::Allocator            *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
    Multipool(int                                numPools,
              const bsls::BlockGrowth::Strategy *growthStrategyArray,
              const int                         *maxBlocksPerChunkArray,
              bslma::Allocator                  *basicAllocator = 0)
    : pool()
    , d_numPools(numPools)
    , d_allocator_p(bslma::Default::allocator(basicAllocator))
    {}
        // Create a multipool memory manager having the specified 'numPools',
        // indicating the number of internally created 'bdlma::Pool' objects;
        // the block size of the first pool is 8 bytes, with the block size of
        // each additional pool successively doubling.  Optionally specify a
        // 'growthStrategy' indicating whether the number of blocks allocated
        // at once for every internally created 'bdlma::Pool' should be either
        // fixed or grow geometrically, starting with 1.  If 'growthStrategy'
        // is not specified, optionally specify a 'growthStrategyArray',
        // indicating the strategies for each individual 'bdlma::Pool' created
        // by this object.  If neither 'growthStrategy' nor
        // 'growthStrategyArray' is specified, the allocation strategy for each
        // internally created 'bdlma::Pool' object will grow geometrically,
        // starting from 1.  Optionally specify a 'maxBlocksPerChunk',
        // indicating the maximum number of blocks to be allocated at once when
        // a pool must be replenished.  If 'maxBlocksPerChunk' is not
        // specified, optionally specify a 'maxBlocksPerChunkArray', indicating
        // the maximum number of blocks to allocate at once for each
        // individually created 'bdlma::Pool' object.  If neither
        // 'maxBlocksPerChunk' nor 'maxBlocksPerChunkArray' is specified, an
        // implementation-defined value is used.  Optionally specify a
        // 'basicAllocator' used to supply memory.  If 'basicAllocator' is 0,
        // the currently installed default allocator is used.  Memory
        // allocation (and deallocation) requests will be satisfied using the
        // internally maintained pool managing memory blocks of the smallest
        // size not less than the requested size, or directly from the
        // underlying allocator (supplied at construction), if no internal pool
        // managing memory blocks of sufficient size exists.  The behavior is
        // undefined unless '1 <= numPools', 'growthStrategyArray' has at least
        // 'numPools' strategies, '1 <= maxBlocksPerChunk', and
        // 'maxBlocksPerChunkArray' has at least 'numPools' positive values.
        // Note that, on platforms where
        // '8 < bsls::AlignmentUtil::BSLS_MAX_ALIGNMENT', excess memory may be
        // allocated for pools managing smaller blocks.  Also note that the
        // maximum need not be an integral power of 2; if geometric growth
        // would exceed a maximum value, the chunk size is capped at that
        // value.

    ~Multipool()
    {
      ///        delete d_allocator_p;
    }
        // Destroy this multipool.  All memory allocated from this memory pool
        // is released.

    // MANIPULATORS
    void *allocate(bsls::Types::size_type size)
    {
        return pool.malloc(size);
    }
        // Return the address of a contiguous block of maximally-aligned memory
        // of (at least) the specified 'size' (in bytes).  If 'size' is 0, no
        // memory is allocated and 0 is returned.  If
        // 'size > maxPooledBlockSize()', the memory allocation is managed
        // directly by the underlying allocator, and will not be pooled, but
        // will be deallocated when the 'release' method is called, or when
        // this object is destroyed.

    void deallocate(void *address)
    {
        pool.free(address);
    }
        // Relinquish the memory block at the specified 'address' back to this
        // multipool object for reuse.  The behavior is undefined unless
        // 'address' is non-zero, was allocated by this multipool object, and
        // has not already been deallocated.

    template <class TYPE>
    void deleteObject(const TYPE *object);
        // Destroy the specified 'object' based on its dynamic type and then
        // use this multipool object to deallocate its memory footprint.  This
        // method has no effect if 'object' is 0.  The behavior is undefined
        // unless 'object', when cast appropriately to 'void *', was allocated
        // using this multipool object and has not already been deallocated.
        // Note that 'dynamic_cast<void *>(object)' is applied if 'TYPE' is
        // polymorphic, and 'static_cast<void *>(object)' is applied otherwise.

    template <class TYPE>
    void deleteObjectRaw(const TYPE *object);
        // Destroy the specified 'object' and then use this multipool to
        // deallocate its memory footprint.  This method has no effect if
        // 'object' is 0.  The behavior is undefined unless 'object' is !not! a
        // secondary base class pointer (i.e., the address is (numerically) the
        // same as when it was originally dispensed by this multipool), was
        // allocated using this multipool, and has not already been
        // deallocated.

    void release()
    {
        pool.release();
    }
        // Relinquish all memory currently allocated via this multipool object.

    void reserveCapacity(bsls::Types::size_type size, int numBlocks)
    {}
        // Reserve memory from this multipool to satisfy memory requests for at
        // least the specified 'numBlocks' having the specified 'size' (in
        // bytes) before the pool replenishes.  If 'size' is 0, this method has
        // no effect.  The behavior is undefined unless
        // 'size <= maxPooledBlockSize()' and '0 <= numBlocks'.

    // ACCESSORS
    int numPools() const;
        // Return the number of pools managed by this multipool object.

    bsls::Types::size_type maxPooledBlockSize() const;
        // Return the maximum size of memory blocks that are pooled by this
        // multipool object.  Note that the maximum value is defined as:
        //..
        //  2 ^ (numPools + 2)
        //..
        // where 'numPools' is either specified at construction, or an
        // implementation-defined value.

                                  // Aspects

    bslma::Allocator *allocator() const;
        // Return the allocator used by this object to allocate memory.  Note
        // that this allocator can not be used to deallocate memory
        // allocated through this pool.
};

// ============================================================================
//                             INLINE DEFINITIONS
// ============================================================================

                             // ---------------
                             // class Multipool
                             // ---------------

// MANIPULATORS
template <class TYPE>
inline
void Multipool::deleteObject(const TYPE *object)
{
    if (nullptr != object)
    {
#ifndef BSLS_PLATFORM_CMP_SUN
        object->~TYPE();
#else
        const_cast<TYPE *>(object)->~TYPE();
#endif
        pool.free(const_cast<TYPE *>(object));
    }
}

template <class TYPE>
inline
void Multipool::deleteObjectRaw(const TYPE *object)
{
    if (nullptr != object)
    {
        void *address = const_cast<TYPE *>(object);

#if defined(BSLS_PLATFORM_CMP_SUN)
        const_cast<TYPE *>(object)->~TYPE();
#else
        object->~TYPE();
#endif

        pool.free(const_cast<TYPE *>(object));
    }
}

// ACCESSORS
inline
int Multipool::numPools() const
{
    return d_numPools;
}

inline
bsls::Types::size_type Multipool::maxPooledBlockSize() const
{
    return 8 << (d_numPools - 1);
}

// Aspects

inline
bslma::Allocator *Multipool::allocator() const
{
    return d_allocator_p;
}

}  // close package namespace
}  // close enterprise namespace

#endif
