/******************************************************************************
 * Cache Stress Test Module Implementation
 *****************************************************************************/

#include "cache_stress.h"
#include <stdlib.h>

/******************************************************************************
 * Cache Stress Test
 *
 * Three access patterns to defeat cache policies:
 *   1. Sequential forward  - baseline; warm in L2
 *   2. Sequential reverse  - defeats simple next-line hardware prefetcher
 *   3. Power-of-2 stride   - creates set-associativity conflicts in 4-way caches
 *****************************************************************************/
void cache_stress_test(void)
{
    const uint32_t cache_lines = 2048u;
    const uint32_t alloc_size = cache_lines * CACHE_LINE_SIZE + CACHE_LINE_SIZE;
    uint8_t *buf = (uint8_t *)malloc(alloc_size);
    uint8_t *arr;
    uint32_t i, j;
    volatile uint8_t sink = 0;

    if (buf == NULL)
        return;

    /* Align to cache-line boundary */
    arr = (uint8_t *)(((uintptr_t)buf + (uintptr_t)(CACHE_LINE_SIZE - 1)) & ~(uintptr_t)(CACHE_LINE_SIZE - 1));

    /* --- Forward stride --------------------------------------------------- */
    for (i = 0; i < 2000u; i++)
    {
        for (j = 0; j < cache_lines; j++)
            sink += arr[j * CACHE_LINE_SIZE];
    }

    /* --- Reverse stride --------------------------------------------------- */
    for (i = 0; i < 2000u; i++)
    {
        for (j = cache_lines; j-- > 0;)
            sink += arr[j * CACHE_LINE_SIZE];
    }

    /* --- Conflict pattern: stride = 128 lines (power of 2) ---------------- */
    for (i = 0; i < 2000u; i++)
    {
        for (j = 0; j < cache_lines; j += 16u)
            arr[j * CACHE_LINE_SIZE] = (uint8_t)(i + j);
    }

    (void)sink;
    free(buf);
}