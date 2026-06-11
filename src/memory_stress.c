/******************************************************************************
 * Memory Stress Test Module Implementation
 *****************************************************************************/

#include "memory_stress.h"
#include <stdlib.h>
#include <stdio.h>

void log_message(const char *format, ...);

/* ---- Memory test patterns ------------------------------------------------- */
const uint32_t TEST_PATTERNS[] = {
    0x5555AAAAu, 0xAAAA5555u, /* alternating nibbles */
    0xFF00FF00u, 0x00FF00FFu, /* alternating bytes   */
    0xDEADBEEFu, 0xCAFEBABEu, /* arbitrary           */
    0x00000000u, 0xFFFFFFFFu  /* all-zeros / all-ones */
};

/* ---- Global state --------------------------------------------------------- */
extern volatile uint32_t g_memory_errors;

/******************************************************************************
 * Memory Stress Test
 *
 * For each of the 8 test patterns:
 *   1. Write pattern to entire block
 *   2. Verify every word - increment g_memory_errors on mismatch
 *
 * Then:
 *   3. Walking-bit test (1 bit per byte, shifted by position)
 *   4. Prime-stride random access to stress DRAM row buffers / bank conflicts
 *
 * No disk I/O performed.
 *****************************************************************************/
void memory_stress_test(void)
{
    uint8_t *block = NULL;
    uint32_t size = MEMORY_BLOCK_SIZE;
    uint32_t i, p;

    block = (uint8_t *)malloc(size);
    while (block == NULL && size >= 1024u * 1024u)
    {
        size /= 2;
        block = (uint8_t *)malloc(size);
    }
    if (block == NULL)
    {
         log_message("Warning: memory allocation failed\n");
        return;
    }

    /* --- Pattern write/verify --------------------------------------------- */
    for (p = 0; p < NUM_PATTERNS; p++)
    {
        uint32_t pat = TEST_PATTERNS[p];

        for (i = 0; i + (uint32_t)sizeof(uint32_t) <= size; i += (uint32_t)sizeof(uint32_t))
            *(uint32_t *)(block + i) = pat;

        for (i = 0; i + (uint32_t)sizeof(uint32_t) <= size; i += (uint32_t)sizeof(uint32_t))
        {
            if (*(uint32_t *)(block + i) != pat)
            {
                g_memory_errors++;
                 log_message("MEM ERR offset +%u  expected %08X  got %08X\n",
                       i, pat, *(uint32_t *)(block + i));
            }
        }
    }

    /* --- Walking-bit test ------------------------------------------------- */
    for (i = 0; i < size; i++)
        block[i] = (uint8_t)(1u << (i % 8u));

    for (i = 0; i < size; i++)
    {
        if (block[i] != (uint8_t)(1u << (i % 8u)))
            g_memory_errors++;
    }

    /* --- Prime-stride traversal: thrash DRAM row buffers / bank conflicts -- */
    {
        const uint32_t PRIME_STEP = 65537u; /* prime > typical block size */
        uint32_t pos = 0;
        for (i = 0; i < 50000u; i++)
        {
            pos = (pos + PRIME_STEP) % size;
            block[pos] ^= (uint8_t)(i & 0xFFu);
        }
    }

    free(block);
}