/******************************************************************************
 * Display Module
 *
 * Handles program header and status display
 *****************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <windows.h>
#include <stdint.h>

/* Configuration constants */
#define STATUS_INTERVAL 5  /* iterations between reports */

/* External variables */
extern DWORD g_start_tick;
extern volatile uint32_t g_memory_errors;

/* Function prototypes */
void print_header(void);
void print_status(uint32_t iterations, double cpu_usage);

#endif /* DISPLAY_H */
