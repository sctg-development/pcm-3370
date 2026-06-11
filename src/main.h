/******************************************************************************
 * PCM-3370 Stress Test Program - Main Module Header
 *
 * Description: Header file for main module global variables
 *
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *****************************************************************************/

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <time.h>

/* ---- Global state --------------------------------------------------------- */
extern volatile uint32_t g_memory_errors;
extern DWORD g_start_tick;
extern int   g_verbose;
extern int   g_log_enabled;
extern int   g_report_enabled;
extern FILE *g_log_file;
extern FILE *g_json_file;
extern FILE *g_html_file;
extern time_t g_timestamp;
extern char g_timestamp_str[64];
extern char g_json_filename[80];
extern char g_html_filename[80];

#endif // MAIN_H