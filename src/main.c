/******************************************************************************
 * PCM-3370 Stress Test Program - Main Module
 *
 * Description: Comprehensive stress test for PCM-3370 single-board computer
 *              Targeting Windows XP 32-bit environment
 *              Designed for continuous operation and system validation
 *
 * Hardware: PCM-3370 with Intel Celeron 400MHz/650MHz or Pentium III
 *           VIA VT8606/TwiserT + VT82C686B chipset
 *           Up to 512MB SDRAM
 *
 * NOTE: No disk I/O - compact flash is write-protected during stress test
 *
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdint.h>

/* ---- Global state --------------------------------------------------------- */
volatile uint32_t g_memory_errors = 0;
DWORD g_start_tick = 0;

/* ---- Module includes ------------------------------------------------------ */
#include "temperature.h"
#include "cpu_stress.h"
#include "memory_stress.h"
#include "cache_stress.h"
#include "display.h"

/* ---- Function prototypes -------------------------------------------------- */
void infinite_loop(void);
static double get_cpu_usage(void);

/* ---- CPU snapshot for GetSystemTimes-based measurement -------------------- */
typedef struct {
    ULONGLONG idle;
    ULONGLONG total;
} CpuSnapshot;

/******************************************************************************
 * FILETIME -> ULONGLONG (100-ns intervals)
 *****************************************************************************/
static ULONGLONG ft_to_ull(const FILETIME *ft)
{
    return ((ULONGLONG)ft->dwHighDateTime << 32) | (ULONGLONG)ft->dwLowDateTime;
}

/******************************************************************************
 * CPU usage via GetSystemTimes (XP+)
 *****************************************************************************/
static double get_cpu_usage(void)
{
    static CpuSnapshot prev = {0, 0};
    FILETIME idle_ft, kernel_ft, user_ft;
    ULONGLONG idle, total, d_idle, d_total;
    double pct;

    if (!GetSystemTimes(&idle_ft, &kernel_ft, &user_ft))
        return 0.0;

    idle = ft_to_ull(&idle_ft);
    total = ft_to_ull(&kernel_ft) + ft_to_ull(&user_ft);

    if (prev.total == 0)
    {
        prev.idle = idle;
        prev.total = total;
        return 0.0;
    }

    d_idle = idle - prev.idle;
    d_total = total - prev.total;
    prev.idle = idle;
    prev.total = total;

    if (d_total == 0)
        return 0.0;

    pct = 100.0 * (1.0 - (double)d_idle / (double)d_total);
    if (pct < 0.0)
        pct = 0.0;
    if (pct > 100.0)
        pct = 100.0;
    return pct;
}

/******************************************************************************
 * Main infinite stress loop
 *****************************************************************************/
void infinite_loop(void)
{
    uint32_t iter = 0;

    /* Prime the CPU baseline snapshot before entering the loop */
    get_cpu_usage();
    Sleep(500);

    while (1)
    {
        cpu_stress_test();
        memory_stress_test();
        cache_stress_test();

        iter++;
        if (iter % STATUS_INTERVAL == 0)
        {
            /* Single CPU measurement per status update - fixes the 0.0% bug  */
            double cpu = get_cpu_usage();
            print_status(iter, cpu);
        }

        Sleep(50);
    }
}

/******************************************************************************
 * Entry point
 *****************************************************************************/
int main(void)
{
    g_start_tick = GetTickCount();

    /* Try WMI ACPI first; fall back to VIA 686B direct ISA access */
    init_temperature();
    if (!g_wmi_ready)
        init_via686b();

    print_header();
    infinite_loop();
    return 0;
}