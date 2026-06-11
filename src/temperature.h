/******************************************************************************
 * Temperature Monitoring Module
 *
 * VIA VT82C686B hardware monitor accessed via direct ISA I/O.
 * Requires PortTalk.sys (installed by AIDA64, manual start).
 *
 * Copyright (c) 2026 Ronan LE MEILLAT - SCTG Development for ISMO Group Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#ifndef TEMPERATURE_H
#define TEMPERATURE_H

#include <windows.h>

/* ISA base address of the VIA VT82C686B hardware monitor (confirmed by AIDA64) */
#define VIA686B_BASE      0x6000u

/* PortTalk v2.2 (Craig Peacock) IOCTL codes.
 * CTL_CODE(PORTTALK_TYPE=0x9C40, func, METHOD_BUFFERED=0, FILE_ANY_ACCESS=0)
 * = (0x9C40<<16) | (func<<2)                                               */
#define IOCTL_READ_PORT_UCHAR   0x9C402410UL   /* func=0x904 */
#define IOCTL_WRITE_PORT_UCHAR  0x9C402414UL   /* func=0x905 */

/* Module state (set by init_via686b, read by display/stress loops) */
extern int g_wmi_ready;
extern int g_via686b_ready;
extern int g_verbose;

/* Lifecycle */
int  init_temperature(void);
int  init_via686b(void);
void cleanup_via686b(void);

/* Temperature readings in degrees Celsius.
 * Returns -1.0 if hardware access is not available. */
double get_cpu_temperature(void);
double get_mb_temperature(void);
double get_aux_temperature(void);

#endif /* TEMPERATURE_H */
