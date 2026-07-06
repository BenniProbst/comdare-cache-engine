# comdare-platform

**Foundation BASE** | Platform Abstraction Module

## Overview

Platform-specific abstractions for CPU detection, asynchronous I/O, and OS-level APIs.

## Features

- **SIMDDetect**: CPUID-based CPU feature detection (SSE, AVX, AVX-512, BMI, ADX)
- **CPUInfo**: Cache sizes, core counts, vendor/brand strings
- **AsyncIO**: Platform-abstracted async I/O (io_uring on Linux, IOCP on Windows)
- **OSAbstraction**: Memory mapping, file operations, process info

## Dependencies

None (Foundation module)

## Usage

```cpp
#include <comdare/platform/SIMDDetect.hpp>
#include <comdare/platform/AsyncIO.hpp>

// CPU Feature Detection
const auto& features = comdare::platform::SIMDDetector::detect();
if (features.avx2) {
    // Use AVX2 code path
}

// Best SIMD level
auto level = comdare::platform::SIMDDetector::best_level();
```

## License

Proprietary - BEP Venture UG
