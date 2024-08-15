// Note that once "size" field in the metadata is set for a block in this section in the metadata,
// it’s not going to change.

#include <unistd.h>
#include <string.h>

#define NOT_FOUND NULL
#define MAX_SMALLOC_SIZE 100000000 // 10^8
#define ZERO_SMALLOC 0
#define ZERO 0


struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

int num_free_block = ZERO; // every time we turn is_free to "true" we increment by 1
int num_free_bytes = ZERO; // every time we turn is_free to "true" we increment by the size of the block

int num_allocated_blocks = ZERO; // every time we add a block to the list we increment by 1
int num_allocated_bytes = ZERO; // every time we add a block to the list we increment by the size of the block

struct MetadataList {
    MallocMetadata* head;
    MallocMetadata* tail;

    MetadataList() : head(NULL), tail(NULL) {}

    void _addToList(MallocMetadata *metadata) {
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

        MallocMetadata *current = head;

        // if reached here then need to add to middle of list, since not head and not tail:
        // will add before the first block with a higher address
        while (current != NULL) {
            if (metadata < current) {
                metadata->next = current;
                metadata->prev = current->prev;
                current->prev->next = metadata; // notice prev!=NULL exists since "current" is not head
                current->prev = metadata;
                return;
            }
            current = current->next;
        }
    }
};

/** Global variables */
MetadataList blockList = MetadataList();



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
    void* newBlock = NOT_FOUND;

    MallocMetadata* current = blockList.head;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            current->is_free = false;
            num_free_block--;
            num_free_bytes -= (int)(current->size);
            newBlock = current;
            break;
        }
        current = current->next;
    }

    // if none found, allocate a new block using sbrk
    if (newBlock == NOT_FOUND) {
        // allocate new block with extra room for metadata
        newBlock = (MallocMetadata*) sbrk(size + sizeof(MallocMetadata));
        num_allocated_blocks++;
        num_allocated_bytes += (int)size;


        // return NULL if sbrk  failed
        if ( newBlock == (void*)(-1) ) {
            return NULL;
        }
        // update metadata list
        // write metadata at start of new block
        MallocMetadata* newBlockMetadata = (MallocMetadata*)newBlock;
        newBlockMetadata->size = size;
        newBlockMetadata->is_free = false;
        newBlockMetadata->next = NULL;
        newBlockMetadata->prev = NULL;

        blockList._addToList(newBlockMetadata);
    }



    // return pointer to first byte in allocated block if successful
    // notice that we want to jump sizeof(MallocMetadata) bytes ahead
    // and char is 1 byte, so we cast to char* and add sizeof(MallocMetadata):
    return (void*) ((char*)newBlock + sizeof(MallocMetadata));
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
    size_t totalSize = num * size;
    // notice that scalloc is similar to smalloc, so we can use smalloc to allocate the block
    // if totalSize is greater than 10^8, then smalloc will return NULL
    // notice: (totalSize == 0) if and only if (num == 0 || size == 0)
    void* newBlock = smalloc(totalSize);
    if (newBlock == NULL) {
        return NULL;
    }
    return memset(newBlock, 0, totalSize);
}

/**
 * Releases the usage of the block that starts with the pointer ‘p’.
 * If ‘p’ is NULL or already released, simply returns.
 * Presume that all pointers ‘p’ truly points to the beginning of an allocated block*/

//  Assume that the pointers sent to sfree are
//  legal pointers that point to the first allocated byte,
//  the same ones that are returned by the allocation functions.
void sfree(void* p){
    if (p == NULL) {
        return;
    }
    if ( ( (MallocMetadata*) ( ((char*)p) - sizeof(MallocMetadata) ))->is_free){
        return;
    }
    MallocMetadata* blockMetadata = (MallocMetadata*)( ((char*)p) - sizeof(MallocMetadata));
    blockMetadata->is_free = true;
    num_free_block++;
    num_free_bytes += (int)(blockMetadata->size);
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
    if (size == ZERO || size > MAX_SMALLOC_SIZE) {
        return NULL;
    }

    if (oldp == NULL) {
        return smalloc(size);
    }

    MallocMetadata* oldpBlockMetadata = (MallocMetadata*)( ((char*)oldp) - sizeof(MallocMetadata));
    size_t oldpSize = oldpBlockMetadata->size;

    if (size <= oldpSize){
        if (oldpBlockMetadata->is_free){
            oldpBlockMetadata->is_free = false;
            num_free_block--;
            num_free_bytes -= (int)(oldpSize);
        }
        return oldp;
    }
    else{ // if size > oldpBlockMetadata->size
        void* newBlock = smalloc(size);
        if (newBlock == NULL) {
            return NULL;
        }
        memmove(newBlock, oldp, oldpSize);
        sfree(oldp);
        return newBlock;
    }
}

/** @Return: Number of allocated blocks in the heap that are currently free.*/
size_t _num_free_blocks(){
    return (size_t)num_free_block;
}

/** @Return: Number of bytes in all allocated blocks in the heap that are currently free,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_free_bytes(){
    return (size_t)num_free_bytes;
}

/** @Return: the overall (free and used) number of allocated blocks in the heap.*/
size_t _num_allocated_blocks(){
    return (size_t)num_allocated_blocks;
}

/** @Return: The overall number (free and used) of allocated bytes in the heap,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_allocated_bytes(){
    return (size_t)num_allocated_bytes;
}

/** @Return: The overall number of meta-data bytes currently in the heap.*/
size_t _num_meta_data_bytes(){
    return (size_t) (num_allocated_blocks * sizeof(MallocMetadata));
}

/** @Return: The number of bytes of a single meta-data structure in your system. */
size_t _size_meta_data(){
    return (size_t)sizeof(MallocMetadata);
}