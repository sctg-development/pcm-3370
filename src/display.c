/******************************************************************************
 * Display Module Implementation
 *****************************************************************************/

#include <windows.h>
#include "display.h"
#include <stdio.h>
#include "temperature.h"
#include "memory_stress.h"

extern DWORD g_start_tick;

void log_message(const char *format, ...);

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

    log_message("============================================================\n");
    log_message(" PCM-3370 Stress Test  -  SCTG Development / ISMO Group\n");
    log_message("============================================================\n");
    log_message(" OS  : Windows %lu.%lu (build %lu) %s\n",
           osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber,
           osvi.dwPlatformId == VER_PLATFORM_WIN32_NT ? "NT" : "9x");
    log_message(" CPU : %lu x ", sys.dwNumberOfProcessors);
    switch (sys.wProcessorArchitecture)
    {
    case PROCESSOR_ARCHITECTURE_INTEL:
        log_message("x86");
        break;
    case PROCESSOR_ARCHITECTURE_IA64:
        log_message("IA-64");
        break;
    case PROCESSOR_ARCHITECTURE_AMD64:
        log_message("x86-64");
        break;
    default:
        log_message("Unknown");
        break;
    }
    log_message("  (page %lu B)\n", sys.dwPageSize);
    log_message(" MEM : %u MB total  /  %u MB free\n",
           (unsigned)(mem.dwTotalPhys >> 20),
           (unsigned)(mem.dwAvailPhys >> 20));
    if (g_via686b_ready)
        log_message(" TEMP: VIA VT82C686B  ISA 0x%04X  (PortTalk.sys / NtSysDbg)\n",
                    VIA686B_BASE);
    else
        log_message(" TEMP: VIA VT82C686B  not accessible (run as admin + PortTalk.sys)\n");
    log_message("============================================================\n");
    log_message(" Tests: CPU-FP  CPU-INT  CPU-Branch  Cache  Memory(%u patterns)\n",
           (unsigned)NUM_PATTERNS);
    log_message(" NOTE: No disk I/O - compact flash protected\n");
    log_message(" Ctrl+C to stop\n");
    log_message("============================================================\n\n");
    log_message(" %-9s %-7s %-7s %-7s %-7s %-10s %-10s %-10s\n",
           "Elapsed", "Iter", "CPU%", "CPUTemp", "MBTemp", "FreeMem", "MemErr", "Rate");
    log_message("------------------------------------------------------------\n");
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
    double temp_mb = get_mb_temperature();

    GlobalMemoryStatus(&mem);

    if (temp_c >= 0.0 && temp_mb >= 0.0)
        log_message(" %02u:%02u:%02u  %-7u %5.1f%%  %5.1fC  %5.1fC  %4u MB    %-10u %.1f/min\n",
               h, m, s, iterations, cpu_usage, temp_c, temp_mb,
               (unsigned)(mem.dwAvailPhys >> 20),
               g_memory_errors, iter_per_min);
    else if (temp_c >= 0.0)
        log_message(" %02u:%02u:%02u  %-7u %5.1f%%  %5.1fC  N/A    %4u MB    %-10u %.1f/min\n",
               h, m, s, iterations, cpu_usage, temp_c,
               (unsigned)(mem.dwAvailPhys >> 20),
               g_memory_errors, iter_per_min);
    else if (temp_mb >= 0.0)
        log_message(" %02u:%02u:%02u  %-7u %5.1f%%  N/A    %5.1fC  %4u MB    %-10u %.1f/min\n",
               h, m, s, iterations, cpu_usage, temp_mb,
               (unsigned)(mem.dwAvailPhys >> 20),
               g_memory_errors, iter_per_min);
    else
        log_message(" %02u:%02u:%02u  %-7u %5.1f%%  N/A    N/A    %4u MB    %-10u %.1f/min\n",
               h, m, s, iterations, cpu_usage,
               (unsigned)(mem.dwAvailPhys >> 20),
               g_memory_errors, iter_per_min);
}