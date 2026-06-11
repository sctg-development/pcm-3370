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
#include <time.h>

/* ---- Global state --------------------------------------------------------- */
volatile uint32_t g_memory_errors = 0;
DWORD g_start_tick = 0;
int   g_verbose    = 0;
int   g_log_enabled = 0;
int   g_report_enabled = 0;
FILE *g_log_file = NULL;
FILE *g_json_file = NULL;
FILE *g_html_file = NULL;
time_t g_timestamp = 0;
char g_timestamp_str[64] = {0};
char g_json_filename[80] = {0};
char g_html_filename[80] = {0};

/* ---- Module includes ------------------------------------------------------ */
#include "temperature.h"
#include "cpu_stress.h"
#include "memory_stress.h"
#include "cache_stress.h"
#include "display.h"
#include "html_report.h"
#include <stdarg.h>
#include <time.h>

/* ---- Function prototypes -------------------------------------------------- */
void infinite_loop(void);
static double get_cpu_usage(void);
void log_message(const char *format, ...);
static void print_help(void);
void write_json_status(uint32_t iterations, double cpu_usage);

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
            write_json_status(iter, cpu);
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

/******************************************************************************
 * Write JSON status data to report file
 *****************************************************************************/
void write_json_status(uint32_t iterations, double cpu_usage)
{
    if (!g_report_enabled || !g_json_file)
        return;

    MEMORYSTATUS mem;
    DWORD elapsed_s = (GetTickCount() - g_start_tick) / 1000u;
    uint32_t h = elapsed_s / 3600u;
    uint32_t m = (elapsed_s % 3600u) / 60u;
    uint32_t s = elapsed_s % 60u;
    double iter_per_min = (elapsed_s > 0)
                              ? ((double)iterations / (double)elapsed_s * 60.0)
                              : 0.0;
    double temp_c = get_cpu_temperature();
    double temp_mb = get_mb_temperature();

    GlobalMemoryStatus(&mem);

    // Format elapsed time as HH:MM:SS
    char elapsed_time[16];
    snprintf(elapsed_time, sizeof(elapsed_time), "%02u:%02u:%02u", h, m, s);

    // Write JSON data (one complete JSON object per line - NDJSON format)
    fprintf(g_json_file, "{\"elapsed_time\":\"%s\",\"iteration\":%u,\"cpu_usage\":%.1f,\"cpu_temp\":%.1f,\"mb_temp\":%.1f,\"free_memory_mb\":%u,\"memory_errors\":%u,\"rate_iter_per_min\":%.1f}\n",
            elapsed_time, iterations, cpu_usage,
            temp_c >= 0.0 ? temp_c : -1.0,
            temp_mb >= 0.0 ? temp_mb : -1.0,
            (unsigned)(mem.dwAvailPhys >> 20),
            g_memory_errors,
            iter_per_min);
    fflush(g_json_file);

    // Mirror the same data into the self-contained HTML report
    update_html_report(elapsed_time, iterations, cpu_usage,
                       temp_c >= 0.0 ? temp_c : -1.0,
                       temp_mb >= 0.0 ? temp_mb : -1.0,
                       (unsigned)(mem.dwAvailPhys >> 20),
                       g_memory_errors, iter_per_min);
}

static BOOL WINAPI ctrl_handler(DWORD ctrl_type)
{
    (void)ctrl_type;
    return FALSE;
}

/******************************************************************************
 * Print help information
 *****************************************************************************/
static void print_help(void)
{
    printf("PCM-3370 Stress Test Program - Usage\n");
    printf("\n");
    printf("Usage: pcm3370_stress [options]\n");
    printf("\n");
    printf("Options:\n");
    printf("  --verbose    Enable verbose output\n");
    printf("  --log        Enable logging to file\n");
    printf("  --report     Enable report generation (JSON + HTML)\n");
    printf("  -h, --help   Show this help message\n");
    printf("\n");
    printf("Description:\n");
    printf("  Comprehensive stress test for PCM-3370 single-board computer\n");
    printf("  Tests CPU, memory, and cache subsystems continuously\n");
    printf("  Designed for Windows XP 32-bit environment\n");
    printf("\n");
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
        else if (strcmp(argv[i], "--report") == 0)
            g_report_enabled = 1;
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            print_help();
            return 0;
        }
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
        snprintf(log_filename, sizeof(log_filename), "%s.log", timestamp_str);

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

    // Initialize report if enabled
    if (g_report_enabled)
    {
        // Generate Unix timestamp for filename
        g_timestamp = time(NULL);
        snprintf(g_timestamp_str, sizeof(g_timestamp_str), "%ld", g_timestamp);

        // Create filenames with timestamp
        snprintf(g_json_filename, sizeof(g_json_filename), "%s.ndjson", g_timestamp_str);
        snprintf(g_html_filename, sizeof(g_html_filename), "%s.html", g_timestamp_str);

        // Open JSON file for writing
        g_json_file = fopen(g_json_filename, "w");
        if (!g_json_file)
        {
            log_message("Warning: Could not open %s for writing\n", g_json_filename);
            g_report_enabled = 0;
        }
        else
        {
            // NDJSON format - no array brackets, just newline-delimited JSON objects
            // File starts empty, we'll append complete JSON objects line by line
            fflush(g_json_file);
        }

        // Open HTML file for writing
        g_html_file = fopen(g_html_filename, "w");
        if (!g_html_file)
        {
            log_message("Warning: Could not open %s for writing\n", g_html_filename);
            // Don't disable report entirely, just HTML part
        }
        else
        {
            // Initialize HTML report
            init_html_report();
        }
    }

    init_temperature();

    print_header();
    infinite_loop();



    if (g_log_file)
        fclose(g_log_file);

    return 0;
}
