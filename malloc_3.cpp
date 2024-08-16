/** COPY OF MALLOC_2.CPP */
// Note that once "size" field in the metadata is set for a block in this section in the metadata,
// it’s not going to change.

#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <cstdint>

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

    bool isEmpty(){
        return (head == NULL);
    }

    MallocMetadata* _popHead(){
        if (head == NULL){
            return NULL;
        }
        MallocMetadata* oldHead = head;
        head = head->next;
        oldHead->next = NULL;
        if (head != NULL){
            head->prev = NULL;
        }
        else{ // if new head is NULL, then tail should be NULL as well
            tail = NULL;
        }
        return oldHead;
    }

    void _removeFromList(MallocMetadata* metadata) {
        if (metadata == head) {
            head = metadata->next;
            metadata->next = NULL;
            if (head != NULL) {
                head->prev = NULL;
            }
        }
        if (metadata == tail) {
            tail = metadata->prev;
            metadata->prev = NULL;
            if (tail != NULL) {
                tail->next = NULL;
            }
        }
        else { // if metadata is in the middle of the list
            if (metadata->next != NULL){
                metadata->next->prev = metadata->prev;
            }
            if (metadata->prev != NULL){
                metadata->prev->next = metadata->next;
            }
            metadata->next = NULL;
            metadata->prev = NULL;
        }
    }
};

/** Global variables */
//MetadataList blockList = MetadataList();


bool firstAlloc = true;

// need an of lists of blocks:
static const int NUMBER_OF_LEVELS = 11;
static const int MAX_ORDER = 10;

MetadataList freeBlockListArray[NUMBER_OF_LEVELS]; // 0-10

static const int MAX_ORDER_BLOCK_SIZE = 131072;// 128kb = 2^17 bytes
static const int INITIAL_MALLOC_BLOCK_AMOUNT = 32;
static const int PROGRAM_BREAK_DIVISOR= 4194304; // 32*(128kb) bytes

static const bool FIRST_ALLOC_SUCCESS = true;
static const bool FIRST_ALLOC_FAIL = false;

static const int ORDER_SIZE = 128;

bool firstAllocator(){
    // Initialize the array of lists of free blocks
    for (int i = 0; i < NUMBER_OF_LEVELS; i++) {
        freeBlockListArray[i] = MetadataList();
    }

    // make the program break a multiple of 32*128kb
    int addressRemainder = reinterpret_cast<uintptr_t>(sbrk(0)) % PROGRAM_BREAK_DIVISOR;
    if (addressRemainder != 0){
        if (sbrk(PROGRAM_BREAK_DIVISOR - addressRemainder) == (void*)(-1)) {
            return FIRST_ALLOC_FAIL;
        }
    }

    // move program break to allocate 32 blocks of 128kb
    if (sbrk(INITIAL_MALLOC_BLOCK_AMOUNT * MAX_ORDER_BLOCK_SIZE) == (void*)(-1)) {
        return FIRST_ALLOC_FAIL;
    }

    void* programBreak = sbrk(0);


    // split into 32 blocks, each 128kb:
    for (int i = 1; i <= INITIAL_MALLOC_BLOCK_AMOUNT; i++) {
        MallocMetadata* blockStart = (MallocMetadata*)((char*)programBreak - i * MAX_ORDER_BLOCK_SIZE);

        blockStart->size = MAX_ORDER_BLOCK_SIZE - sizeof(MallocMetadata);
        blockStart->is_free = true;
        blockStart->next = NULL;
        blockStart->prev = NULL;

        // add to list in the tenth index of the array:
        freeBlockListArray[MAX_ORDER]._addToList(blockStart);

        // update global counters:
        num_free_block++;
        num_allocated_blocks++;
        num_free_bytes += (int)(blockStart->size);
        num_allocated_bytes += (int)(MAX_ORDER_BLOCK_SIZE);
    }
    return FIRST_ALLOC_SUCCESS;
}

MetadataList mmapedList = MetadataList();

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

    num_allocated_blocks++;
    num_allocated_bytes += (int)size;

    mmapedList._addToList(newBlockMetadata);

    return (void*) ((char*)newBlock + sizeof(MallocMetadata));
}

int findTightOrder(size_t size){
    for (int i = 0; i <= MAX_ORDER; i++) {
        if (size < ( (128 * (2 << i) ) - sizeof(MallocMetadata)) ) {
            return i;
        }
    }
    return MAX_ORDER;
}

size_t getOrderSize(int order){
    return (size_t)(128 * (2 << order));
}

void* splitBlock(MallocMetadata* blockToSplit, int highOrder, int tightOrder){

    int amountOfSplits = highOrder - tightOrder;

    // decrement highOrder by one because we popped the head from it's list already:
    highOrder--;

    while(highOrder >= tightOrder){
        // split the block into two blocks:
        MallocMetadata* blockLeftHalf = blockToSplit;
        MallocMetadata* blockRightHalf = (MallocMetadata*) ((char*)blockToSplit + getOrderSize(highOrder));

        blockLeftHalf->size = getOrderSize(highOrder) - sizeof(MallocMetadata);
        blockLeftHalf->is_free = false; // leftmost black will be the final block
        blockLeftHalf->next = NULL;
        blockLeftHalf->prev = NULL;

        blockRightHalf->size = getOrderSize(highOrder) - sizeof(MallocMetadata);
        blockRightHalf->is_free = true;
        freeBlockListArray[highOrder]._addToList(blockRightHalf);

        highOrder--;
    }

    num_free_bytes -= amountOfSplits * sizeof(MetadataList);
    num_free_bytes -= (int)(blockToSplit->size);

    num_free_block += (amountOfSplits - 1);

    num_allocated_blocks += amountOfSplits;
    num_allocated_bytes -= amountOfSplits * sizeof(MetadataList);

    return (void*) ((char*)blockToSplit + sizeof(MallocMetadata));
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
    if (size == ZERO_SMALLOC || size > MAX_SMALLOC_SIZE) {
        return NULL;
    }

    if (firstAlloc) {
        if (firstAllocator() == FIRST_ALLOC_FAIL) {
            return NULL;
        }
    }
    if (size >= MAX_ORDER_BLOCK_SIZE) {
        // use mmap:
        return allocateByMmap(size);
    }

    // find order that will wrap size tightly
    int tightOrder = findTightOrder(size);

    // find a free block in the tight order
    if ( !( (freeBlockListArray[tightOrder].isEmpty()) ) ) {
        MallocMetadata* block = freeBlockListArray[tightOrder]._popHead();
        block->is_free = false;
        num_free_block--;
        num_free_bytes -= (int) (block->size);
        return (void*) ((char *) block + sizeof(MallocMetadata));
    }
    else { // if no free block found in the order, search in the next orders
        for (int highOrder = tightOrder + 1; highOrder <= MAX_ORDER; highOrder++) {
            if (!(freeBlockListArray[highOrder].isEmpty())) {
                // found a block with a higher order than needed
                MallocMetadata* blockToSplit = freeBlockListArray[highOrder]._popHead();
                // split the closest available block (if necessary) and return the new block
                return splitBlock(blockToSplit, highOrder, tightOrder);
            }
        }
    }
    return NULL;
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
    size_t blockSize = block->size + sizeof(MallocMetadata);
    std::uintptr_t blockAddress = reinterpret_cast<std::uintptr_t>((void*)block);
    std::uintptr_t buddyAddress = blockAddress ^ (std::uintptr_t)blockSize;
    return reinterpret_cast<void*>(buddyAddress);
}

MallocMetadata* mergeBlocks(MallocMetadata* blockMetadata, MallocMetadata* buddyAddress){
    MallocMetadata* leftBlock;
    MallocMetadata* rightBlock;

    reinterpret_cast<std::uintptr_t>(blockMetadata) < reinterpret_cast<std::uintptr_t>(buddyAddress) ?
                        (leftBlock = blockMetadata, rightBlock = buddyAddress)
                        :
                        (leftBlock = buddyAddress, rightBlock = blockMetadata);

    int order = findTightOrder(leftBlock->size);

    freeBlockListArray[order]._removeFromList(rightBlock);
    freeBlockListArray[order]._removeFromList(leftBlock);

    leftBlock->size = leftBlock->size + rightBlock->size + sizeof(MallocMetadata);

    leftBlock->is_free = true;

    num_free_block--;
    num_free_bytes += (int)sizeof(MallocMetadata);
    num_allocated_blocks--;
    num_allocated_bytes += (int)sizeof(MallocMetadata);

    return leftBlock;
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
    MallocMetadata* blockMetadata = (MallocMetadata*)( ((char*)p) - sizeof(MallocMetadata));
    if (blockMetadata->is_free){
        return;
    }
    if (blockMetadata->size >= MAX_ORDER_BLOCK_SIZE) {
        num_allocated_blocks--;
        num_allocated_bytes -= (int) (blockMetadata->size);

        // use munmap:
        munmap((void *) blockMetadata, blockMetadata->size + sizeof(MallocMetadata));
        mmapedList._removeFromList(blockMetadata);
        return;
    }
    else{ // if blockMetadata->size < MAX_ORDER_BLOCK_SIZE, meaning it was created by sbrk
        while(blockMetadata->size < (MAX_ORDER_BLOCK_SIZE - sizeof(MallocMetadata)) ) {
            MallocMetadata* buddyAddress = (MallocMetadata *) findBuddyAddress(blockMetadata);
            if ( !(buddyAddress->is_free) || (buddyAddress->size != blockMetadata->size)){
                break;
            }
            blockMetadata = mergeBlocks(blockMetadata, buddyAddress);
        }
        blockMetadata->is_free = true;
        int blockOrder = findTightOrder(blockMetadata->size);
        freeBlockListArray[blockOrder]._addToList(blockMetadata);

        num_free_block++;
        num_free_bytes += (int)(blockMetadata->size);
    }
}


MallocMetadata* pickLeftBlock(MallocMetadata* block1, MallocMetadata* block2){
    return (reinterpret_cast<std::uintptr_t>(block1) < reinterpret_cast<std::uintptr_t>(block2)) ? block1 : block2;
}

// find buddy address by size, so we don't have to change the size of the block to find the buddy:
void* findBuddyAddressBySize(MallocMetadata* block, size_t size){
    std::uintptr_t blockAddress = reinterpret_cast<std::uintptr_t>((void*)block);
    std::uintptr_t buddyAddress = blockAddress ^ (std::uintptr_t)size;
    return reinterpret_cast<void*>(buddyAddress);
}

// check if we can allocate the requested size by merging blocks
// so we don't have to merge blocks and then unmerge them if it's not possible to allocate the requested size
bool canAllocateWithMerges(MallocMetadata* oldpBlockMetadata, int requestedOrder) {
    MallocMetadata* blockToMerge = oldpBlockMetadata;
    size_t mergedSize = blockToMerge->size;
    while (mergedSize < (MAX_ORDER_BLOCK_SIZE - sizeof(MallocMetadata)) && mergedSize < getOrderSize(requestedOrder))
    {
        MallocMetadata* buddyAddress = (MallocMetadata *) findBuddyAddressBySize(blockToMerge, mergedSize);
        if ( !(buddyAddress->is_free) || (buddyAddress->size != mergedSize)) {
            return false;
        }
        blockToMerge = pickLeftBlock(blockToMerge, buddyAddress);
        mergedSize = mergedSize + buddyAddress->size + sizeof(MallocMetadata);
        if (mergedSize >= getOrderSize(requestedOrder)) {
            return true;
        }
    }
    return false;
}

MallocMetadata* mergeAndStopAtOrder(MallocMetadata* oldpBlockMetadata, int requestedOrder){
    MallocMetadata* blockToMerge = oldpBlockMetadata;
    while (blockToMerge->size < (MAX_ORDER_BLOCK_SIZE - sizeof(MallocMetadata)) && blockToMerge->size < getOrderSize(requestedOrder))
    {
        MallocMetadata* buddyAddress = (MallocMetadata*) findBuddyAddress(blockToMerge);
        if ( !(buddyAddress->is_free) || (buddyAddress->size != blockToMerge->size)){
            return NULL;
        }
        blockToMerge = mergeBlocks(blockToMerge, buddyAddress);
        if (blockToMerge->size >= getOrderSize(requestedOrder)){
            return blockToMerge;
        }
    }
    return NULL;
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

    if (oldp == NULL || size >= MAX_ORDER_BLOCK_SIZE) {
        return smalloc(size);
    }

    // from here on we assume that oldp was allocated from the blocks created by sbrk
    MallocMetadata* oldpBlockMetadata = (MallocMetadata*)( ((char*)oldp) - sizeof(MallocMetadata));
    size_t oldpSize = oldpBlockMetadata->size;

    if ((oldpSize >= (size_t)MAX_ORDER_BLOCK_SIZE) || (size >= (size_t)MAX_ORDER_BLOCK_SIZE)){
        // if oldp was allocated by mmap, then we can't reuse it unless the size is the same
        if (size == oldpSize){
            if (oldpBlockMetadata->is_free){
                num_free_bytes -= (int)(oldpBlockMetadata->size);
                num_free_block--;
                oldpBlockMetadata->is_free = false;
            }
            return oldp;
        }
        else{
            void* newBlock = smalloc(size);
            if (newBlock == NULL) {
                return NULL;
            }
            memmove(newBlock, oldp, oldpBlockMetadata->size);
            sfree(oldp);
            return newBlock;
        }
    }

    // from here on we assume that oldp was allocated by the initial sbrk

    if (size <= oldpSize){
        int tightOrder = findTightOrder(size);
        int oldpOrder = findTightOrder(oldpSize);
        if (tightOrder == oldpOrder){
            return oldp;
        }
        else{
            MallocMetadata* blockToSplit = oldpBlockMetadata;
            return splitBlock(blockToSplit, oldpOrder, tightOrder);
        }
    }
    else{ // if size > oldpBlockMetadata->size
        int requestedOrder = findTightOrder(size);
        if(canAllocateWithMerges(oldpBlockMetadata, requestedOrder)){
            MallocMetadata* mergedBlock = mergeAndStopAtOrder(oldpBlockMetadata, requestedOrder);
            if (mergedBlock == NULL){
                return NULL;
            }
            if(mergedBlock->is_free){
                num_free_bytes -= (int)(oldpBlockMetadata->size);
                num_free_block--;
                mergedBlock->is_free = false;
            }
            return (void*)((char*)mergedBlock + sizeof(MallocMetadata));
        }
        else{
            void* newAllocation = smalloc(size);
            if (newAllocation == NULL) {
                return NULL;
            }
            MallocMetadata* newAllocatedBlock = (MallocMetadata*)((char*)newAllocation - sizeof(MallocMetadata));
            memmove(newAllocatedBlock, oldp, oldpSize);
            sfree(oldp);
            return (void*)((char*)newAllocatedBlock + sizeof(MallocMetadata));
        }
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

/** END OF COPY OF MALLOC_2.CPP */



