/******************************************************************************
 * PCM-3370 Stress Test Program - HTML Report Test
 *
 * Description: Test program to validate HTML report generation
 *              Uses simulated data from 123456789.ndson file
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
#include "../src/html_report.h"

// Mock global variables for testing
volatile uint32_t g_memory_errors = 0;
int g_verbose = 0;
int g_log_enabled = 0;
int g_report_enabled = 1;
FILE *g_log_file = NULL;
FILE *g_json_file = NULL;
FILE *g_html_file = NULL;
time_t g_timestamp = 123456789;
char g_timestamp_str[64] = "123456789";
char g_json_filename[80] = "123456789.ndjson";
char g_html_filename[80] = "123456789.html";

/******************************************************************************
 * Main test function
 *****************************************************************************/
int main(int argc, char **argv)
{
    printf("PCM-3370 HTML Report Generation Test\n");
    printf("===================================\n\n");

    // Set timestamp for consistent test results
    g_timestamp = 123456789;
    snprintf(g_timestamp_str, sizeof(g_timestamp_str), "%ld", g_timestamp);
    snprintf(g_html_filename, sizeof(g_html_filename), "%s.html", g_timestamp_str);

    printf("Generating HTML report: %s\n", g_html_filename);

    // Open HTML file for writing
    g_html_file = fopen(g_html_filename, "w");
    if (!g_html_file)
    {
        printf("Error: Could not open %s for writing\n", g_html_filename);
        return 1;
    }

    // Initialize HTML report
    init_html_report();

    // Close the file
    fclose(g_html_file);
    g_html_file = NULL;

    printf("HTML report generated successfully!\n");

    // Verify the file was created
    FILE *test_file = fopen(g_html_filename, "r");
    if (!test_file)
    {
        printf("Error: Could not open generated file for verification\n");
        return 1;
    }

    // Check file size
    fseek(test_file, 0, SEEK_END);
    long file_size = ftell(test_file);
    fseek(test_file, 0, SEEK_SET);

    printf("Generated file size: %ld bytes\n", file_size);

    // Check for key HTML elements
    char buffer[1024];
    int has_doctype = 0;
    int has_title = 0;
    int has_chart = 0;
    int has_table = 0;

    while (fgets(buffer, sizeof(buffer), test_file))
    {
        if (strstr(buffer, "<!DOCTYPE html>")) has_doctype = 1;
        if (strstr(buffer, "PCM-3370 Stress Test Report")) has_title = 1;
        if (strstr(buffer, "stressChart")) has_chart = 1;
        if (strstr(buffer, "dataTable")) has_table = 1;
    }

    fclose(test_file);

    // Verify all required elements are present
    printf("\nValidation Results:\n");
    printf("  DOCTYPE declaration: %s\n", has_doctype ? "✓" : "✗");
    printf("  Title element: %s\n", has_title ? "✓" : "✗");
    printf("  Chart container: %s\n", has_chart ? "✓" : "✗");
    printf("  Data table: %s\n", has_table ? "✓" : "✗");

    if (has_doctype && has_title && has_chart && has_table)
    {
        printf("\n✓ All tests passed! HTML report generation is working correctly.\n");
        return 0;
    }
    else
    {
        printf("\n✗ Some tests failed! HTML report generation has issues.\n");
        return 1;
    }
}