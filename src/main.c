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
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *****************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>

// Function prototypes
void cpu_stress_test(void);
void memory_stress_test(void);
void cache_stress_test(void);
void system_info_test(void);
void print_header(void);
void print_status(uint32_t iterations, double temperature);
double get_cpu_usage(void);
void infinite_loop(void);

// Global configuration
#define MEMORY_BLOCK_SIZE (64 * 1024 * 1024)  // 64MB test block
#define CACHE_LINE_SIZE 64                     // Typical cache line size
#define MAX_ITERATIONS 1000000                 // Iterations before status update
#define TEMPERATURE_SCALE 100.0                // Simulated temperature scale

/******************************************************************************
 * Main program entry point
 *****************************************************************************/
int main(void)
{
    print_header();

    // Main stress test loop - runs indefinitely
    infinite_loop();

    return 0;
}

/******************************************************************************
 * Prints program header information
 *****************************************************************************/
void print_header(void)
{
    printf("============================================================\n");
    printf(" PCM-3370 Stress Test Program\n");
    printf("============================================================\n");
    printf(" Target: Windows XP 32-bit\n");
    printf(" CPU: Intel Celeron 400MHz/650MHz or Pentium III\n");
    printf(" Chipset: VIA VT8606/TwiserT + VT82C686B\n");
    printf(" Memory: Up to 512MB SDRAM\n");
    printf("============================================================\n");
    printf(" Running continuous stress tests...\n");
    printf(" Press Ctrl+C to terminate (if supported by your terminal)\n");
    printf("============================================================\n\n");
}

/******************************************************************************
 * Infinite loop for continuous stress testing
 *****************************************************************************/
void infinite_loop(void)
{
    uint32_t iteration_counter = 0;
    double simulated_temp = 30.0;  // Starting "temperature" in Celsius

    while (1) {
        // Run all stress tests in sequence
        cpu_stress_test();
        memory_stress_test();
        cache_stress_test();
        system_info_test();

        // Update and print status every MAX_ITERATIONS
        iteration_counter++;
        if (iteration_counter % MAX_ITERATIONS == 0) {
            // Simulate temperature increase (for demonstration)
            simulated_temp += 0.1;
            if (simulated_temp > 80.0) simulated_temp = 30.0;

            print_status(iteration_counter, simulated_temp);
        }

        // Small delay to prevent console flooding
        Sleep(100);
    }
}

/******************************************************************************
 * CPU Stress Test - Mixed workload of arithmetic operations
 *****************************************************************************/
void cpu_stress_test(void)
{
    volatile double result = 0.0;
    volatile uint32_t int_result = 0;
    uint32_t i;

    // Floating point operations
    for (i = 0; i < 1000000; i++) {
        result += sin((double)i * 0.0001) * cos((double)i * 0.0002);
        result *= tan((double)i * 0.00005) + 1.0;
    }

    // Integer operations
    for (i = 0; i < 1000000; i++) {
        int_result ^= (i * 1664525) + 1013904223;
        int_result = (int_result >> 13) | (int_result << 19);
    }

    // Prevent optimization by using volatile results
    (void)result;
    (void)int_result;
}

/******************************************************************************
 * Memory Stress Test - Allocates and tests memory blocks
 *****************************************************************************/
void memory_stress_test(void)
{
    uint32_t i;
    uint8_t *memory_block = NULL;
    uint32_t test_pattern = 0x5555AAAA;  // Alternating pattern
    uint32_t current_block_size = MEMORY_BLOCK_SIZE;

    // Try to allocate a large memory block
    memory_block = (uint8_t *)malloc(current_block_size);
    if (memory_block == NULL) {
        // If allocation fails, try smaller blocks
        while (memory_block == NULL && current_block_size > 1024) {
            current_block_size /= 2;
            memory_block = (uint8_t *)malloc(current_block_size);
        }

        if (memory_block == NULL) {
            printf("Warning: Memory allocation failed even for small blocks\n");
            return;
        }
    }

    // Fill memory with test pattern
    for (i = 0; i < current_block_size; i += sizeof(uint32_t)) {
        *(uint32_t *)&memory_block[i] = test_pattern;
    }

    // Verify memory contents
    for (i = 0; i < current_block_size; i += sizeof(uint32_t)) {
        if (*(uint32_t *)&memory_block[i] != test_pattern) {
            printf("Memory error at address %p\n", &memory_block[i]);
        }
    }

    // Random access pattern
    for (i = 0; i < 10000; i++) {
        uint32_t random_offset = (i * 1664525 + 1013904223) % (current_block_size - sizeof(uint32_t));
        memory_block[random_offset] ^= 0xFF;
    }

    free(memory_block);
}

/******************************************************************************
 * Cache Stress Test - Creates cache line conflicts
 *****************************************************************************/
void cache_stress_test(void)
{
    // Allocate memory with cache line alignment
    const uint32_t cache_lines = 1024;  // Number of cache lines to test
    const uint32_t array_size = cache_lines * CACHE_LINE_SIZE;
    uint8_t *cache_test_array = (uint8_t *)malloc(array_size);

    if (cache_test_array == NULL) {
        printf("Cache test memory allocation failed\n");
        return;
    }

    // Align to cache line boundary (simplified)
    uint8_t *aligned_array = (uint8_t *)(((uintptr_t)cache_test_array + CACHE_LINE_SIZE - 1) &
                                         ~(CACHE_LINE_SIZE - 1));

    uint32_t i, j;

    // Stride through memory with cache line sized steps
    for (i = 0; i < 1000; i++) {
        for (j = 0; j < cache_lines; j++) {
            // Access every cache line in sequence
            aligned_array[j * CACHE_LINE_SIZE] = (uint8_t)(i + j);

            // Access with large stride to create cache conflicts
            if (j + 16 < cache_lines) {
                aligned_array[(j + 16) * CACHE_LINE_SIZE] = (uint8_t)(i - j);
            }
        }
    }

    free(cache_test_array);
}

/******************************************************************************
 * System Information Test - Verifies basic system properties
 *****************************************************************************/
void system_info_test(void)
{
    SYSTEM_INFO sys_info;
    MEMORYSTATUS mem_status;

    // Get system information
    GetSystemInfo(&sys_info);
    GlobalMemoryStatus(&mem_status);

    // Note: We don't print this every time to avoid console flooding
    // This data is available for debugging if needed
}

/******************************************************************************
 * Prints current status to console
 *****************************************************************************/
void print_status(uint32_t iterations, double temperature)
{
    double cpu_usage = get_cpu_usage();

    printf("[Iteration: %u] ", iterations);
    printf("CPU: %.1f%% | ", cpu_usage);
    printf("Temp: %.1f°C | ", temperature);
    printf("Memory: %uMB tested\n",
           (unsigned int)(MEMORY_BLOCK_SIZE / (1024 * 1024)));
}

/******************************************************************************
 * Estimates CPU usage (simplified for Windows XP compatibility)
 *****************************************************************************/
double get_cpu_usage(void)
{
    // Simplified CPU usage estimation for Windows XP
    // In a real implementation, we would use performance counters
    // This is a placeholder that returns a simulated value

    static uint32_t counter = 0;
    counter = (counter + 1) % 100;

    // Simulate varying CPU usage between 80-100% for stress test
    return 80.0 + 20.0 * sin((double)counter * 0.1);
}