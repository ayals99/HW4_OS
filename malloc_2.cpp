// Note that once "size" field in the metadata is set for a block in this section in the metadata,
// it’s not going to change.

#include <unistd.h>
#define NOT_FOUND NULL
#define MAX_SMALLOC_SIZE 100000000 // 10^8
#define ZERO_SMALLOC 0


struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
    MallocMetadata* getNext(){
        return next;
    }
    MallocMetadata* getPrev(){
        return prev;
    }
    size_t getSize(){
        return size;
    }
    bool isFree(){
        return is_free;
    }
};

struct MetadataList {
    MallocMetadata* head;
    MallocMetadata* tail;

    void* _findAvailableBlock(size_t size){
        MallocMetadata* current = freeList.head;
        while (current != NULL) {
            if (current->size >= size) {
                return current;
            }
            current = current->next;
        }
        return NOT_FOUND;
    }

    void addToList(MallocMetadata* metadata) {
        if (head == NULL) {
            head = metadata;
            tail = metadata;
            return;
        }

        // check if need to add to end of list
        // by checking if the address is higher than the tail's address:
        if (metadata > tail) {
            tail->next = metadata;
            metadata->prev = tail;
            tail = metadata;
            return;
        }

        // check if need to add to start of list
        // by checking if the address is lower than the head's address:
        if (metadata < head) {
            metadata->next = head;
            head->prev = metadata;
            head = metadata;
            return;
        }

        MallocMetadata* current = head;

        // if reached here then need to add to middle of list, since not head and not tail:
        // will add before the first block with a higher address
        while (current != NULL) {
            if (metadata < current){
                metadata->next = current;
                metadata->prev = current->prev;
                current->prev->next = metadata; // notice prev!=NULL exists since "current" is not head
                current->prev = metadata;
                return;
            }
            current = current->next;
        }
    }

    void removeFromList(MallocMetadata* metadata){
        if (metadata == head) {
            head = metadata->next;
        }
        if (metadata == tail) {
            tail = metadata->prev;
        }
        if (metadata->prev != NULL) {
            metadata->prev->next = metadata->next;
        }
        if (metadata->next != NULL) {
            metadata->next->prev = metadata->prev;
        }
    }
};



/** Global variables */
MetadataList freeList = {NULL, NULL};

/**
 * Searches for a free block with at least ‘size’ bytes
 * or allocates (sbrk()) one if none are found.
 * @Return:
 * i. Success – returns pointer to the first byte in the allocated block
 *      (excluding the meta-data of course)
 * ii. Failure –
 *      a. If size is 0 returns NULL.
 *      b. If ‘size’ is more than 108, return NULL.
 *      c. If sbrk fails in allocating the needed space, return NULL*/
void* smalloc(size_t size){
    if (size == ZERO_SMALLOC || size > MAX_SMALLOC_SIZE) {
        return NULL;
    }

    // search through list for a block at least size bytes
    void* newBlock = freeList._findAvailableBlock(size);
    // if none found, allocate a new block using sbrk
    if (newBlock == NOT_FOUND) {
        // allocate new block with extra room for metadata
        newBlock = sbrk(size + sizeof(MallocMetadata));
    }
    // return NULL if sbrk  failed
    if (newBlock == (void*)-1) {
        return NULL;
    }

    // write metadata at start of new block
    MallocMetadata* metadata = (MallocMetadata*)newBlock;
    metadata->size = size;
    metadata->is_free = false;
    metadata->next = NULL;
    metadata->prev = NULL;

    // return pointer to first byte in allocated block if successful
    // notice that we want to jump sizeof(MallocMetadata) bytes ahead
    // and char is 1 byte, so we cast to char* and add sizeof(MallocMetadata):
    return (void*)((char*)newBlock + sizeof(MallocMetadata));
}

/**
 * Searches for a free block of at least ‘num’ elements,
 * each ‘size’ bytes that are all set to 0 or allocates if none are found.
 * In other words, find/allocate (size * num) bytes and set all bytes to 0.
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

/** @Return: Number of allocated blocks in the heap that are currently free.*/
size_t _num_free_blocks(){

}

/** @Return: Number of bytes in all allocated blocks in the heap that are currently free,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_free_bytes(){

}

/** @Return: the overall (free and used) number of allocated blocks in the heap.*/
size_t _num_allocated_blocks(){

}

/** @Return: The overall number (free and used) of allocated bytes in the heap,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_allocated_bytes(){

}

/** @Return: The overall number of meta-data bytes currently in the heap.*/
size_t _num_meta_data_bytes(){

}

/** @Return: The number of bytes of a single meta-data structure in your system. */
size_t _size_meta_data(){

}