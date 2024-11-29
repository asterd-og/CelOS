/*

Memory allocator written by: Astrido
I wrote this for the EchoCore LibC, which follows unix-like style
but now I had to port it for CelOS which has NT-like style.
Yes. I know the style is fucked.

*/

#include <alloc.h>
#include <vmm.h>
#include <pmm.h>
#include <stdbool.h>
#include <string.h>
#include <printf.h>

#define FREE_BIT 1
#define BLK_GET_SIZE(blk) (blk->size >> 1)

#define PAGE_SIZE 4096

typedef struct block {
    uint8_t *region_addr; // Address of the region it's located in
    struct block *next;
    struct block *prev;
    size_t size;
} block_t;

typedef struct region {
    struct region *next;
    size_t free_size;
    size_t total_size;
    uint8_t *data_area;
} region_t;

region_t *main_region = NULL;

region_t *mm_create_region(size_t area_size) {
    region_t *region = MmVirtAllocatePages(g_pKernelPageMap, 1, MM_READ | MM_WRITE);
    region->free_size = area_size - sizeof(block_t);
    region->total_size = area_size;
    region->data_area = MmVirtAllocatePages(g_pKernelPageMap, DIV_ROUND_UP(area_size, PAGE_SIZE), MM_READ | MM_WRITE);
    region->next = NULL;
    block_t *blk = (block_t*)region->data_area;
    blk->region_addr = (uint8_t*)region;
    blk->next = NULL;
    blk->prev = NULL;
    blk->size = ((area_size - sizeof(block_t)) << 1) | FREE_BIT;
    return region;
}

void MmAllocInit() {
    size_t area_size = PAGE_SIZE * 3;
    main_region = mm_create_region(area_size);
}

block_t *mm_split(block_t *blk, size_t size) {
    block_t *new_blk = (block_t*)((uint8_t*)blk + sizeof(block_t) + size);
    size_t old_size = BLK_GET_SIZE(blk);
    new_blk->next = blk->next;
    new_blk->prev = blk;
    new_blk->size = (old_size - size - sizeof(block_t)) << 1;
    blk->next = new_blk;
    blk->size = size << 1;
    return blk;
}

void *MmAlloc(size_t size) {
    if (size % 0x10) {
        size -= (size % 0x10);
        size += 0x10;
    }
    // Search a region with enough free size
    region_t *region;
    bool found = false;
    for (region = main_region; region != NULL; region = region->next) {
        if (region->free_size >= size + sizeof(block_t)) {
            found = true;
            break;
        }
        if (region->next == NULL)
            break;
    }

    if (!found) {
        region_t *new_region = mm_create_region(size * 2);
        region->next = new_region;
        region = new_region;
    }

    block_t *blk = (block_t*)region->data_area;
    while (blk && !(blk->size & FREE_BIT)) {
        if (BLK_GET_SIZE(blk) >= size && (blk->size & FREE_BIT))
            break;
        blk = blk->next; // We don't do any additional checks here
        // because we're sure we found a region big enough.
    }

    if (BLK_GET_SIZE(blk) > size) {
        blk = mm_split(blk, size);
        blk->next->size |= FREE_BIT;
        region->free_size -= sizeof(block_t);
    }
    region->free_size -= size;
    blk->region_addr = (uint8_t*)region;
    blk->size &= ~FREE_BIT;

    return (void*)((uint8_t*)blk + sizeof(block_t));
}

// Merge block with the next one.
block_t* mm_merge(block_t *blk, region_t *region) {
    size_t new_size = (BLK_GET_SIZE(blk) + BLK_GET_SIZE(blk->next) + sizeof(block_t));
    blk->size = new_size << 1;
    blk->next = blk->next->next;
    if (blk->next) blk->next->prev = blk;
    memset((uint8_t*)blk + sizeof(block_t), 0, new_size);
    region->free_size += BLK_GET_SIZE(blk) + sizeof(block_t);
    return blk;
}

void MmFree(void *ptr) {
    block_t *blk = (block_t*)((uint8_t*)ptr - sizeof(block_t));
    region_t *region = (region_t*)blk->region_addr;

    // Find neighboring free blocks and merge them if we find them
    while (blk->next && (blk->next->size & FREE_BIT) == 1)
        mm_merge(blk, region);
    while (blk->prev && (blk->prev->size & FREE_BIT) == 1)
        blk = mm_merge(blk->prev, region);
    blk->size |= FREE_BIT;
}

void MmAllocPrint() {
    int i = 0;
    for (region_t *region = main_region; region != NULL; region = region->next) {
        printf("Region %d:\n", i++);
        block_t *blk = (block_t*)region->data_area;
        size_t region_size = 0;
        while (blk != NULL) {
            printf(" block info:\n  - block start: 0x%lx\n  - block size: 0x%lx (sizeof block: 0x%lx, both combined: 0x%lx)\n  - block free: %d\n",
                (uint64_t)blk, BLK_GET_SIZE(blk), sizeof(block_t), (BLK_GET_SIZE(blk)) + sizeof(block_t), blk->size & FREE_BIT);
            region_size += BLK_GET_SIZE(blk) + sizeof(block_t);
            blk = blk->next;
        }
        printf(" Region size: %ld | 0x%lx\n Region free size: %ld | 0x%lx\n", region_size, region_size, region->free_size, region->free_size);
    }
}

void MmAllocDestroy() {
    region_t *next_region = main_region->next;
    for (region_t *region = main_region; region != NULL; region = next_region) {
        next_region = region->next;
        munmap(region->data_area, region->total_size);
        munmap(region, sizeof(region_t));
    }
}