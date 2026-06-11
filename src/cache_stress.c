/******************************************************************************
 * Cache Stress Test Module Implementation
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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