/******************************************************************************
 * Display Module Implementation
 *****************************************************************************/

#include <windows.h>
#include "display.h"
#include <stdio.h>
#include "temperature.h"
#include "memory_stress.h"

extern DWORD g_start_tick;
extern int g_wmi_ready;
extern int g_via686b_ready;

/******************************************************************************
 * Print program header with real hardware/OS information
 *****************************************************************************/
void print_header(void)
{
    OSVERSIONINFO osvi;
    SYSTEM_INFO sys;
    MEMORYSTATUS mem;

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx(&osvi);
    GetSystemInfo(&sys);
    GlobalMemoryStatus(&mem);

    printf("============================================================\n");
    printf(" PCM-3370 Stress Test  -  SCTG Development / ISMO Group\n");
    printf("============================================================\n");
    printf(" OS  : Windows %lu.%lu (build %lu) %s\n",
           osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber,
           osvi.dwPlatformId == VER_PLATFORM_WIN32_NT ? "NT" : "9x");
    printf(" CPU : %lu x ", sys.dwNumberOfProcessors);
    switch (sys.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        printf("x86");
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        printf("IA-64");
        break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        printf("x86-64");
        break;
    default:
        printf("Unknown");
        break;
    }
    printf("  (page %lu B)\n", sys.dwPageSize);
    printf(" MEM : %u MB total  /  %u MB free\n",
           (unsigned)(mem.dwTotalPhys >> 20),
           (unsigned)(mem.dwAvailPhys >> 20));
    printf(" TEMP: %s\n",
           g_wmi_ready     ? "WMI ACPI thermal zone" :
           g_via686b_ready ? "VIA VT82C686B ISA 0x6000 (direct)" :
                             "N/A (no sensor accessible)");
    printf("============================================================\n");
    printf(" Tests: CPU-FP  CPU-INT  CPU-Branch  Cache  Memory(%u patterns)\n",
           (unsigned)NUM_PATTERNS);
    printf(" NOTE: No disk I/O - compact flash protected\n");
    printf(" Ctrl+C to stop\n");
    printf("============================================================\n\n");
    printf(" %-9s %-7s %-7s %-7s %-10s %-10s %-10s\n",
           "Elapsed", "Iter", "CPU%", "Temp", "FreeMem", "MemErr", "Rate");
    printf("------------------------------------------------------------\n");
}

/******************************************************************************
 * Print one status line
 *
 * cpu_usage is passed in by the caller - never re-queried here.
 * Temperature is queried from the persistent WMI session (or shows N/A).
 *****************************************************************************/
void print_status(uint32_t iterations, double cpu_usage)
{
    MEMORYSTATUS mem;
    DWORD elapsed_s = (GetTickCount() - g_start_tick) / 1000u;
    uint32_t h = elapsed_s / 3600u;
    uint32_t m = (elapsed_s % 3600u) / 60u;
    uint32_t s = elapsed_s % 60u;
    double iter_per_min = (elapsed_s > 0)
                              ? ((double)iterations / (double)elapsed_s * 60.0)
                              : 0.0;
    double temp_c = get_cpu_temperature();

    GlobalMemoryStatus(&mem);

    if (temp_c >= 0.0)
        printf(" %02u:%02u:%02u  %-7u %5.1f%%  %5.1fC  %4u MB    %-10u %.1f/min\n",
               h, m, s, iterations, cpu_usage, temp_c,
               (unsigned)(mem.dwAvailPhys >> 20),
               g_memory_errors, iter_per_min);
    else
        printf(" %02u:%02u:%02u  %-7u %5.1f%%  N/A    %4u MB    %-10u %.1f/min\n",
               h, m, s, iterations, cpu_usage,
               (unsigned)(mem.dwAvailPhys >> 20),
               g_memory_errors, iter_per_min);
}