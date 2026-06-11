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

// Test-specific global variables (mock)
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

/******************************************************************************
 * Initialize HTML report file
 *****************************************************************************/
void init_html_report(void)
{
    if (!g_report_enabled || !g_html_file)
        return;

    // Write HTML header with Chart.js
    fprintf(g_html_file, "<!DOCTYPE html>\n");
    fprintf(g_html_file, "<html lang=\"en\">\n");
    fprintf(g_html_file, "<head>\n");
    fprintf(g_html_file, "    <meta charset=\"UTF-8\">\n");
    fprintf(g_html_file, "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n");
    fprintf(g_html_file, "    <title>PCM-3370 Stress Test Report</title>\n");
    fprintf(g_html_file, "    <script src=\"https://cdnjs.cloudflare.com/ajax/libs/Chart.js/4.5.0/chart.min.js\" integrity=\"sha512-n/G+dROKbKL3GVngGWmWfwK0yPctjZQM752diVYnXZtD/48agpUKLIn0xDQL9ydZ91x6BiOmTIFwWjjFi2kEFg==\" crossorigin=\"anonymous\" referrerpolicy=\"no-referrer\"></script>\n");
    fprintf(g_html_file, "    <style>\n");
    fprintf(g_html_file, "        body { font-family: Arial, sans-serif; margin: 20px; }\n");
    fprintf(g_html_file, "        h1 { color: #333; }\n");
    fprintf(g_html_file, "        .chart-container { width: 100%%%%; height: 400px; margin: 20px 0; }\n");
    fprintf(g_html_file, "        table { border-collapse: collapse; width: 100%%%%; margin: 20px 0; }\n");
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
    fprintf(g_html_file, "                <th>CPU Usage (%%%%)</th>\n");
    fprintf(g_html_file, "                <th>CPU Temp (°C)</th>\n");
    fprintf(g_html_file, "                <th>MB Temp (°C)</th>\n");
    fprintf(g_html_file, "                <th>Free Memory (MB)</th>\n");
    fprintf(g_html_file, "                <th>Memory Errors</th>\n");
    fprintf(g_html_file, "                <th>Rate (iter/min)</th>\n");
    fprintf(g_html_file, "            </tr>\n");
    fprintf(g_html_file, "        </thead>\n");
    fprintf(g_html_file, "        <tbody id=\"tableBody\">\n");
    fprintf(g_html_file, "        </tbody>\n");
    fprintf(g_html_file, "    </table>\n");
    fprintf(g_html_file, "    <script>\n");
    fprintf(g_html_file, "        // Fetch and parse NDJSON data\n");
    fprintf(g_html_file, "        async function loadData() {\n");
    fprintf(g_html_file, "            try {\n");
    fprintf(g_html_file, "                const timestamp = '%s';\n", g_timestamp_str);
    fprintf(g_html_file, "                const response = await fetch(timestamp + '.ndjson');\n");
    fprintf(g_html_file, "                if (!response.ok) throw new Error('Failed to fetch data');\n");
    fprintf(g_html_file, "                \n");
    fprintf(g_html_file, "                const text = await response.text();\n");
    fprintf(g_html_file, "                const lines = text.trim().split('\\n');\n");
    fprintf(g_html_file, "                \n");
    fprintf(g_html_file, "                const chartData = [];\n");
    fprintf(g_html_file, "                const tableBody = document.getElementById('tableBody');\n");
    fprintf(g_html_file, "                \n");
    fprintf(g_html_file, "                lines.forEach((line, index) => {\n");
    fprintf(g_html_file, "                    if (!line.trim()) return;\n");
    fprintf(g_html_file, "                    \n");
    fprintf(g_html_file, "                    try {\n");
    fprintf(g_html_file, "                        const data = JSON.parse(line);\n");
    fprintf(g_html_file, "                        chartData.push({\n");
    fprintf(g_html_file, "                            x: index + 1,\n");
    fprintf(g_html_file, "                            y: data.cpu_usage\n");
    fprintf(g_html_file, "                        });\n");
    fprintf(g_html_file, "                        \n");
    fprintf(g_html_file, "                        // Add row to table\n");
    fprintf(g_html_file, "                        const row = document.createElement('tr');\n");
    fprintf(g_html_file, "                        row.innerHTML = `\n");
    fprintf(g_html_file, "                            <td>${data.elapsed_time}</td>\n");
    fprintf(g_html_file, "                            <td>${data.iteration}</td>\n");
    fprintf(g_html_file, "                            <td>${data.cpu_usage}</td>\n");
    fprintf(g_html_file, "                            <td>${data.cpu_temp}</td>\n");
    fprintf(g_html_file, "                            <td>${data.mb_temp}</td>\n");
    fprintf(g_html_file, "                            <td>${data.free_memory_mb}</td>\n");
    fprintf(g_html_file, "                            <td>${data.memory_errors}</td>\n");
    fprintf(g_html_file, "                            <td>${data.rate_iter_per_min}</td>\n");
    fprintf(g_html_file, "                        `;\n");
    fprintf(g_html_file, "                        tableBody.appendChild(row);\n");
    fprintf(g_html_file, "                    } catch (e) {\n");
    fprintf(g_html_file, "                        console.error('Error parsing line', index, ':', e);\n");
    fprintf(g_html_file, "                    }\n");
    fprintf(g_html_file, "                });\n");
    fprintf(g_html_file, "                \n");
    fprintf(g_html_file, "                // Create chart\n");
    fprintf(g_html_file, "                const ctx = document.getElementById('stressChart').getContext('2d');\n");
    fprintf(g_html_file, "                new Chart(ctx, {\n");
    fprintf(g_html_file, "                    type: 'line',\n");
    fprintf(g_html_file, "                    data: {\n");
    fprintf(g_html_file, "                        datasets: [{\n");
    fprintf(g_html_file, "                            label: 'CPU Usage (%)',\n");
    fprintf(g_html_file, "                            data: chartData,\n");
    fprintf(g_html_file, "                            borderColor: 'rgb(75, 192, 192)',\n");
    fprintf(g_html_file, "                            tension: 0.1,\n");
    fprintf(g_html_file, "                            fill: false\n");
    fprintf(g_html_file, "                        }]\n");
    fprintf(g_html_file, "                    },\n");
    fprintf(g_html_file, "                    options: {\n");
    fprintf(g_html_file, "                        responsive: true,\n");
    fprintf(g_html_file, "                        scales: {\n");
    fprintf(g_html_file, "                            y: {\n");
    fprintf(g_html_file, "                                beginAtZero: true,\n");
    fprintf(g_html_file, "                                max: 100\n");
    fprintf(g_html_file, "                            }\n");
    fprintf(g_html_file, "                        }\n");
    fprintf(g_html_file, "                    }\n");
    fprintf(g_html_file, "                });\n");
    fprintf(g_html_file, "                \n");
    fprintf(g_html_file, "            } catch (error) {\n");
    fprintf(g_html_file, "                console.error('Error loading data:', error);\n");
    fprintf(g_html_file, "                document.getElementById('stressChart').parentNode.innerHTML = '<p style=\"color: red;\">Error loading data: ' + error.message + '</p>';\n");
    fprintf(g_html_file, "            }\n");
    fprintf(g_html_file, "        }\n");
    fprintf(g_html_file, "        \n");
    fprintf(g_html_file, "        // Load data when page is ready\n");
    fprintf(g_html_file, "        document.addEventListener('DOMContentLoaded', loadData);\n");
    fprintf(g_html_file, "    </script>\n");
    fflush(g_html_file);
}