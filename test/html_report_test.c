/******************************************************************************
 * PCM-3370 Stress Test Program - HTML Report Generation Module (Test Version)
 *
 * Description: HTML report generation for testing on macOS
 *              This is a simplified version without Windows dependencies
 *
 * Author: Ronan Le Meillat - SCTG Development for ISMO Group Ltd.
 * Date: 2026
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "html_report.h"

/* Test-specific global variables (mock) */
extern volatile uint32_t g_memory_errors;
extern int g_verbose;
extern int g_log_enabled;
extern int g_report_enabled;
extern FILE *g_log_file;
extern FILE *g_json_file;
extern FILE *g_html_file;
extern time_t g_timestamp;
extern char g_timestamp_str[64];
extern char g_json_filename[80];
extern char g_html_filename[80];

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

    fprintf(g_html_file, "\n];\n");
    fprintf(g_html_file, "(function(rows){\n");
    fprintf(g_html_file, "    var tb=document.getElementById('tableBody');\n");
    fprintf(g_html_file, "    if(tb){for(var i=0;i<rows.length;i++){\n");
    fprintf(g_html_file, "        var d=rows[i],r=document.createElement('tr');\n");
    fprintf(g_html_file, "        r.innerHTML='<td>'+d.elapsed_time+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.iteration+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.cpu_usage+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.cpu_temp+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.mb_temp+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.free_memory_mb+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.memory_errors+'</td>'\n");
    fprintf(g_html_file, "            +'<td>'+d.rate_iter_per_min+'</td>';\n");
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
    fprintf(g_html_file, "    <script src=\"https://cdn.jsdelivr.net/npm/chart.js@3.9.1/dist/chart.min.js\"></script>\n");
    fprintf(g_html_file, "    <style>\n");
    fprintf(g_html_file, "        body { font-family: Arial, sans-serif; margin: 20px; }\n");
    fprintf(g_html_file, "        h1 { color: #333; }\n");
    fprintf(g_html_file, "        .chart-container { width: 100%%; height: 400px; margin: 20px 0; }\n");
    fprintf(g_html_file, "        table { border-collapse: collapse; width: 100%%; margin: 20px 0; }\n");
    fprintf(g_html_file, "        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n");
    fprintf(g_html_file, "        th { background-color: #f2f2f2; }\n");
    fprintf(g_html_file, "        tr:nth-child(even) { background-color: #f9f9f9; }\n");
    fprintf(g_html_file, "    </style>\n");
    fprintf(g_html_file, "</head>\n");
    fprintf(g_html_file, "<body>\n");
    fprintf(g_html_file, "    <h1>PCM-3370 Stress Test Report</h1>\n");
    fprintf(g_html_file, "    <p>Generated on: %s</p>\n", ctime(&g_timestamp));
    fprintf(g_html_file, "    <div class=\"chart-container\">\n");
    fprintf(g_html_file, "        <canvas id=\"stressChart\"></canvas>\n");
    fprintf(g_html_file, "    </div>\n");
    fprintf(g_html_file, "    <h2>Detailed Data</h2>\n");
    fprintf(g_html_file, "    <table id=\"dataTable\">\n");
    fprintf(g_html_file, "        <thead>\n");
    fprintf(g_html_file, "            <tr>\n");
    fprintf(g_html_file, "                <th>Elapsed Time</th>\n");
    fprintf(g_html_file, "                <th>Iteration</th>\n");
    fprintf(g_html_file, "                <th>CPU Usage (%%)</th>\n");
    fprintf(g_html_file, "                <th>CPU Temp (\xc2\xb0\x43)</th>\n");
    fprintf(g_html_file, "                <th>MB Temp (\xc2\xb0\x43)</th>\n");
    fprintf(g_html_file, "                <th>Free Memory (MB)</th>\n");
    fprintf(g_html_file, "                <th>Memory Errors</th>\n");
    fprintf(g_html_file, "                <th>Rate (iter/min)</th>\n");
    fprintf(g_html_file, "            </tr>\n");
    fprintf(g_html_file, "        </thead>\n");
    fprintf(g_html_file, "        <tbody id=\"tableBody\">\n");
    fprintf(g_html_file, "        </tbody>\n");
    fprintf(g_html_file, "    </table>\n");
    fprintf(g_html_file, "    <script>\n");
    fprintf(g_html_file, "var stressData=[\n");

    fflush(g_html_file);
    g_html_footer_pos = ftell(g_html_file);
    write_html_footer();
}

/******************************************************************************
 * Append one data row and keep the HTML footer valid.
 *****************************************************************************/
void update_html_report(const char *elapsed_time, uint32_t iteration,
                        double cpu_usage, double cpu_temp, double mb_temp,
                        uint32_t free_memory_mb, uint32_t memory_errors,
                        double rate_iter_per_min)
{
    if (!g_report_enabled || !g_html_file)
        return;

    fseek(g_html_file, g_html_footer_pos, SEEK_SET);

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
    write_html_footer();
}
