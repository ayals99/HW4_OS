// Note that once "size" field in the metadata is set for a block in this section in the metadata,
// it’s not going to change.
struct MallocMetadata {
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

/**
 * Searches for a free block with at least ‘size’ bytes or allocates (sbrk()) one if none are
 * found.
 * @Return:
 * i. Success – returns pointer to the first byte in the allocated block
 *      (excluding the meta-data of course)
 * ii. Failure –
 *      a. If size is 0 returns NULL.
 *      b. If ‘size’ is more than 108, return NULL.
 *      c. If sbrk fails in allocating the needed space, return NULL*/

void* smalloc(size_t size){

}

/**
 * Searches for a free block of at least ‘num’ elements,
 * each ‘size’ bytes that are all set to 0
 * or allocates if none are found.
 * In other words, find/allocate size * num bytes and set all
 * bytes to 0.
 * @Return:
 * i. Success - returns pointer to the first byte in the allocated block.
 * ii. Failure –
 *      a. If size or num is 0 returns NULL.
 *      b. If ‘size * num’ is more than 108, return NULL.
 *      c. If sbrk fails in allocating the needed space, return NULL.*/

// You should use std::memset for setting values to 0 in your scalloc().

void* scalloc(size_t num, size_t size){

}

/**
 * Releases the usage of the block that starts with the pointer ‘p’.
 * If ‘p’ is NULL or already released, simply returns.
 * Presume that all pointers ‘p’ truly points to the beginning of an allocated block*/

//  Assume that the pointers sent to sfree are
//  legal pointers that point to the first allocated byte,
//  the same ones that are returned by the allocation functions.
void sfree(void* p){

}

/**
 * If ‘size’ is smaller than or equal to the current block’s size, reuses the same block.
 * Otherwise, finds/allocates ‘size’ bytes for a new space, copies content of oldp into the
 * new allocated space and frees the oldp.
 * @Return:
 * Success –
 *      a. Returns pointer to the first byte in the (newly) allocated space.
 *      b. If ‘oldp’ is NULL, allocates space for ‘size’ bytes and returns a pointer to it.
 * Failure –
 *      a. If size is 0 returns NULL.
 *      b. If ‘size’ if more than 108, return NULL.
 *      c. If sbrk fails in allocating the needed space, return NULL.
 *      d. Do not free ‘oldp’ if srealloc() fails.*/
// You should use std::memmove for copying data in srealloc().

// Assume that the pointers sent to srealloc are
// legal pointers that point to the first allocated byte,
// the same ones that are returned by the allocation functions.

void* srealloc(void* oldp, size_t size){

}

/** Returns the number of allocated blocks in the heap that are currently free.*/
size_t _num_free_blocks(){

}

/** Returns the number of bytes in all allocated blocks in the heap that are currently free,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_free_bytes(){

}

/** Returns the overall (free and used) number of allocated blocks in the heap.*/
size_t _num_allocated_blocks(){

}

/** Returns the overall number (free and used) of allocated bytes in the heap,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_allocated_bytes(){

}

/** Returns the overall number of meta-data bytes currently in the heap.*/
size_t _num_meta_data_bytes(){

}

/** Returns the number of bytes of a single meta-data structure in your system. */
size_t _size_meta_data(){

}

