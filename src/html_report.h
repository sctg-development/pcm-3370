/******************************************************************************
 * PCM-3370 Stress Test Program - HTML Report Generation Header
 *
 * Description: Header file for HTML report generation module
 *
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *****************************************************************************/

#ifndef HTML_REPORT_H
#define HTML_REPORT_H

/******************************************************************************
 * Function prototypes
 *****************************************************************************/
void init_html_report(void);
void update_html_report(const char *elapsed_time, uint32_t iteration,
                        double cpu_usage, double cpu_temp, double mb_temp,
                        uint32_t free_memory_mb, uint32_t memory_errors,
                        double rate_iter_per_min);

#endif // HTML_REPORT_H