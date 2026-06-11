/******************************************************************************
 * Memory Stress Test Module
 *
 * Implements memory testing with patterns, walking-bit, and random access
 *****************************************************************************/

#ifndef MEMORY_STRESS_H
#define MEMORY_STRESS_H

#include <stdint.h>

extern volatile uint32_t g_memory_errors;

/* Configuration constants */
#define MEMORY_BLOCK_SIZE (64 * 1024 * 1024) /* 64 MB test block */
#define NUM_PATTERNS 8

/* External global variable */
extern volatile uint32_t g_memory_errors;

/* Memory test patterns */
extern const uint32_t TEST_PATTERNS[];

/* Function prototype */
void memory_stress_test(void);

#endif /* MEMORY_STRESS_H */