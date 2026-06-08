# PCM-3370 Stress Test Program

## Overview
Comprehensive stress testing utility for the PCM-3370 single-board computer running Windows XP 32-bit. Designed to validate system stability, CPU performance, memory subsystem, and cache behavior under continuous heavy load. Developed by SCTG Development for ISMO Group Ltd.

## Hardware Target
- **Motherboard**: Advantech PCM-3370 (PC/104 form factor)
- **CPU**: Intel Celeron 400MHz/650MHz or Pentium III (single-core)
- **Chipset**: VIA VT8606/TwiserT + VT82C686B
- **Memory**: Up to 512MB SDRAM
- **Storage**: CompactFlash (IDE interface, write-protected during stress test)
- **OS**: Windows XP 32-bit

## Features

### CPU Stress Testing
- Three specialized sub-tests:
  - Heavy floating-point operations (sqrt, log, atan2, fmod) targeting x87 FPU
  - Integer operations (multiply, divide, rotate) stressing ALU and divider unit
  - Branch prediction stress with data-dependent unpredictable branches
- Configurable iteration counts for each test type
- Volatile variables prevent compiler optimization

### Memory Validation
- Allocates and tests large memory blocks (up to 64MB with fallback)
- Eight test patterns including alternating nibbles/bytes and arbitrary values
- Walking-bit test for individual bit verification
- Prime-stride random access to stress DRAM row buffers
- Graceful fallback for memory allocation failures

### Cache Testing
- Cache line aligned memory access patterns (64-byte lines)
- Three access patterns to defeat cache policies:
  - Sequential forward access
  - Sequential reverse access (defeats hardware prefetcher)
  - Power-of-2 stride (creates set-associativity conflicts)
- Targets the 256KB/512KB L2 cache architecture

### System Monitoring
- Real CPU usage measurement via GetSystemTimes API
- Actual CPU temperature reporting via WMI ACPI thermal zones
- Iteration counter for progress tracking
- Console status updates at regular intervals (configurable)
- Memory usage statistics
- Performance rate tracking (iterations per minute)

## Build Requirements

### Toolchain
- LLVM-MinGW for Windows XP 32-bit (version 22.1.0)
- GNU Make 3.81 or compatible

### Dependencies
- Windows XP 32-bit API headers
- Standard C library (msvcrt)
- Windows Management Instrumentation (WMI) for temperature monitoring
- Required libraries: ole32, oleaut32, wbemuuid, user32, kernel32, advapi32

## Building the Project

1. **Prerequisites**:
   - Install LLVM-MinGW toolchain for Windows XP (version 22.1.0)
   - Ensure GNU Make is available in your PATH
   - Toolchain should be in `./llvm-mingw-xp-22.1.0-msvcrt-macos-universal/bin/`

2. **Build Command**:
   ```bash
   make
   ```

3. **Clean Build**:
   ```bash
   make clean
   ```

4. **Package for Distribution**:
   ```bash
   make package
   ```
   Creates a ZIP archive in the `dist/` directory

## Usage

1. Copy the `pcm3370_stress_test.exe` to your PCM-3370 system
2. Run from Windows XP command prompt:
   ```cmd
   pcm3370_stress_test.exe
   ```
3. The program will run indefinitely until manually terminated (Ctrl+C)

### Output Format
```
[Elapsed Time] [Iterations] [CPU Usage] [Temperature] [Free Memory] [Memory Errors] [Rate]
```
Example:
```
00:05:23  1000000  98.7%  45.6°C  256 MB    0          1200.5/min
```

## Output Interpretation

The program displays detailed status updates including:

- **Elapsed Time**: Test runtime in HH:MM:SS format
- **Iterations**: Number of completed test cycles
- **CPU%**: Actual CPU utilization percentage (measured via GetSystemTimes)
- **Temp**: Actual CPU temperature in Celsius (via WMI ACPI thermal zones)
- **FreeMem**: Available system memory in MB
- **MemErr**: Count of memory errors detected
- **Rate**: Test iteration rate in iterations per minute

### Header Information
The program starts by displaying system information including:
- Windows version and build number
- CPU architecture and page size
- Total and available memory
- Temperature sensor availability

## Technical Notes

### Windows XP Compatibility
- Compiled with `_WIN32_WINNT=0x0501` (Windows XP SP1)
- Uses Windows XP compatible Win32 API calls
- Statically linked for standalone operation
- No disk I/O operations (CompactFlash is write-protected)

### Performance Considerations
- Optimized for single-core Intel Celeron/Pentium III architecture
- Configurable test parameters:
  - Memory block size (64MB with fallback)
  - CPU test iteration counts
  - Cache line size (64 bytes)
  - Status update interval
- Memory tests adapt to available system memory
- Cache tests specifically target the 256KB/512KB L2 cache architecture

### Safety Features
- Memory allocation fallback mechanism (halves block size until successful)
- No disk I/O operations (safe for CompactFlash)
- Console-only output (no GUI dependencies)
- Graceful handling of WMI temperature sensor absence

### Implementation Details
- Uses COM object macros for C (no C++ dependencies)
- WMI session for persistent temperature monitoring
- CPU usage measured via GetSystemTimes API
- Memory tests use 8 distinct patterns for comprehensive validation
- Cache tests use three access patterns to defeat cache policies

## Project Structure

```
├── Makefile            # Build configuration with LLVM-MinGW settings
├── README.md           # This documentation
├── dist/               # Built executables and packages
│   ├── pcm3370_stress_test.exe
│   └── pcm3370_stress_test.zip
└── src/
    └── main.c          # Complete source code with all test implementations
```

## License
This project is provided as-is for hardware validation purposes. MIT License applies.

## Contributing
This is a specialized utility for specific hardware. Contributions would need to:
1. Maintain Windows XP 32-bit compatibility
2. Preserve the single-core optimization
3. Keep memory footprint appropriate for the target hardware
4. Maintain compatibility with the VIA chipset architecture

## Author
Ronan Le Meillat - SCTG Development for ISMO Group Ltd. - 2026
