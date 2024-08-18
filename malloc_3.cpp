#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <cstdint>
#include <math.h>

#define NOT_FOUND NULL
#define MAX_SMALLOC_SIZE 100000000 // 10^8
#define ZERO_SMALLOC 0
#define ZERO 0

static const int NUMBER_OF_LEVELS = 11;
static const int MAX_ORDER = 10;

static const int MAX_ORDER_BLOCK_SIZE = 131072;// 128kb = 2^17 bytes
static const int INITIAL_MALLOC_BLOCK_AMOUNT = 32;
static const int PROGRAM_BREAK_DIVISOR= 4194304; // 32*(128kb) bytes

static const bool FIRST_ALLOC_SUCCESS = true;
static const bool FIRST_ALLOC_FAIL = false;

static const bool ADD_TO_LIST_SUCCESS = true;
static const bool ADD_TO_LIST_FAIL = false;

static const bool REMOVE_FROM_LIST_SUCCESS = true;
static const bool REMOVE_FROM_LIST_FAIL = false;

static const int ORDER_SIZE = 128;


struct MallocMetadata{
    size_t size;
    bool is_free;
    MallocMetadata* next;
    MallocMetadata* prev;
};

struct MetadataList{
    MallocMetadata* head;
    int num_blocks;
    int num_bytes;

    MetadataList() : head(NULL), num_blocks(ZERO), num_bytes(ZERO) {}

    bool _addToList(MallocMetadata *metadata) {
        // if the list is empty:
        if (num_blocks == ZERO) {
            metadata->next = NULL;
            metadata->prev = NULL;
            head = metadata;
            num_blocks++;
            num_bytes += metadata->size;
            return ADD_TO_LIST_SUCCESS;
        }

        // if the list is not empty:

        // insert before head
        if (metadata < head) {
            metadata->next = head;
            metadata->prev = NULL;
            head->prev = metadata;
            head = metadata;
            num_blocks++;
            num_bytes += metadata->size;
            return ADD_TO_LIST_SUCCESS;
        }

        // from here, assume metadata is larger than the head:
        MallocMetadata* current = head;
        while (current != NULL) {
            // in case we already have the same address, we don't want to insert it again:
            if (metadata == current) {
                return ADD_TO_LIST_FAIL;
            }
            if (metadata > current && (current->next != NULL) && (metadata) < current->next ) {
                // insert between current and current->next
                metadata->next = current->next;
                metadata->prev = current;
                current->next->prev = metadata;
                current->next = metadata;
                num_blocks++;
                num_bytes += metadata->size;
                return ADD_TO_LIST_SUCCESS;
            }
            if (metadata > current && current->next == NULL) {
                // reached last node, insert after current
                metadata->next = NULL;
                metadata->prev = current;
                current->next = metadata;
                num_blocks++;
                num_bytes += metadata->size;
                return ADD_TO_LIST_SUCCESS;
            }
            current = current->next;
        }
        return ADD_TO_LIST_FAIL;
    }

    bool isEmpty(){
        return (num_blocks == ZERO);
    }

    MallocMetadata* _popHead(){
        if (isEmpty()) {
            return NULL;
        }
        MallocMetadata* oldHead = head;
        head = oldHead->next;
        oldHead->next = NULL;

        // if the new head is not NULL, then we need to update its prev pointer to NULL:
        if (head != NULL) {
            head->prev = NULL;
        }

        num_blocks--;
        num_bytes -= temp->size;
        return oldHead;
    }

    bool _removeFromList(MallocMetadata* toRemove) {
        if (toRemove == NULL || isEmpty()) {
            return REMOVE_FROM_LIST_FAIL;
        }
        if (toRemove == head) {
            head = toRemove->next;
            toRemove->next = NULL;
            if (head != NULL) {
                head->prev = NULL;
            }
        }
        else {
            if (toRemove->next != NULL) {
                toRemove->next->prev = toRemove->prev;
            }

            // note that "toRemove" is not the head, so it's prev is not NULL:
            toRemove->prev->next = toRemove->next;

            // set toRemove next and prev to NULL:
            toRemove->next = NULL;
            toRemove->prev = NULL;
        }
        num_blocks--;
        num_bytes -= toRemove->size;
        return REMOVE_FROM_LIST_SUCCESS;
    }
};

MetadataList usedSBRKBlockList = MetadataList();
MetadataList mmapedList = MetadataList();
MetadataList freeBlockListArray[NUMBER_OF_LEVELS]; // 0-10

bool firstAlloc = true;

bool firstAllocator(){
    // Initialize the array of lists of free blocks
    for (int i = 0; i < NUMBER_OF_LEVELS; i++) {
        freeBlockListArray[i] = MetadataList();
    }

    return FIRST_ALLOC_SUCCESS;
}



void* allocateByMmap(size_t size){
    void* newBlock = mmap(NULL, size + sizeof(MallocMetadata),
                          PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (newBlock == (void*)(-1)) {
        return (void*)(-1);
    }
    MallocMetadata* newBlockMetadata = (MallocMetadata*)newBlock;
    newBlockMetadata->size = size;
    newBlockMetadata->is_free = false;
    newBlockMetadata->next = NULL;
    newBlockMetadata->prev = NULL;

    mmapedList._addToList(newBlockMetadata);

//    num_allocated_blocks++;
//    num_allocated_bytes += (int)size;

    return (void*) ((char*)newBlock + sizeof(MallocMetadata));
}

int findTightOrder(size_t size){
    for (int i = 0; i <= MAX_ORDER; i++) {
        if (size <= ( (128 * (1 << i) ) - sizeof(MallocMetadata)) ) {
            return i;
        }
    }
    return MAX_ORDER;
}

size_t getOrderSize(int order){
    return (size_t)(128 * (1 << order));
}

void* splitBlock(MallocMetadata* blockToSplit, int highOrder, int tightOrder){

    return
}

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
void* smalloc(size_t size) {

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


// find buddy address
void* findBuddyAddress(MallocMetadata* block){

}

//find buddy address by size, so we don't have to change the size of the block to find the buddy:
void* findBuddyAddressBySize(MallocMetadata* block, size_t size){

}

// receives two blocks of the same size, buddy is free but blockMetadata might not be free:
MallocMetadata* mergeBlocks(MallocMetadata* blockMetadata, MallocMetadata* buddyAddress){

}

/**
 * Releases the usage of the block that starts with the pointer ‘p’.
 * If ‘p’ is NULL or already released, simply returns.
 * Presume that all pointers ‘p’ truly points to the beginning of an allocated block*/

//  Assume that the pointers sent to sfree are
//  legal pointers that point to the first allocated byte,
//  the same ones that are returned by the allocation functions.
void sfree(void* p) {

}

MallocMetadata* pickLeftBlock(MallocMetadata* block1, MallocMetadata* block2){
    return (reinterpret_cast<std::uintptr_t>(block1) < reinterpret_cast<std::uintptr_t>(block2)) ? block1 : block2;
}

// check if we can allocate the requested size by merging blocks
// so we don't have to merge blocks and then unmerge them if it's not possible to allocate the requested size
bool canAllocateWithMerges(MallocMetadata* oldpBlockMetadata, int requestedOrder) {

}

MallocMetadata* mergeAndStopAtOrder(MallocMetadata* oldpBlockMetadata, int requestedOrder){

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
    int numFreeBlocks = ZERO;
    for (int i = 0; i <= MAX_ORDER; i++) {
        numFreeBlocks += freeBlockListArray[i].num_blocks;
    }
    return (size_t)numFreeBlocks;
}

/** @Return: Number of bytes in all allocated blocks in the heap that are currently free,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_free_bytes(){
    int numFreeBytes = ZERO;
    for (int i = 0; i <= MAX_ORDER; i++) {
        numFreeBytes += freeBlockListArray[i].num_bytes;
    }
    return (size_t)numFreeBytes;
}

/** @Return: the overall (free and used) number of allocated blocks in the heap.*/
size_t _num_allocated_blocks(){
    int numAllocatedBlocks = ZERO;
    for (int i = 0; i <= MAX_ORDER; i++) {
        numAllocatedBlocks += freeBlockListArray[i].num_blocks;
    }
    numAllocatedBlocks += usedSBRKBlockList.num_blocks;
    numAllocatedBlocks += mmapedList.num_blocks;
    return (size_t)numAllocatedBlocks;
}

/** @Return: The overall number (free and used) of allocated bytes in the heap,
 * excluding the bytes used by the meta-data structs.*/
size_t _num_allocated_bytes(){
    int numAllocatedBytes = ZERO;
    for (int i = 0; i <= MAX_ORDER; i++) {
        numAllocatedBytes += freeBlockListArray[i].num_bytes;
    }
    numAllocatedBytes += usedSBRKBlockList.num_bytes;
    numAllocatedBytes += mmapedList.num_bytes;
    return (size_t)numAllocatedBytes;
}

/** @Return: The overall number of meta-data bytes currently in the heap.*/
size_t _num_meta_data_bytes(){
    return (size_t) (_num_allocated_blocks() * sizeof(MallocMetadata));
}

/** @Return: The number of bytes of a single meta-data structure in your system. */
size_t _size_meta_data(){
    return (size_t)sizeof(MallocMetadata);
}