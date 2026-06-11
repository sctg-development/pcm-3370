/******************************************************************************
 * Temperature Monitoring Module Implementation
 *****************************************************************************/

#include <windows.h>
#define COBJMACROS

#include <stdio.h>
#include <objbase.h>
#include <wbemidl.h>
#include "temperature.h"

/* ---- WMI session for temperature ----------------------------------------- */
static IWbemServices *g_wmi_svc = NULL;
int g_wmi_ready = 0;

/* ---- VIA VT82C686B hardware monitor ------------------------------------ */
typedef struct {
    ULONGLONG BusAddress; /* I/O port address (32-bit port in 64-bit field) */
    PVOID     Buffer;     /* receives the byte/word/dword                   */
    ULONG     Request;    /* transfer width in bytes: 1, 2, or 4            */
    ULONG     Completed;  /* bytes transferred (set by kernel)              */
} SYSDBG_IO_SPACE;

typedef LONG (NTAPI *PNtSystemDebugControl)(
    ULONG  Command,
    PVOID  InputBuffer,
    ULONG  InputLength,
    PVOID  OutputBuffer,
    ULONG  OutputLength,
    PULONG ReturnLength
);

static PNtSystemDebugControl g_NtSysDbg = NULL;
int g_via686b_ready = 0;

/* 0 = not probed, 1 = NtSystemDebugControl, 2 = direct IN instruction */
static int g_io_access_mode = 0;

/******************************************************************************
 * WMI temperature: open one persistent session to ROOT\WMI
 *****************************************************************************/
int init_temperature(void)
{
    IWbemLocator *pLoc = NULL;
    HRESULT hr;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
        return 0;

    /* Ignore error: may already have been called */
    CoInitializeSecurity(NULL, -1, NULL, NULL,
                         RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE,
                         NULL, EOAC_NONE, NULL);

    hr = CoCreateInstance(&CLSID_WbemLocator, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWbemLocator, (void **)&pLoc);
    if (FAILED(hr))
    {
        CoUninitialize();
        return 0;
    }

    hr = IWbemLocator_ConnectServer(pLoc,
                                    L"ROOT\\WMI", NULL, NULL, 0, 0, 0, 0, &g_wmi_svc);
    IWbemLocator_Release(pLoc);
    if (FAILED(hr))
    {
        CoUninitialize();
        return 0;
    }

    hr = CoSetProxyBlanket((IUnknown *)g_wmi_svc,
                           RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
                           RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE,
                           NULL, EOAC_NONE);
    if (FAILED(hr))
    {
        IWbemServices_Release(g_wmi_svc);
        g_wmi_svc = NULL;
        CoUninitialize();
        return 0;
    }

    g_wmi_ready = 1;
    return 1;
}

/******************************************************************************
 * Read CPU temperature from WMI ACPI thermal zone
 *****************************************************************************/
static double get_wmi_temperature(void)
{
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG returned = 0;
    VARIANT vt;
    double temp = -1.0;
    HRESULT hr;

    if (!g_wmi_ready || !g_wmi_svc)
        return -1.0;

    hr = IWbemServices_ExecQuery(g_wmi_svc,
                                 L"WQL",
                                 L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature",
                                 WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                                 NULL, &pEnum);
    if (FAILED(hr))
        return -1.0;

    if (IEnumWbemClassObject_Next(pEnum, WBEM_INFINITE, 1, &pObj, &returned) == S_OK && returned > 0)
    {
        VariantInit(&vt);
        hr = IWbemClassObject_Get(pObj, L"CurrentTemperature", 0, &vt, NULL, NULL);
        if (SUCCEEDED(hr))
        {
            /* WMI reports tenths of Kelvin; convert to Celsius */
            if (vt.vt == VT_I4)
                temp = (double)vt.lVal / 10.0 - 273.15;
            else if (vt.vt == VT_UI4)
                temp = (double)vt.ulVal / 10.0 - 273.15;
        }
        VariantClear(&vt);
        IWbemClassObject_Release(pObj);
    }
    IEnumWbemClassObject_Release(pEnum);
    return temp;
}

/******************************************************************************
 * VIA VT82C686B hardware monitor — direct ISA I/O via NtSystemDebugControl
 *****************************************************************************/
void acquire_debug_privilege(void)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return;
    if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid))
    {
        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    }
    CloseHandle(hToken);
}

/* Returns TRUE on success, FALSE if the NtSystemDebugControl call itself fails */
static BOOL read_io_byte_nt(ULONG port, BYTE *out)
{
    SYSDBG_IO_SPACE ios;
    ULONG ret = 0;
    LONG status;

    *out = 0;
    ios.BusAddress = (ULONGLONG)port;
    ios.Buffer     = out;
    ios.Request    = 1;
    ios.Completed  = 0;
    status = g_NtSysDbg(SYSDBG_READ_IO, &ios, sizeof(ios), NULL, 0, &ret);
    return (BOOL)(status >= 0); /* NT_SUCCESS */
}

static BYTE read_via686b(ULONG port)
{
    BYTE val = 0xFF;
    if (g_io_access_mode == 1)
        read_io_byte_nt(port, &val);
    return val;
}

int init_via686b(void)
{
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    BYTE probe;

    if (hNtdll)
        g_NtSysDbg = (PNtSystemDebugControl)
            GetProcAddress(hNtdll, "NtSystemDebugControl");

    if (!g_NtSysDbg)
        return 0;

    acquire_debug_privilege();

    /*
     * Probe via NtSystemDebugControl: success is determined by the NTSTATUS
     * return value, not the register content.  A valid sensor can legitimately
     * read 0xFF (-1 °C signed), so the old "if (probe == 0xFF) fail" logic was
     * wrong — it rejected the sensor whenever NtSystemDebugControl itself
     * failed (leaving val at 0xFF) or at cold temperatures.
     */
    if (!read_io_byte_nt(VIA686B_BASE + VIA686B_REG_TEMP, &probe))
        return 0;

    g_io_access_mode = 1;
    g_via686b_ready  = 1;
    return 1;
}

static double get_via686b_temperature(void)
{
    if (!g_via686b_ready)
        return -1.0;
    /* VIA 686B stores temperature as 8-bit two's-complement Celsius */
    return (double)(signed char)read_via686b(VIA686B_BASE + VIA686B_REG_TEMP);
}

/******************************************************************************
 * Get CPU temperature - tries WMI first, falls back to VIA 686B
 *****************************************************************************/
double get_cpu_temperature(void)
{
    double temp = get_wmi_temperature();
    if (temp < 0.0)
        temp = get_via686b_temperature();
    return temp;
}