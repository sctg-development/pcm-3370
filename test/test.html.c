/******************************************************************************
 * PCM-3370 Stress Test Program - HTML Report Test
 *
 * Description: Test program to validate HTML report generation.
 *              Reads simulated data from 123456789.ndson and calls
 *              update_html_report() for each line so the output HTML is
 *              self-contained (no external fetch needed).
 *
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include "../src/html_report.h"

/* Mock global variables */
volatile uint32_t g_memory_errors = 0;
int g_verbose        = 0;
int g_log_enabled    = 0;
int g_report_enabled = 1;
FILE *g_log_file     = NULL;
FILE *g_json_file    = NULL;
FILE *g_html_file    = NULL;
time_t g_timestamp   = 123456789;
char g_timestamp_str[64] = "123456789";
char g_json_filename[80] = "123456789.ndjson";
char g_html_filename[80] = "123456789.html";

/******************************************************************************
 * Parse one NDJSON line (known fixed format) and call update_html_report.
 * Returns 1 on success, 0 on parse failure.
 *****************************************************************************/
static int process_ndjson_line(const char *line)
{
    char elapsed_time[20] = {0};
    unsigned int iteration = 0;
    double cpu_usage = 0.0, cpu_temp = 0.0, mb_temp = 0.0;
    unsigned int free_memory_mb = 0, memory_errors = 0;
    double rate = 0.0;

    int n = sscanf(line,
        "{\"elapsed_time\":\"%19[^\"]\"," \
        "\"iteration\":%u," \
        "\"cpu_usage\":%lf," \
        "\"cpu_temp\":%lf," \
        "\"mb_temp\":%lf," \
        "\"free_memory_mb\":%u," \
        "\"memory_errors\":%u," \
        "\"rate_iter_per_min\":%lf}",
        elapsed_time, &iteration,
        &cpu_usage, &cpu_temp, &mb_temp,
        &free_memory_mb, &memory_errors, &rate);

    if (n != 8)
        return 0;

    update_html_report(elapsed_time, (uint32_t)iteration,
                       cpu_usage, cpu_temp, mb_temp,
                       (uint32_t)free_memory_mb, (uint32_t)memory_errors, rate);
    return 1;
}

/******************************************************************************
 * Main test function
 *****************************************************************************/
int main(int argc, char **argv)
{
    (void)argc; (void)argv;

    printf("PCM-3370 HTML Report Generation Test\n");
    printf("=====================================\n\n");

    g_timestamp = 123456789;
    snprintf(g_timestamp_str, sizeof(g_timestamp_str), "%ld", (long)g_timestamp);
    snprintf(g_html_filename, sizeof(g_html_filename), "%s.html", g_timestamp_str);

    printf("Generating HTML report: %s\n", g_html_filename);

    g_html_file = fopen(g_html_filename, "w+");
    if (!g_html_file) {
        printf("Error: Could not open %s for writing\n", g_html_filename);
        return 1;
    }

    /* Write the HTML shell (data array starts empty but file is valid) */
    init_html_report();

    /* Load sample NDJSON and embed each row into the HTML */
    FILE *ndjson = fopen("123456789.ndson", "r");
    int rows_written = 0;
    if (ndjson) {
        char line[512];
        while (fgets(line, sizeof(line), ndjson)) {
            /* Strip trailing newline */
            size_t len = strlen(line);
            while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r'))
                line[--len] = '\0';
            if (len == 0)
                continue;
            if (process_ndjson_line(line))
                rows_written++;
            else
                printf("Warning: could not parse line: %s\n", line);
        }
        fclose(ndjson);
        printf("Embedded %d data rows from 123456789.ndson\n", rows_written);
    } else {
        printf("Note: 123456789.ndson not found - generating empty report\n");
    }

    fclose(g_html_file);
    g_html_file = NULL;

    printf("HTML report generated successfully!\n");

    /* --- Validation --- */
    FILE *test_file = fopen(g_html_filename, "r");
    if (!test_file) {
        printf("Error: Could not open generated file for verification\n");
        return 1;
    }

    fseek(test_file, 0, SEEK_END);
    long file_size = ftell(test_file);
    rewind(test_file);

    printf("Generated file size: %ld bytes\n", file_size);

    char buffer[512];
    int has_doctype = 0, has_title = 0, has_chart = 0, has_table = 0;
    int has_data = 0, has_no_fetch = 1;

    while (fgets(buffer, sizeof(buffer), test_file)) {
        if (strstr(buffer, "<!DOCTYPE html>"))  has_doctype = 1;
        if (strstr(buffer, "PCM-3370 Stress Test Report")) has_title = 1;
        if (strstr(buffer, "stressChart"))       has_chart   = 1;
        if (strstr(buffer, "dataTable"))         has_table   = 1;
        if (strstr(buffer, "stressData"))        has_data    = 1;
        if (strstr(buffer, "fetch("))            has_no_fetch = 0;
    }
    fclose(test_file);

    printf("\nValidation Results:\n");
    printf("  DOCTYPE declaration:      %s\n", has_doctype  ? "PASS" : "FAIL");
    printf("  Title element:            %s\n", has_title    ? "PASS" : "FAIL");
    printf("  Chart container:          %s\n", has_chart    ? "PASS" : "FAIL");
    printf("  Data table:               %s\n", has_table    ? "PASS" : "FAIL");
    printf("  Inline stressData array:  %s\n", has_data     ? "PASS" : "FAIL");
    printf("  No fetch() call:          %s\n", has_no_fetch ? "PASS" : "FAIL");

    if (has_doctype && has_title && has_chart && has_table && has_data && has_no_fetch) {
        printf("\nAll tests passed!\n");
        return 0;
    } else {
        printf("\nSome tests FAILED.\n");
        return 1;
    }
}
