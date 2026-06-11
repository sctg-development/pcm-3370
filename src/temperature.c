/******************************************************************************
 * VIA VT82C686B Hardware Monitor — Direct ISA I/O via PortTalk.sys
 *
 * Access method:
 *   PortTalk.sys modifies the IOPB in the
 *   TSS so that IN/OUT instructions execute in ring 3 without faulting.
 *   After the IOCTL, inline asm inb/outb work directly.
 *
 * IOCTL codes (both tried in order):
 *   0x9C402003  METHOD_NEITHER  — original Allen Denver PortTalk
 *   0x9C402000  METHOD_BUFFERED — some re-packaged variants
 *   CTL_CODE(FILE_DEVICE_PORTTALK=0x9C40, func=0x800, method, FILE_ANY_ACCESS)
 *
 * Register map — identical to Linux via686a.c via686a_update_device():
 *   reg 0x20  CPU temp high byte   + 0x4B bits 7:6 (mask 0xC0, shift 6)
 *   reg 0x21  MB  temp high byte   + 0x49 bits 5:4 (mask 0x30, shift 4)
 *   reg 0x1F  Aux temp high byte   + 0x49 bits 7:6 (mask 0xC0, shift 6)
 *   reg 0x40  Config (bit0=start, bit7=0)
 *   reg 0x4B  Temp mode (bits 5:0=0 for continuous) / CPU frac (bits 7:6)
 *
 * Verified against AIDA64 Debug T raw bytes (0x85=Aux, 0xD9=CPU, 0xC6=MB):
 *   lut[0x85=133]=259 -> 25.9 C  (AIDA64 Aux=25 C)
 *   lut[0xD9=217]=759 -> 75.9 C  (AIDA64 CPU=73 C)
 *   lut[0xC6=198]=592 -> 59.2 C  (AIDA64 MB =57 C)
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

#include "temperature.h"
#include <stdio.h>

/* log_message() is defined in main.c */
extern void log_message(const char *format, ...);

#define VLOG(...) do { if (g_verbose) log_message("[VIA686B] " __VA_ARGS__); } while (0)

/* --- ISA register offsets from VIA686B_BASE --------------------------------
 * Source: Linux via686a.c via686a_update_device()                          */
#define REG_TEMP_CPU        0x20u  /* temp[0] high byte                      */
#define REG_TEMP_MB         0x21u  /* temp[1] high byte                      */
#define REG_TEMP_AUX        0x1Fu  /* temp[2] high byte                      */
#define REG_TEMP_LOW_CPU    0x4Bu  /* CPU  frac: bits 7:6 (mask 0xC0, >>6)   */
#define REG_TEMP_LOW_MB_AUX 0x49u  /* MB   frac: bits 5:4 (mask 0x30, >>4)   */
                                   /* Aux  frac: bits 7:6 (mask 0xC0, >>6)   */
#define REG_CONFIG          0x40u
#define REG_TEMP_MODE       0x4Bu  /* same address as REG_TEMP_LOW_CPU        */

/* --- Module state ---------------------------------------------------------- */
int g_wmi_ready      = 0;
int g_via686b_ready  = 0;

static HANDLE g_hPortTalk = INVALID_HANDLE_VALUE;

/* --- Temperature lookup table (Linux via686a.c, temp_lut[256]) -------------
 * Index = raw 8-bit register value; value = temperature in 0.1 C units.
 * Formula: 5th-order poly fit to calibration data (see Linux driver comment). */
static const short temp_lut[256] = {
    -709, -688, -667, -646, -627, -607, -589, -570, -553, -536, -519,
    -503, -487, -471, -456, -442, -428, -414, -400, -387, -375,
    -362, -350, -339, -327, -316, -305, -295, -285, -275, -265,
    -255, -246, -237, -229, -220, -212, -204, -196, -188, -180,
    -173, -166, -159, -152, -145, -139, -132, -126, -120, -114,
    -108, -102,  -96,  -91,  -85,  -80,  -74,  -69,  -64,  -59,  -54,  -49,
     -44,  -39,  -34,  -29,  -25,  -20,  -15,  -11,   -6,   -2,    3,    7,
      12,   16,   20,   25,   29,   33,   37,   42,   46,   50,   54,   59,
      63,   67,   71,   75,   79,   84,   88,   92,   96,  100,  104,  109,
     113,  117,  121,  125,  130,  134,  138,  142,  146,  151,  155,  159,
     163,  168,  172,  176,  181,  185,  189,  193,  198,  202,  206,  211,
     215,  219,  224,  228,  232,  237,  241,  245,  250,  254,  259,  263,
     267,  272,  276,  281,  285,  290,  294,  299,  303,  307,  312,  316,
     321,  325,  330,  334,  339,  344,  348,  353,  357,  362,  366,  371,
     376,  380,  385,  390,  395,  399,  404,  409,  414,  419,  423,  428,
     433,  438,  443,  449,  454,  459,  464,  469,  475,  480,  486,  491,
     497,  502,  508,  514,  520,  526,  532,  538,  544,  551,  557,  564,
     571,  578,  584,  592,  599,  606,  614,  621,  629,  637,  645,  654,
     662,  671,  680,  689,  698,  708,  718,  728,  738,  749,  759,  770,
     782,  793,  805,  818,  830,  843,  856,  870,  883,  898,  912,  927,
     943,  958,  975,  991, 1008, 1026, 1044, 1062, 1081, 1101, 1121, 1141,
    1162, 1184, 1206, 1229, 1252, 1276, 1301, 1326, 1352, 1378, 1406, 1434,
    1462
};

/* --- Low-level I/O --------------------------------------------------------- */

/* I/O via PortTalk v2.2 DeviceIoControl (Craig Peacock driver).
 * The driver executes IN/OUT in kernel and returns the result — no IOPB patch. */
static BYTE via686b_inb(WORD port)
{
    BYTE  ret = 0;
    DWORD nBytes;
    DeviceIoControl(g_hPortTalk, IOCTL_READ_PORT_UCHAR,
                    &port, sizeof(port),
                    &ret,  sizeof(ret),
                    &nBytes, NULL);
    return ret;
}

static void via686b_outb(WORD port, BYTE val)
{
    BYTE  buf[3];
    DWORD nBytes;
    *(WORD *)&buf[0] = port;
    buf[2] = val;
    DeviceIoControl(g_hPortTalk, IOCTL_WRITE_PORT_UCHAR,
                    buf, sizeof(buf),
                    NULL, 0,
                    &nBytes, NULL);
}

/* --- Temperature conversion ------------------------------------------------
 * Mirrors TEMP_FROM_REG10() in Linux via686a.c exactly:
 *   val10 bits 9:2 = 8-bit LUT index, bits 1:0 = sub-resolution fraction.
 *   Linear interpolation between adjacent LUT entries for frac != 0.       */
static double temp_from_reg10(unsigned short val10, int do_log)
{
    unsigned char idx  = (unsigned char)(val10 >> 2);
    unsigned char frac = (unsigned char)(val10 & 3u);
    double result;

    if (frac == 0 || idx == 255) {
        result = temp_lut[idx] / 10.0;
        if (do_log)
            VLOG("  lut[%u]=%d (frac=0) -> %.2f C\n", idx, temp_lut[idx], result);
    } else {
        result = (temp_lut[idx] * (4 - frac) +
                  temp_lut[(unsigned char)(idx + 1u)] * frac) * 0.025;
        if (do_log)
            VLOG("  lut[%u]=%d lut[%u]=%d frac=%u -> %.2f C\n",
                 idx, temp_lut[idx],
                 (unsigned char)(idx + 1u), temp_lut[(unsigned char)(idx + 1u)],
                 frac, result);
    }
    return result;
}

/* Read one temperature sensor, following via686a_update_device() exactly:
 *   temp = inb(base + reg_high) << 2
 *   temp |= (inb(base + reg_low) & mask) >> shift                          */
static double read_via686b_temp(const char *name,
                                BYTE reg_high, BYTE reg_low, int low_shift)
{
    WORD  port_hi = (WORD)(VIA686B_BASE + reg_high);
    WORD  port_lo = (WORD)(VIA686B_BASE + reg_low);
    BYTE  mask    = (low_shift == 6) ? 0xC0u : 0x30u;
    BYTE  hi      = via686b_inb(port_hi);
    BYTE  lo      = via686b_inb(port_lo);
    BYTE  frac    = (BYTE)((lo & mask) >> low_shift);
    unsigned short val10 = (unsigned short)((hi << 2u) | frac);

    if (g_verbose)
        VLOG("%s: reg[0x%04X]=0x%02X  reg[0x%04X]=0x%02X"
             "  mask=0x%02X shift=%d  frac=%u  val10=0x%03X\n",
             name, port_hi, hi, port_lo, lo, mask, low_shift, frac, val10);

    return temp_from_reg10(val10, g_verbose);
}

/* --- PortTalk management --------------------------------------------------- */

static int try_porttalk(void)
{
    VLOG("Opening \\\\.\\PortTalk...\n");
    g_hPortTalk = CreateFileA("\\\\.\\PortTalk",
                               GENERIC_READ | GENERIC_WRITE,
                               0, NULL, OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL, NULL);

    if (g_hPortTalk == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        VLOG("Open failed (error %lu) — trying StartService\n", err);

        {
            SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
            if (hSCM) {
                SC_HANDLE hSvc = OpenServiceA(hSCM, "PortTalk", SERVICE_START);
                if (hSvc) {
                    if (StartServiceA(hSvc, 0, NULL))
                        VLOG("StartService(PortTalk) OK\n");
                    else
                        VLOG("StartService(PortTalk) failed (error %lu)\n",
                             GetLastError());
                    CloseServiceHandle(hSvc);
                } else {
                    VLOG("OpenService(PortTalk) failed (error %lu)\n",
                         GetLastError());
                }
                CloseServiceHandle(hSCM);
            }
        }

        g_hPortTalk = CreateFileA("\\\\.\\PortTalk",
                                   GENERIC_READ | GENERIC_WRITE,
                                   0, NULL, OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL, NULL);
        if (g_hPortTalk == INVALID_HANDLE_VALUE) {
            VLOG("PortTalk still unavailable (error %lu)\n", GetLastError());
            return 0;
        }
    }

    VLOG("PortTalk device opened\n");
    return 1;
}

/* --- Public API ------------------------------------------------------------ */

int init_via686b(void)
{
    BYTE cfg;

    VLOG("init_via686b: ISA base=0x%04X\n", VIA686B_BASE);

    if (!try_porttalk()) {
        log_message("[VIA686B] PortTalk.sys not accessible\n");
        return 0;
    }

    /* Enable monitoring if not already running (REG_CONFIG bit 0) */
    cfg = via686b_inb((WORD)(VIA686B_BASE + REG_CONFIG));
    VLOG("REG_CONFIG (0x%04X+0x%02X) = 0x%02X\n", VIA686B_BASE, REG_CONFIG, cfg);
    if (!(cfg & 0x01u)) {
        BYTE new_cfg = (BYTE)((cfg | 0x01u) & 0x7Fu);
        via686b_outb((WORD)(VIA686B_BASE + REG_CONFIG), new_cfg);
        VLOG("REG_CONFIG written 0x%02X (monitoring started)\n", new_cfg);
    } else {
        VLOG("REG_CONFIG bit0 set (monitoring already active)\n");
    }

    /* Set continuous temperature conversion (clear bits 5:0 of REG_TEMP_MODE) */
    cfg = via686b_inb((WORD)(VIA686B_BASE + REG_TEMP_MODE));
    VLOG("REG_TEMP_MODE (0x%04X+0x%02X) = 0x%02X\n",
         VIA686B_BASE, REG_TEMP_MODE, cfg);
    {
        BYTE new_mode = (BYTE)(cfg & 0xC0u);
        via686b_outb((WORD)(VIA686B_BASE + REG_TEMP_MODE), new_mode);
        VLOG("REG_TEMP_MODE written 0x%02X (continuous)\n", new_mode);
    }

    g_via686b_ready = 1;
    log_message("[VIA686B] Init OK — ISA 0x%04X, PortTalk.sys\n", VIA686B_BASE);
    return 1;
}

int init_temperature(void)
{
    return init_via686b();
}

void cleanup_via686b(void)
{
    if (g_hPortTalk != INVALID_HANDLE_VALUE) {
        CloseHandle(g_hPortTalk);
        g_hPortTalk = INVALID_HANDLE_VALUE;
    }
    g_via686b_ready = 0;
}

double get_cpu_temperature(void)
{
    if (!g_via686b_ready) return -1.0;
    /* temp[0]: high=0x20, frac=0x4B bits 7:6 (mask 0xC0, shift 6) */
    return read_via686b_temp("CPU", REG_TEMP_CPU, REG_TEMP_LOW_CPU, 6);
}

double get_mb_temperature(void)
{
    if (!g_via686b_ready) return -1.0;
    /* temp[1]: high=0x21, frac=0x49 bits 5:4 (mask 0x30, shift 4) */
    return read_via686b_temp("MB",  REG_TEMP_MB,  REG_TEMP_LOW_MB_AUX, 4);
}

double get_aux_temperature(void)
{
    if (!g_via686b_ready) return -1.0;
    /* temp[2]: high=0x1F, frac=0x49 bits 7:6 (mask 0xC0, shift 6) */
    return read_via686b_temp("Aux", REG_TEMP_AUX, REG_TEMP_LOW_MB_AUX, 6);
}
