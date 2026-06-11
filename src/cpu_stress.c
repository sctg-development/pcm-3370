/******************************************************************************
 * CPU Stress Test Module Implementation
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

#include "cpu_stress.h"
#include <math.h>

/******************************************************************************
 * CPU Stress Test
 *
 * Three sub-tests:
 *  1. Heavy FP  : sqrt / log / atan2 / fmod - hammers the x87 FPU
 *  2. Integer   : multiply + divide + rotate - stresses ALU and divider unit
 *  3. Branches  : data-dependent unpredictable branches - stresses BTB / RSB
 *****************************************************************************/
void cpu_stress_test(void)
{
    volatile double fp = 1.0;
    volatile uint32_t u = 0xDEADBEEFu;
    uint32_t i;

    /* --- Heavy FP: sqrt/log/atan2 are expensive on old x87 FPUs ----------- */
    for (i = 1; i <= CPU_FP_ITERS; i++)
    {
        fp += sqrt((double)i) * log((double)i + 1.0);
        fp -= atan2((double)(i & 0xFFFFu), (double)((i >> 8) | 1u));
        if ((i & 0x3FFu) == 0)
            fp = fmod(fp, 1.0e10); /* prevent FP overflow */
    }

    /* --- Integer: ALU heavy + integer divide (slow on Celeron/P3) ---------- */
    for (i = 1; i <= CPU_INT_ITERS; i++)
    {
        u ^= i * 2654435761u;      /* Knuth multiplicative hash */
        u = (u >> 13) | (u << 19); /* rotate32 */
        u += u / (i | 1u);         /* integer divide stresses divider unit */
    }

    /* --- Branch prediction stress: data-dependent, unpredictable ----------- */
    for (i = 0; i < CPU_BRANCH_ITERS; i++)
    {
        u ^= i * 1664525u + 1013904223u;
        if (u & 0x01u)
            fp += 1e-6;
        if (u & 0x02u)
            fp -= 1e-6;
        if (u & 0x04u)
            u ^= 0xABCD1234u;
        if (u & 0x08u)
            u += i;
        if (u & 0x10u)
            fp *= 1.0 + 1e-9;
        if (u & 0x20u)
            fp /= 1.0 + 1e-9;
        if (u & 0x40u)
            u -= i ^ 0xCAFEu;
        if (u & 0x80u)
            fp += sqrt((double)(u & 0xFFFFu) + 1.0);
    }

    (void)fp;
    (void)u;
}