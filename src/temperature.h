/******************************************************************************
 * Temperature Monitoring Module
 *
 * Handles CPU temperature monitoring via WMI ACPI and VIA VT82C686B direct I/O
 *****************************************************************************/

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <windows.h>
#include <wbemidl.h>

/* VIA VT82C686B hardware monitor constants */
#define VIA686B_BASE      0x6000u
#define VIA686B_REG_TEMP  0x20u   /* CPU temperature, 8-bit signed °C */
#define SYSDBG_READ_IO    14      /* SysDbgReadIoSpace command code */

/* External variables */
extern int g_wmi_ready;
extern int g_via686b_ready;
extern int g_verbose;

/* Function prototypes */
int init_temperature(void);
double get_cpu_temperature(void);
int init_via686b(void);
void acquire_debug_privilege(void);
void cleanup_via686b(void);

#endif /* TEMPERATURE_H */