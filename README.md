# Husky Malloc

######A free-list based memory allocator

Husky Malloc provides three functions:

*   void* hmalloc(size_t size); // Allocate "size" bytes of memory.
*   void hfree(void* item); // Free the memory pointed to by "item".
*   void hprintstats(); // Print allocator stats to stderr in a specific format.

## hmalloc
######The memory allocation processes works as follows:

To allocate memory B bytes of memory, first sizeof(size_t) is added to B to make space to track the size of the block, and then:

For requests with (B < 1 page = 4096 bytes):

*   See if there's a big enough block on the free list. If so, select the first one and remove it from the list.
*   If the program doesn't don't have a block, mmap a new block (1 page = 4096).
*   If the block is bigger than the request, and the leftover is big enough to store a free list cell, return the extra to the free list.
*   Use the start of the block to store its size.
*   Return a pointer to the block _after_ the size field.

For requests with (B >= 1 page = 4096 bytes):

*   Calculate the number of pages needed for this block.
*   Allocate that many pages with mmap.
*   Fill in the size of the block as (# of pages * 4096).
*   Return a pointer to the block _after_ the size field.

The allocator maintains a free list of available blocks of memory. This is a singly linked list sorted by block address. 

When inserting items into the free list,two invariants are maintained:

*   The free list is sorted by memory address of the blocks.
*   Any two adjacent blocks on the free list get coalesced (joined together) into one bigger block.

## Stat Tracker

The memory allocator tracks 5 stats:

*   pages_mapped: How many pages total has the allocator requested with mmap?
*   pages_unmapped: How many pages total has the allocator given back with munmap?
*   chunks_allocated: How many hmalloc calls have happened?
*   chunks_freed: How many hfree calls have happend?
*   free_length: How long is the free list at the time of the call?



