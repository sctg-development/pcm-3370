/******************************************************************************
 * Cache Stress Test Module
 *
 * Implements cache stress tests with different access patterns
 *****************************************************************************/

#ifndef CACHE_STRESS_H
#define CACHE_STRESS_H

#include <stdint.h>

/* Configuration constants */
#define CACHE_LINE_SIZE 64  /* x86 cache line */

/* Function prototype */
void cache_stress_test(void);

#endif /* CACHE_STRESS_H */