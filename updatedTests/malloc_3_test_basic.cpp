#include "my_stdlib.h"
#include <catch2/catch_test_macros.hpp>

#include <unistd.h>
#include <cmath>
#include <sys/wait.h>
#include <unistd.h>
#include <iostream>

void performCorruption()
{
    // Allocate memory
    void *ptr1 = smalloc(16); // Allocate 16 bytes
    REQUIRE(ptr1 != nullptr);
    void *ptr2 = smalloc(32); // Allocate 32 bytes
    REQUIRE(ptr2 != nullptr);

    // Overflow the first allocation
    char *overflowPtr = reinterpret_cast<char *>(ptr1);
    for (int i = 0; i < 2000; i++)
    {
        overflowPtr[i] = 'A';
    }

    // Allocate more memory
    void *ptr3 = smalloc(64); // Allocate 64 bytes
    REQUIRE(ptr3 != nullptr);

    // Free the allocations
    sfree(ptr1);
    sfree(ptr2);
    sfree(ptr3);
}
#define MAX_ALLOCATION_SIZE (1e8)
#define MMAP_THRESHOLD (128 * 1024)
#define MIN_SPLIT_SIZE (128)
#define MAX_ELEMENT_SIZE (128 * 1024)
// static inline size_t aligned_size(size_t size)
//{
//     return (size % 8) ? (size & (size_t)(-8)) + 8 : size;
// }

#define verify_blocks(allocated_blocks, allocated_bytes, free_blocks, free_bytes)  \
    do                                                                             \
    {                                                                              \
        REQUIRE(_num_allocated_blocks() == allocated_blocks);                      \
        REQUIRE(_num_allocated_bytes() == (allocated_bytes));                      \
        REQUIRE(_num_free_blocks() == free_blocks);                                \
        REQUIRE(_num_free_bytes() == (free_bytes));                                \
        REQUIRE(_num_meta_data_bytes() == (_size_meta_data() * allocated_blocks)); \
    } while (0)

#define verify_size(base)                                                                             \
    do                                                                                                \
    {                                                                                                 \
        void *after = sbrk(0);                                                                        \
        REQUIRE(_num_allocated_bytes() + aligned_size(_size_meta_data() * _num_allocated_blocks()) == \
                (size_t)after - (size_t)base);                                                        \
    } while (0)

#define verify_size_with_large_blocks(base, diff)      \
    do                                                 \
    {                                                  \
        void *after = sbrk(0);                         \
        REQUIRE(diff == (size_t)after - (size_t)base); \
    } while (0)
void verify_block_by_order(int order0free, int order0used, int order1free, int order1used,
                           int order2free, int order2used,
                           int order3free, int order3used,
                           int order4free, int order4used,
                           int order5free, int order5used,
                           int order6free, int order6used,
                           int order7free, int order7used,
                           int order8free, int order8used,
                           int order9free, int order9used,
                           int order10free, int order10used,
                           int big_blocks_count, long big_blocks_size)

{
    unsigned int __total_blocks = order0free + order0used + order1free + order1used + order2free + order2used + order3free + order3used + order4free + order4used + order5free + order5used + order6free + order6used + order7free + order7used + order8free + order8used + order9free + order9used + order10free + order10used + big_blocks_count;
    unsigned int __total_free_blocks = order0free + order1free + order2free + order3free + order4free + order5free + order6free + order7free + order8free + order9free + order10free;
    unsigned int __total_free_bytes_with_meta = order0free * 128 * pow(2, 0) + order1free * 128 * pow(2, 1) + order2free * 128 * pow(2, 2) + order3free * 128 * pow(2, 3) + order4free * 128 * pow(2, 4) + order5free * 128 * pow(2, 5) + order6free * 128 * pow(2, 6) + order7free * 128 * pow(2, 7) + order8free * 128 * pow(2, 8) + order9free * 128 * pow(2, 9) + order10free * 128 * pow(2, 10);
    unsigned int testing_allocated_bytes;
    if (__total_blocks == 0)
        testing_allocated_bytes = 0;
    else
        testing_allocated_bytes = big_blocks_size + 32 * MAX_ELEMENT_SIZE - (__total_blocks - big_blocks_count) * (_size_meta_data());
    verify_blocks(__total_blocks, testing_allocated_bytes, __total_free_blocks, __total_free_bytes_with_meta - __total_free_blocks * (_size_meta_data()));
}

// TEST_CASE("alignment test")
//{
//     // Initial state
//     void *base = sbrk(0);
//     REQUIRE(((size_t)base)%MAX_ELEMENT_SIZE ==0);
//
// }

TEST_CASE("Challenge 0 - Memory Utilization", "[malloc3]")
{
    // Initial state
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    // Allocate small block (order 0)
    void *ptr1 = smalloc(40);
    REQUIRE(ptr1 != nullptr);
    //    verify_size(base);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Allocate large block (order 10)
    void *ptr2 = smalloc(MAX_ELEMENT_SIZE + 100);
    REQUIRE(ptr2 != nullptr);
    //    verify_size_with_large_blocks(base, (128 * 1024+100 +_size_meta_data()));
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 1, MAX_ELEMENT_SIZE + 100);

    // Allocate another small block
    void *ptr3 = smalloc(50);
    REQUIRE(ptr3 != nullptr);
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 1, MAX_ELEMENT_SIZE + 100);

    // Free the first small block
    sfree(ptr1);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 1, MAX_ELEMENT_SIZE + 100);

    // Allocate another small block
    void *ptr4 = smalloc(40);
    REQUIRE(ptr4 != nullptr);
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 1, MAX_ELEMENT_SIZE + 100);

    // Free all blocks
    sfree(ptr3);
    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 1, MAX_ELEMENT_SIZE + 100);
    sfree(ptr1); // free again
    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    //    verify_size_with_large_blocks(base, 0);
    //    verify_size(base);
}

TEST_CASE("test all sizes", "[malloc3]")
{
    // Initial state
    //    verify_block_by_order(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,0,0,0);
    void *ptr;
    ptr = smalloc(128 * pow(2, 0) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 1) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 2) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 3) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 4) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 5) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 6) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 7) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 8) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 9) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 31, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    ptr = smalloc(128 * pow(2, 10) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 31, 1, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("Finding buddies test", "[malloc3]")
{
    std::vector<void *> allocations;

    // Allocate 64 blocks of size 128 * 2^9 - 64
    for (int i = 0; i < 64; i++)
    {
        printf("%d\n", i);
        fflush(stdout);
        void *ptr = smalloc(128 * std::pow(2, 9) - 64);
        REQUIRE(ptr != nullptr);
        allocations.push_back(ptr);
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, allocations.size() % 2, allocations.size(), 32 - (int)(i / 2) - 1, 0, 0, 0);
        printf("OK\n");
        fflush(stdout);
    }

    REQUIRE(smalloc(40) == NULL);
    // Free the allocated blocks
    while (!allocations.empty())
    {
        void *ptr = allocations.back();
        allocations.pop_back();
        sfree(ptr);
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, allocations.size() % 2, allocations.size(), 32 - (int)(allocations.size() / 2) - (allocations.size() % 2), 0, 0, 0);
    }

    // Verify that all blocks are merged into a single large block
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);

    for (int i = 0; i < 64; i++)
    {
        void *ptr = smalloc(128 * std::pow(2, 9) - 64);
        REQUIRE(ptr != nullptr);
        allocations.push_back(ptr);
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, allocations.size() % 2, allocations.size(), 32 - (int)(i / 2) - 1, 0, 0, 0);
    }
    REQUIRE(smalloc(40) == NULL);
    // Free the allocated blocks
    while (!allocations.empty())
    {
        void *ptr = allocations.front();
        allocations.erase(allocations.begin());
        sfree(ptr);
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, allocations.size() % 2, allocations.size(), 32 - (int)(allocations.size() / 2) - (allocations.size() % 2), 0, 0, 0);
    }
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("multiple big allocs test", "[malloc3]")
{
    // Allocate large block (order 10)
    std::vector<void *> allocations;
    int i = 1;
    for (; i < 64; i++)
    {
        void *ptr = smalloc(MAX_ELEMENT_SIZE + 100);
        REQUIRE(ptr != nullptr);
        allocations.push_back(ptr);
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, i, i * (MAX_ELEMENT_SIZE + 100));
    }
    i--;

    while (!allocations.empty())
    {
        void *ptr = allocations.back();
        allocations.pop_back();
        sfree(ptr);
        i--;
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, i, i * (MAX_ELEMENT_SIZE + 100));
    }

    // Verify that all blocks are merged into a single large block
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);

    for (i = 1; i < 64; i++)
    {
        void *ptr = smalloc(MAX_ELEMENT_SIZE + 100);
        REQUIRE(ptr != nullptr);
        allocations.push_back(ptr);
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, i, i * (MAX_ELEMENT_SIZE + 100));
    }
    i--;
    while (!allocations.empty())
    {
        void *ptr = allocations.front();
        allocations.erase(allocations.begin());
        sfree(ptr);
        i--;
        verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, i, i * (MAX_ELEMENT_SIZE + 100));
    }
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("srealloc test", "[malloc3]")
{
    // Initial state
    //    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);

    // Allocate a small block
    void *ptr1 = smalloc(40);
    REQUIRE(ptr1 != nullptr);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Reallocate to a larger size
    void *ptr2 = srealloc(ptr1, 60);
    REQUIRE(ptr2 != nullptr);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Reallocate to a smaller size
    void *ptr3 = srealloc(ptr2, 30);
    REQUIRE(ptr3 != nullptr);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Free the block
    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("scalloc test", "[malloc3]")
{
    // Allocate and initialize memory with zeros
    void *ptr = scalloc(5, sizeof(int));
    REQUIRE(ptr != nullptr);

    // Verify that the memory is zero-initialized
    int *arr = static_cast<int *>(ptr);
    for (int i = 0; i < 5; i++)
    {
        REQUIRE(arr[i] == 0);
    }

    // Modify the memory
    for (int i = 0; i < 5; i++)
    {
        arr[i] = i + 1;
    }

    // Reallocate the memory with a larger size
    void *newPtr = scalloc(10, sizeof(int));
    REQUIRE(newPtr != nullptr);

    // Verify that the original values are preserved in the first 5 elements
    int *newArr = static_cast<int *>(newPtr);

    // Verify that the additional elements are zero-initialized
    for (int i = 0; i < 10; i++)
    {
        REQUIRE(newArr[i] == 0);
        newArr[i] = i + 1;
    }

    for (int i = 0; i < 10; i++)
    {
        REQUIRE(newArr[i] == i + 1);
    }

    // Free the memory
    sfree(ptr);
    sfree(newPtr);
}

TEST_CASE("srealloc merges test", "[malloc3]")
{
    // Initial state
    //    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);

    // Allocate a small block
    void *ptr1 = smalloc(40);
    REQUIRE(ptr1 != nullptr);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Reallocate to a larger size
    void *ptr2 = srealloc(ptr1, 128 * pow(2, 2) - 64);
    REQUIRE(ptr2 != nullptr);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    int *newArr = static_cast<int *>(ptr2);

    // Verify  elements are copied
    for (int i = 0; i < 10; i++)
    {
        newArr[i] = i + 1;
    }

    // Reallocate to a larger size
    void *ptr3 = srealloc(ptr2, 100);
    REQUIRE(ptr3 != nullptr);
    REQUIRE(ptr2 == ptr3);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    void *ptr4 = srealloc(ptr3, 128 * pow(2, 8) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 31, 0, 0, 0);
    int *newArr2 = static_cast<int *>(ptr4);
    for (int i = 0; i < 10; i++)
    {
        REQUIRE(newArr2[i] == i + 1);
    }
    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("weird values", "[malloc3]")
{
    // Initial state
    //    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    void *ptr = smalloc(100000001);
    REQUIRE(ptr == NULL);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
    sfree(ptr);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! size of blocks and meta data", "[malloc3]")
{
    void *ptr1 = smalloc(1);
    void *ptr2 = smalloc(1);
    unsigned long start = (unsigned long)ptr1;
    unsigned long end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128);
    sfree(ptr1);
    sfree(ptr2);

    int size_meta_data = _size_meta_data();

    ptr1 = smalloc(128 * pow(2, 1) - 64);
    ptr2 = smalloc(128 * pow(2, 1) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 1));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 1) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 1) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 1));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 2) - 64);
    ptr2 = smalloc(128 * pow(2, 2) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 2));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 2) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 2) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 2));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 3) - 64);
    ptr2 = smalloc(128 * pow(2, 3) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 3));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 3) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 3) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 3));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 4) - 64);
    ptr2 = smalloc(128 * pow(2, 4) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 4));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 4) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 4) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 4));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 5) - 64);
    ptr2 = smalloc(128 * pow(2, 5) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 5));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 5) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 5) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 5));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 6) - 64);
    ptr2 = smalloc(128 * pow(2, 6) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 6));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 6) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 6) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 6));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 7) - 64);
    ptr2 = smalloc(128 * pow(2, 7) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 7));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 7) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 7) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 7));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 8) - 64);
    ptr2 = smalloc(128 * pow(2, 8) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 8));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 8) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 8) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 8));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 9) - 64);
    ptr2 = smalloc(128 * pow(2, 9) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 9));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 9) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 9) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 9));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 10) - 64);
    ptr2 = smalloc(128 * pow(2, 10) - 64);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 10));
    sfree(ptr1);
    sfree(ptr2);

    ptr1 = smalloc(128 * pow(2, 10) - size_meta_data);
    ptr2 = smalloc(128 * pow(2, 10) - size_meta_data);
    start = (unsigned long)ptr1;
    end = (unsigned long)ptr2;
    REQUIRE((end - start) == 128 * pow(2, 10));
    sfree(ptr1);
    sfree(ptr2);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! GEPETA 1", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;

    // Test 1: Allocate a small block
    std::cout << "Test GEPETA 1: 1" << std::endl;
    ptr1 = smalloc(128 * pow(2, 0) - 64); // Request smaller than base block size
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Test 2: Allocate and initialize a block using scalloc
    std::cout << "Test GEPETA 1: 2" << std::endl;
    ptr2 = scalloc(1, 128 * pow(2, 1) - 64); // One block of order 1 size, initialized to 0
    verify_block_by_order(1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Test 3: Reallocate the first block to a larger size
    std::cout << "Test GEPETA 1: 3" << std::endl;
    ptr1 = srealloc(ptr1, 128 * pow(2, 3) - 64); // Resize to order 3 size
    verify_block_by_order(0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Test 4: Free the second block
    std::cout << "Test GEPETA 1: 4" << std::endl;
    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Test 5: Allocate a large block to test upper limit
    std::cout << "Test GEPETA 1: 5" << std::endl;
    ptr3 = smalloc(128 * pow(2, 10) - 64); // Nearly the largest block size
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 30, 1, 0, 0);

    // Test 6: Allocate a small block
    std::cout << "Test GEPETA 1: 6" << std::endl;
    ptr4 = smalloc(128 * pow(2, 0) - 64); // Request smaller than base block size
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 30, 1, 0, 0);

    // Test 7: Allocate a large block
    std::cout << "Test GEPETA 1: 7" << std::endl;
    ptr5 = smalloc(128 * pow(2, 10) - 64); // Nearly the largest block size
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 8: Allocate a small block
    std::cout << "Test GEPETA 1: 8" << std::endl;
    ptr6 = smalloc(128 * pow(2, 0) - 64); // Request smaller than base block size
    verify_block_by_order(0, 2, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 9: Allocate a small block
    std::cout << "Test GEPETA 1: 9" << std::endl;
    ptr7 = smalloc(128 * pow(2, 0) - 64); // Request smaller than base block size
    verify_block_by_order(1, 3, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 10: Reallocate the third large block to a smaller size
    std::cout << "Test GEPETA 1: 10" << std::endl;
    ptr3 = srealloc(ptr3, 128 * pow(2, 5) - 64); // Reduce size to order 5
    verify_block_by_order(1, 3, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 11: Reallocate the fifth large block to a smaller size as well
    std::cout << "Test GEPETA 1: 11" << std::endl;
    ptr5 = srealloc(ptr5, 128 * pow(2, 5) - 64); // Reduce size to order 5
    verify_block_by_order(1, 3, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 12: Free a small block to test merging of buddies
    std::cout << "Test GEPETA 1: 12" << std::endl;
    sfree(ptr6);
    verify_block_by_order(2, 2, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 13: Reallocate one of the small blocks to a larger size to force splitting
    std::cout << "Test GEPETA 1: 13" << std::endl;
    ptr7 = srealloc(ptr7, 128 * pow(2, 7) - 64); // Increase size to order 7
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 14: Free and reallocate a small block to test quick recycling of memory
    std::cout << "Test GEPETA 1: 14" << std::endl;
    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 29, 2, 0, 0);
    ptr4 = smalloc(128 * pow(2, 0) - 64); // Request smaller than base block size again
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 29, 2, 0, 0);

    // Test 15: Free a large block and then allocate a block of the same size to test reuse
    std::cout << "Test GEPETA 1: 15" << std::endl;
    sfree(ptr5);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 30, 1, 0, 0);
    ptr8 = smalloc(128 * pow(2, 5) - 64); // Allocate a block of order 5 size
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 30, 1, 0, 0);

    // Cleanup remaining allocations
    sfree(ptr1);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 30, 1, 0, 0);
    sfree(ptr3);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr7);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr8);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr1);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr3);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr4); //ADDED
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 1.5", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3;
    ptr1 = smalloc(128 * pow(2, 0) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Allocate a medium block
    ptr2 = smalloc(128 * pow(2, 5) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Allocate a large block
    ptr3 = smalloc(128 * pow(2, 9) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 31, 0, 0, 0);

    // Free the medium block
    sfree(ptr2);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 31, 0, 0, 0);

    // Free the small block
    sfree(ptr1);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 31, 0, 0, 0);

    // Free the large block
    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 2", "[malloc3]")
{

    void *ptr1, *ptr2;
    ptr1 = smalloc(128 * pow(2, 1) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 4) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr1);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    ptr1 = smalloc(128 * pow(2, 6) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr1);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 3", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3;
    ptr1 = smalloc(128 * pow(2, 2) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = scalloc(1, 128 * pow(2, 3) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    ptr3 = smalloc(128 * pow(2, 7) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 31, 0, 0, 0);

    ptr2 = srealloc(ptr1, 128 * pow(2, 5) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 8", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3;
    ptr1 = smalloc(128 * pow(2, 1) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    ptr2 = smalloc(128 * pow(2, 3) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    ptr3 = srealloc(ptr1, 128 * pow(2, 2) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    ptr2 = smalloc(128 * pow(2, 4) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 9", "[malloc3]")
{

    void *ptr1, *ptr2, *ptr3, *ptr4;

    // Allocate two blocks of size 1
    ptr1 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 3, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr4 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 4, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(1, 3, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr3);
    verify_block_by_order(2, 2, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr1 = srealloc(ptr1, 128 * pow(2, 1) - 64); // Resize to size 2
    verify_block_by_order(1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr1);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr1 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(1, 3, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr1 = srealloc(ptr1, 128 * pow(2, 1) - 64); // Resize to size 2
    verify_block_by_order(2, 2, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Allocate one block of size 2
    ptr3 = smalloc(128 * pow(2, 1) - 64); // Size 2 block
    verify_block_by_order(2, 2, 0, 2, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Free the two blocks of size 1
    sfree(ptr1);
    verify_block_by_order(2, 2, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr2);
    verify_block_by_order(1, 1, 2, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr3);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 12", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3, *ptr4;

    // Allocate and free in a pattern that tests edge cases
    ptr1 = smalloc(128 * pow(2, 2) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 4) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = smalloc(128 * pow(2, 8) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 31, 0, 0, 0);

    // Intentional allocation failure due to size constraints
    ptr4 = smalloc(128 * pow(2, 10) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 30, 1, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 30, 1, 0, 0);
    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 30, 1, 0, 0);

    // Reallocate with a size that should fail
    ptr1 = srealloc(ptr1, 128 * pow(2, 9) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 30, 1, 0, 0);

    sfree(ptr1);
    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 13", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3;

    ptr1 = smalloc(128 * pow(2, 0) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 3) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Repeatedly reallocate to stress the memory management system
    ptr3 = srealloc(ptr1, 128 * pow(2, 6) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = srealloc(ptr3, 128 * pow(2, 1) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = srealloc(ptr3, 128 * pow(2, 7) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 14", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3, *ptr4;

    ptr1 = smalloc(128 * pow(2, 1) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 4) - 64);
    verify_block_by_order(0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Stress test with multiple frees and allocations
    sfree(ptr1);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = smalloc(128 * pow(2, 5) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr4 = smalloc(128 * pow(2, 6) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr3);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("THANK YOU AVIGAIL YOU ARE AMAZING! 15", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6;

    // Initial allocations
    ptr1 = smalloc(128 * pow(2, 0) - 64);
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 2) - 64);
    verify_block_by_order(1, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = smalloc(128 * pow(2, 4) - 64);
    verify_block_by_order(1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Complex reallocation patterns
    ptr1 = srealloc(ptr1, 128 * pow(2, 3) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr4 = smalloc(128 * pow(2, 5) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = srealloc(ptr2, 128 * pow(2, 1) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    // Continue with more reallocations and allocations
    ptr5 = smalloc(128 * pow(2, 6) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = srealloc(ptr3, 128 * pow(2, 7) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr6 = smalloc(128 * pow(2, 8) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 31, 0, 0, 0);

    // Reallocate several times in a row to test memory stability
    ptr4 = srealloc(ptr4, 128 * pow(2, 9) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 31, 0, 0, 0);
    ptr5 = srealloc(ptr5, 128 * pow(2, 2) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 31, 0, 0, 0);
    ptr6 = srealloc(ptr6, 128 * pow(2, 0) - 64);
    verify_block_by_order(0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 31, 0, 0, 0);

    // Free some blocks to test deallocation logic
    sfree(ptr2);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1, 31, 0, 0, 0);
    sfree(ptr4);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 31, 0, 0, 0);

    // Final reallocations and cleanup
    ptr1 = srealloc(ptr1, 128 * pow(2, 4) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 31, 0, 0, 0);
    ptr3 = srealloc(ptr3, 128 * pow(2, 5) - 64);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 1, 0, 31, 0, 0, 0);

    sfree(ptr1);
    sfree(ptr3);
    sfree(ptr5);
    sfree(ptr6);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}

TEST_CASE("really free?", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7, *ptr8;

    // Allocate eight blocks of size 1
    ptr1 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 3, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr4 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 4, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr5 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 5, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr6 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 6, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr7 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 7, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr8 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 8, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);


    sfree(ptr1);
    verify_block_by_order(1, 7, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr3);
    verify_block_by_order(2, 6, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr2);
    verify_block_by_order(1, 5, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr5);
    verify_block_by_order(2, 4, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr6);
    verify_block_by_order(1, 3, 2, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr7);
    verify_block_by_order(2, 2, 2, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr4); //ptr8 is still used so can't fully merge
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    sfree(ptr8);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);

}


TEST_CASE("tightest merge", "[malloc3]")
{
    void *ptr1, *ptr2, *ptr3, *ptr4, *ptr5;

    // Allocate eight blocks of size 1
    ptr1 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr2 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr3 = smalloc(128 * pow(2, 0) - 64); // Size 1 block
    verify_block_by_order(1, 3, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);
    ptr4 = smalloc(128 * pow(2, 0) - 64); // Another Size 1 block
    verify_block_by_order(0, 4, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr1);
    sfree(ptr2);
    verify_block_by_order(0, 2, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 31, 0, 0, 0);

    sfree(ptr3);
    ptr4 = srealloc(ptr4, 128 * pow(2, 1) - 64);
    REQUIRE(ptr4 == ptr3);

    ptr5 = smalloc(128 * pow(2, 0) - 64); 
    REQUIRE(ptr5 == ptr1);

    sfree(ptr4);
    sfree(ptr5);
    verify_block_by_order(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0);
}