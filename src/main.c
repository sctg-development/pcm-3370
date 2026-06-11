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

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* ---- Global state --------------------------------------------------------- */
volatile uint32_t g_memory_errors = 0;
DWORD g_start_tick = 0;
int   g_verbose    = 0;
int   g_log_enabled = 0;
static FILE *g_log_file = NULL;

/* ---- Module includes ------------------------------------------------------ */
#include "temperature.h"
#include "cpu_stress.h"
#include "memory_stress.h"
#include "cache_stress.h"
#include "display.h"
#include <stdarg.h>
#include <time.h>

/* ---- Function prototypes -------------------------------------------------- */
void infinite_loop(void);
static double get_cpu_usage(void);
void log_message(const char *format, ...);

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
 * Logging function - writes to both console and [timestamp].log if enabled
 *****************************************************************************/
void log_message(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    // Always print to console
    vprintf(format, args);

    // Also write to log file if logging is enabled
    if (g_log_enabled && g_log_file)
    {
        vfprintf(g_log_file, format, args);
        fflush(g_log_file);
    }

    va_end(args);
}

static BOOL WINAPI ctrl_handler(DWORD ctrl_type)
{
    (void)ctrl_type;
    return FALSE;
}

/******************************************************************************
 * Entry point
 *****************************************************************************/
int main(int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--verbose") == 0)
            g_verbose = 1;
        else if (strcmp(argv[i], "--log") == 0)
            g_log_enabled = 1;
    }

    g_start_tick = GetTickCount();
    SetConsoleCtrlHandler(ctrl_handler, TRUE);

    // Initialize logging if enabled
    if (g_log_enabled)
    {
        // Generate Unix timestamp for filename
        time_t now = time(NULL);
        char timestamp_str[64];
        snprintf(timestamp_str, sizeof(timestamp_str), "%ld", now);

        // Create filename with timestamp
        char log_filename[80];
        snprintf(log_filename, sizeof(log_filename), "[%s].log", timestamp_str);

        g_log_file = fopen(log_filename, "a");
        if (!g_log_file)
        {
            log_message("Warning: Could not open %s for writing\n", log_filename);
            g_log_enabled = 0;
        }
        else
        {
            // Write header to log file
            const char *time_str = ctime(&now);
            // Remove newline from ctime output
            char clean_time[26];
            strncpy(clean_time, time_str, sizeof(clean_time) - 1);
            clean_time[sizeof(clean_time) - 1] = '\0';
            // Remove trailing newline if present
            size_t len = strlen(clean_time);
            if (len > 0 && clean_time[len - 1] == '\n')
                clean_time[len - 1] = '\0';
            log_message("\n\n=== Log started: %s ===\n", clean_time);
            fflush(g_log_file);
        }
    }

    init_temperature();

    print_header();
    infinite_loop();



    if (g_log_file)
        fclose(g_log_file);

    return 0;
}
