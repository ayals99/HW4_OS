#include <unistd.h>

#define MAX_SMALLOC_SIZE 100000000 // 10^8
#define ZERO_SMALLOC 0
/**
void* smalloc(size_t size)
    - Tries to allocate ‘size’ bytes.
    @return:
        Success:
            a pointer to the first allocated byte within the allocated block.
        Failure:
            a. If ‘size’ is 0 returns NULL.
            b. If ‘size’ is more than 10^8, return NULL.
            c. If sbrk fails, return NULL.
 */
void* smalloc(size_t size){
    if (size == ZERO_SMALLOC || size > MAX_SMALLOC_SIZE) {
        return NULL;
    }

    // TODO: check if need to cast to (intptr_t), which is "long int"
    long int sizeCast = (long int)size;

    void* newBlock = sbrk(sizeCast);

    if (newBlock == (void*)(-1)) { // sbrk fails
        return NULL;
    }

    // sbrk returns the old program break,
    // which is now the start of the newly allocated memory
    return newBlock;
}