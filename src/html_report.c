/******************************************************************************
 * PCM-3370 Stress Test Program - HTML Report Generation Module
 *
 * Description: HTML report generation for PCM-3370 stress test results
 *
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *****************************************************************************/

#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include "html_report.h"

// Declare external variables instead of including main.h to avoid Windows dependencies
extern int g_report_enabled;
extern FILE *g_html_file;
extern time_t g_timestamp;
extern char g_timestamp_str[64];

/* Position in file where the data footer begins (seek target for updates) */
static long g_html_footer_pos = 0;
/* Number of data rows already written */
static int  g_html_data_count = 0;

/******************************************************************************
 * Write the constant HTML footer (closing of data array + render script +
 * closing tags).  Always written at g_html_footer_pos so the file remains a
 * valid HTML document after every update.
 *****************************************************************************/
static void write_html_footer(void)
{
    if (!g_html_file)
        return;

    /* Close the stressData array */
    fprintf(g_html_file, "\n];\n");
    /* IIFE: populate table then draw chart (no external data fetch needed) */
    fprintf(g_html_file, "(function(rows){\n");
    fprintf(g_html_file, "    var tb=document.getElementById('tableBody');\n");
    fprintf(g_html_file, "    if(tb){for(var i=0;i<rows.length;i++){\n");
    fprintf(g_html_file, "        var d=rows[i],r=document.createElement('tr');\n");
    fprintf(g_html_file, "        r.className = i %% 2 === 0 ? 'bg-gray-50' : 'bg-white';\n");
    fprintf(g_html_file, "        r.innerHTML='<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.elapsed_time+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.iteration+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.cpu_usage+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.cpu_temp+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.mb_temp+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.free_memory_mb+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.memory_errors+'</td>'\n");
    fprintf(g_html_file, "            +'<td class=\"px-4 py-2 whitespace-nowrap text-sm text-gray-500\">'+d.rate_iter_per_min+'</td>';\n");
    fprintf(g_html_file, "        tb.appendChild(r);\n");
    fprintf(g_html_file, "    }}\n");
    fprintf(g_html_file, "    var cv=document.getElementById('stressChart');\n");
    fprintf(g_html_file, "    if(cv&&typeof Chart!=='undefined'&&rows.length>0){\n");
    fprintf(g_html_file, "        new Chart(cv.getContext('2d'),{\n");
    fprintf(g_html_file, "            type:'line',\n");
    fprintf(g_html_file, "            data:{\n");
    fprintf(g_html_file, "                labels:rows.map(function(d,i){return i+1;}),\n");
    fprintf(g_html_file, "                datasets:[\n");
    fprintf(g_html_file, "                    {label:'CPU Usage (%%)',data:rows.map(function(d){return d.cpu_usage;}),borderColor:'rgb(75,192,192)',tension:0.1,fill:false},\n");
    fprintf(g_html_file, "                    {label:'CPU Temp (C)',data:rows.map(function(d){return d.cpu_temp;}),borderColor:'rgb(255,99,132)',tension:0.1,fill:false},\n");
    fprintf(g_html_file, "                    {label:'MB Temp (C)',data:rows.map(function(d){return d.mb_temp;}),borderColor:'rgb(255,159,64)',tension:0.1,fill:false}\n");
    fprintf(g_html_file, "                ]\n");
    fprintf(g_html_file, "            },\n");
    fprintf(g_html_file, "            options:{responsive:true,scales:{y:{beginAtZero:true,max:100}}}\n");
    fprintf(g_html_file, "        });\n");
    fprintf(g_html_file, "    }\n");
    fprintf(g_html_file, "})(stressData);\n");
    fprintf(g_html_file, "    </script>\n");
    fprintf(g_html_file, "</body>\n");
    fprintf(g_html_file, "</html>\n");
    fflush(g_html_file);
}

/******************************************************************************
 * Initialize HTML report file
 *****************************************************************************/
void init_html_report(void)
{
    if (!g_report_enabled || !g_html_file)
        return;

    g_html_data_count = 0;

    fprintf(g_html_file, "<!DOCTYPE html>\n");
    fprintf(g_html_file, "<html lang=\"en\">\n");
    fprintf(g_html_file, "<head>\n");
    fprintf(g_html_file, "    <meta charset=\"UTF-8\">\n");
    fprintf(g_html_file, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(g_html_file, "    <title>PCM-3370 Stress Test Report</title>\n");
    fprintf(g_html_file, "    <script src=\"https://cdn.jsdelivr.net/npm/tailwindcss@4.3.0/dist/lib.min.js\"></script>\n");
    fprintf(g_html_file, "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js@3.9.1/dist/chart.min.js\"></script>\n");
    fprintf(g_html_file, "    <style>\n");
    fprintf(g_html_file, "        /* Ensure Chart.js uses sans-serif font */\n");
    fprintf(g_html_file, "        canvas {\n");
    fprintf(g_html_file, "            font-family: sans-serif;\n");
    fprintf(g_html_file, "        }\n");
    fprintf(g_html_file, "    </style>\n");
    fprintf(g_html_file, "</head>\n");
    fprintf(g_html_file, "<body class=\"bg-gray-50 font-sans\">\n");
    fprintf(g_html_file, "    <div class=\"max-w-7xl mx-auto p-6 bg-white shadow-lg rounded-lg\">\n");
    fprintf(g_html_file, "        <header class=\"mb-8\">\n");
    fprintf(g_html_file, "            <h1 class=\"text-3xl font-bold text-gray-800 border-b-2 border-blue-600 pb-2 mb-4\">PCM-3370 Stress Test Report</h1>\n");
    fprintf(g_html_file, "            <p class=\"text-gray-600 text-sm\">Generated on: %s</p>\n", ctime(&g_timestamp));
    fprintf(g_html_file, "        </header>\n");
    fprintf(g_html_file, "        <div class=\"bg-white rounded-lg shadow-md p-6 mb-8\">\n");
    fprintf(g_html_file, "            <div class=\"h-96\">\n");
    fprintf(g_html_file, "                <canvas id=\"stressChart\"></canvas>\n");
    fprintf(g_html_file, "            </div>\n");
    fprintf(g_html_file, "        </div>\n");
    fprintf(g_html_file, "        <div class=\"overflow-x-auto\">\n");
    fprintf(g_html_file, "            <h2 class=\"text-2xl font-semibold text-gray-700 mb-4 border-b border-gray-200 pb-2\">Detailed Data</h2>\n");
    fprintf(g_html_file, "            <div class=\"bg-white rounded-lg shadow overflow-hidden\">\n");
    fprintf(g_html_file, "                <table id=\"dataTable\" class=\"min-w-full divide-y divide-gray-200\">\n");
    fprintf(g_html_file, "                    <thead class=\"bg-gray-50\">\n");
    fprintf(g_html_file, "                        <tr>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">Elapsed Time</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">Iteration</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">CPU Usage (%%)</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">CPU Temp (\xc2\xb0\x43)</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">MB Temp (\xc2\xb0\x43)</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">Free Memory (MB)</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">Memory Errors</th>\n");
    fprintf(g_html_file, "                            <th class=\"px-4 py-3 text-left text-xs font-medium text-gray-500 uppercase tracking-wider\">Rate (iter/min)</th>\n");
    fprintf(g_html_file, "                        </tr>\n");
    fprintf(g_html_file, "                    </thead>\n");
    fprintf(g_html_file, "                    <tbody id=\"tableBody\" class=\"bg-white divide-y divide-gray-200\">\n");
    fprintf(g_html_file, "                    </tbody>\n");
    fprintf(g_html_file, "                </table>\n");
    fprintf(g_html_file, "            </div>\n");
    fprintf(g_html_file, "        </div>\n");
    /* Inline data array - rows appended by update_html_report() */
    fprintf(g_html_file, "    <script>\n");
    fprintf(g_html_file, "var stressData=[\n");

    /* Record the position where the footer (and future data rows) start */
    fflush(g_html_file);
    g_html_footer_pos = ftell(g_html_file);

    /* Write a complete footer so the file is valid even with zero data rows */
    write_html_footer();
}

/******************************************************************************
 * Append one data row and keep the HTML footer valid.
 * Safe to call at any frequency - Ctrl+C between two calls still leaves a
 * well-formed HTML document.
 *****************************************************************************/
void update_html_report(const char *elapsed_time, uint32_t iteration,
                        double cpu_usage, double cpu_temp, double mb_temp,
                        uint32_t free_memory_mb, uint32_t memory_errors,
                        double rate_iter_per_min)
{
    if (!g_report_enabled || !g_html_file)
        return;

    /* Seek back to just after the opening '[' of the stressData array,
     * overwriting the previous footer */
    fseek(g_html_file, g_html_footer_pos, SEEK_SET);

    /* Comma-separate entries after the first */
    if (g_html_data_count > 0)
        fprintf(g_html_file, ",\n");

    fprintf(g_html_file,
            "{\"elapsed_time\":\"%s\",\"iteration\":%u,"
            "\"cpu_usage\":%.1f,\"cpu_temp\":%.1f,\"mb_temp\":%.1f,"
            "\"free_memory_mb\":%u,\"memory_errors\":%u,"
            "\"rate_iter_per_min\":%.1f}",
            elapsed_time, iteration,
            cpu_usage, cpu_temp, mb_temp,
            free_memory_mb, memory_errors, rate_iter_per_min);

    g_html_data_count++;
    g_html_footer_pos = ftell(g_html_file);

    /* Rewrite footer so file is always a valid HTML document */
    write_html_footer();
}