/******************************************************************************
 * PCM-3370 Stress Test Program
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

/* COM object macros for C (no C++ this-pointer syntax) */
#define COBJMACROS

#include <windows.h>
#include <objbase.h>
#include <wbemidl.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

/* ---- Configuration -------------------------------------------------------- */
#define MEMORY_BLOCK_SIZE (64 * 1024 * 1024) /* 64 MB test block           */
#define CACHE_LINE_SIZE 64                   /* x86 cache line             */
#define STATUS_INTERVAL 5                    /* iterations between reports */
#define CPU_FP_ITERS 500000u                 /* FP ops per iteration       */
#define CPU_INT_ITERS 1000000u               /* integer ops per iteration  */
#define CPU_BRANCH_ITERS 500000u             /* branch stress iters        */

/* ---- Memory test patterns ------------------------------------------------- */
static const uint32_t TEST_PATTERNS[] = {
    0x5555AAAAu, 0xAAAA5555u, /* alternating nibbles */
    0xFF00FF00u, 0x00FF00FFu, /* alternating bytes   */
    0xDEADBEEFu, 0xCAFEBABEu, /* arbitrary           */
    0x00000000u, 0xFFFFFFFFu  /* all-zeros / all-ones */
};
#define NUM_PATTERNS ((uint32_t)(sizeof(TEST_PATTERNS) / sizeof(TEST_PATTERNS[0])))

/* ---- Global state --------------------------------------------------------- */
static volatile uint32_t g_memory_errors = 0;
static DWORD g_start_tick = 0;

/* ---- WMI session for temperature ----------------------------------------- */
static IWbemServices *g_wmi_svc = NULL;
static int g_wmi_ready = 0;

/* ---- CPU snapshot for GetSystemTimes-based measurement -------------------- */
typedef struct
{
    ULONGLONG idle;
    ULONGLONG total;
} CpuSnapshot;

/* ---- Function prototypes -------------------------------------------------- */
void cpu_stress_test(void);
void memory_stress_test(void);
void cache_stress_test(void);
void print_header(void);
void print_status(uint32_t iterations, double cpu_usage);
double get_cpu_usage(void);
int init_temperature(void);
double get_cpu_temperature(void);
void infinite_loop(void);

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
double get_cpu_usage(void)
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
 * WMI temperature: open one persistent session to ROOT\WMI
 *
 * MSAcpi_ThermalZoneTemperature.CurrentTemperature is in tenths of Kelvin.
 * Returns 1 on success, 0 if WMI is unavailable (no ACPI thermal zones).
 *****************************************************************************/
int init_temperature(void)
{
    IWbemLocator *pLoc = NULL;
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 0;

    /* Ignore error: may already have been called */
    CoInitializeSecurity(NULL, -1, NULL, NULL,
                         RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                         NULL, EOAC_NONE, NULL);

    hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWbemLocator, (void **)&pLoc);
    if (FAILED(hr))
    {
        CoUninitialize();
        return 0;
    }

    hr = IWbemLocator_ConnectServer(pLoc,
                                    L"ROOT\\WMI", NULL, NULL, 0, 0, 0, 0, &g_wmi_svc);
    IWbemLocator_Release(pLoc);
    if (FAILED(hr))
    {
        CoUninitialize();
        return 0;
    }

    hr = CoSetProxyBlanket((IUnknown *)g_wmi_svc,
                           RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        IWbemServices_Release(g_wmi_svc);
        g_wmi_svc = NULL;
        CoUninitialize();
        return 0;
    }

    g_wmi_ready = 1;
    return 1;
}

/******************************************************************************
 * Read CPU temperature from WMI ACPI thermal zone
 *
 * Returns temperature in Celsius, or -1.0 if unavailable.
 *****************************************************************************/
double get_cpu_temperature(void)
{
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG returned = 0;
    VARIANT vt;
    double temp = -1.0;
    HRESULT hr;

    if (!g_wmi_ready || !g_wmi_svc)
        return -1.0;

    hr = IWbemServices_ExecQuery(g_wmi_svc,
                                 L"WQL",
                                 L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature",
                                 WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                 NULL, &pEnum);
    if (FAILED(hr))
        return -1.0;

    if (IEnumWbemClassObject_Next(pEnum, WBEM_INFINITE, 1, &pObj, &returned) == S_OK && returned > 0)
    {
        VariantInit(&vt);
        hr = IWbemClassObject_Get(pObj, L"CurrentTemperature", 0, &vt, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            /* WMI reports tenths of Kelvin; convert to Celsius */
            if (vt.vt == VT_I4)
                temp = (double)vt.lVal / 10.0 - 273.15;
            else if (vt.vt == VT_UI4)
                temp = (double)vt.ulVal / 10.0 - 273.15;
        }
        VariantClear(&vt);
        IWbemClassObject_Release(pObj);
    }
    IEnumWbemClassObject_Release(pEnum);
    return temp;
}

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
        printf("Warning: memory allocation failed\n");
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
                printf("MEM ERR offset +%u  expected %08X  got %08X\n",
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

/******************************************************************************
 * Cache Stress Test
 *
 * Three access patterns to defeat cache policies:
 *   1. Sequential forward  - baseline; warm in L2
 *   2. Sequential reverse  - defeats simple next-line hardware prefetcher
 *   3. Power-of-2 stride   - creates set-associativity conflicts in 4-way caches
 *****************************************************************************/
void cache_stress_test(void)
{
    const uint32_t cache_lines = 2048u;
    const uint32_t alloc_size = cache_lines * CACHE_LINE_SIZE + CACHE_LINE_SIZE;
    uint8_t *buf = (uint8_t *)malloc(alloc_size);
    uint8_t *arr;
    uint32_t i, j;
    volatile uint8_t sink = 0;

    if (buf == NULL)
        return;

    /* Align to cache-line boundary */
    arr = (uint8_t *)(((uintptr_t)buf + (uintptr_t)(CACHE_LINE_SIZE - 1)) & ~(uintptr_t)(CACHE_LINE_SIZE - 1));

    /* --- Forward stride --------------------------------------------------- */
    for (i = 0; i < 2000u; i++)
    {
        for (j = 0; j < cache_lines; j++)
            sink += arr[j * CACHE_LINE_SIZE];
    }

    /* --- Reverse stride --------------------------------------------------- */
    for (i = 0; i < 2000u; i++)
    {
        for (j = cache_lines; j-- > 0;)
            sink += arr[j * CACHE_LINE_SIZE];
    }

    /* --- Conflict pattern: stride = 128 lines (power of 2) ---------------- */
    for (i = 0; i < 2000u; i++)
    {
        for (j = 0; j < cache_lines; j += 16u)
            arr[j * CACHE_LINE_SIZE] = (uint8_t)(i + j);
    }

    (void)sink;
    free(buf);
}

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
    printf(" TEMP: %s\n", g_wmi_ready ? "WMI ACPI thermal zone" : "N/A (no ACPI sensor)");
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

    /* Initialize WMI temperature before printing header */
    init_temperature();

    print_header();
    infinite_loop();
    return 0;
}
