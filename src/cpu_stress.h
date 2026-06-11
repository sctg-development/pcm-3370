/******************************************************************************
 * CPU Stress Test Module
 *
 * Implements CPU stress tests: FP operations, integer operations, and branches
 *****************************************************************************/

#ifndef CPU_STRESS_H
#define CPU_STRESS_H

#include <stdint.h>

/* Configuration constants */
#define CPU_FP_ITERS 500000u      /* FP ops per iteration */
#define CPU_INT_ITERS 1000000u    /* integer ops per iteration */
#define CPU_BRANCH_ITERS 500000u  /* branch stress iterations */

/* Function prototype */
void cpu_stress_test(void);

#endif /* CPU_STRESS_H */