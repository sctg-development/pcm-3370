# PCM-3370 Stress Test Program

## Overview
Comprehensive stress testing utility for the PCM-3370 single-board computer running Windows XP 32-bit. Designed to validate system stability, CPU performance, memory subsystem, and cache behavior under continuous heavy load.

## Hardware Target
- **Motherboard**: Advantech PCM-3370 (PC/104 form factor)
- **CPU**: Intel Celeron 400MHz/650MHz or Pentium III (single-core)
- **Chipset**: VIA VT8606/TwiserT + VT82C686B
- **Memory**: Up to 512MB SDRAM
- **Storage**: CompactFlash (IDE interface)
- **OS**: Windows XP 32-bit

## Features

### CPU Stress Testing
- Mixed workload of floating-point and integer operations
- Heavy arithmetic computations to maximize CPU utilization
- Designed for single-core optimization
- Volatile variables prevent compiler optimization

### Memory Validation
- Allocates and tests large memory blocks (up to 64MB)
- Pattern filling and verification
- Random access patterns to test memory subsystem
- Graceful fallback for memory allocation failures

### Cache Testing
- Cache line aligned memory access patterns
- Strided access to create cache conflicts
- Tests both sequential and random access patterns
- Targets the 256KB/512KB L2 cache architecture

### System Monitoring
- Simulated temperature reporting
- CPU usage estimation
- Iteration counter for progress tracking
- Console status updates at regular intervals

## Build Requirements

### Toolchain
- LLVM-MinGW targeting Windows XP 32-bit
- GNU Make 3.81 or compatible

### Dependencies
- Windows XP 32-bit API headers
- Standard C library (msvcrt)
- No external dependencies required

## Building the Project

1. **Prerequisites**:
   - Install LLVM-MinGW toolchain for Windows XP
   - Ensure GNU Make is available in your PATH

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

## Usage

1. Copy the `pcm3370_stress_test.exe` to your PCM-3370 system
2. Run from Windows XP command prompt:
   ```cmd
   pcm3370_stress_test.exe
   ```
3. The program will run indefinitely until manually terminated (Ctrl+C)

## Output Interpretation

The program displays status updates in the following format:
```
[Iteration: 1000000] CPU: 95.2% | Temp: 45.6°C | Memory: 64MB tested
```

- **Iteration**: Number of completed test cycles
- **CPU**: Simulated CPU utilization percentage
- **Temp**: Simulated temperature (for demonstration)
- **Memory**: Size of memory block being tested

## Technical Notes

### Windows XP Compatibility
- Compiled with `_WIN32_WINNT=0x0501` (Windows XP SP1)
- Uses only Windows XP compatible Win32 API calls
- Statically linked for standalone operation

### Performance Considerations
- Optimized for single-core Intel Celeron/Pentium III
- Memory tests adapt to available system memory
- Cache tests target the specific cache architecture

### Safety Features
- Memory allocation fallback mechanism
- No disk I/O operations (safe for CompactFlash)
- Console-only output (no GUI dependencies)

## Project Structure

```
├── Makefile            # Build configuration
├── README.md           # This documentation
├── dist/               # Built executables and packages
│   ├── pcm3370_stress_test.exe
│   └── pcm3370_stress_test.zip
└── src/
    └── main.c          # Source code
```

## License
This project is provided as-is for hardware validation purposes. MIT License applies.

## Contributing
This is a specialized utility for specific hardware. Contributions would need to:
1. Maintain Windows XP 32-bit compatibility
2. Preserve the single-core optimization
3. Keep memory footprint appropriate for the target hardware

## Author
Cline - 2026