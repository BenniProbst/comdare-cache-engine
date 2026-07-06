# comdare-simd

**Foundation BASE** | SIMD Operations Module

## Overview

SIMD-accelerated operations with runtime dispatch for Scalar, SSE4.2, AVX2, and AVX-512.

## Features

- **SIMDOps**: Vectorized arithmetic, bitwise, and memory operations
- **SIMDDispatcher**: Runtime SIMD level selection based on CPU capabilities
- **Fallback**: Automatic scalar fallback when SIMD unavailable

## Dependencies

- comdare-platform (for CPU detection)

## Usage

```cpp
#include <comdare/simd/SIMDOps.hpp>
#include <comdare/simd/SIMDDispatcher.hpp>

// Initialize dispatcher (once at startup)
comdare::simd::Dispatcher::init();

// Use dispatched operations
uint64_t carry = comdare::simd::dispatch::add(a, b, result, count);
```

## Supported Operations

- `add_u64`: Addition with carry
- `sub_u64`: Subtraction with borrow
- `compare_u64`: Lexicographic comparison
- `mul_u32`: Schoolbook multiplication
- `bitwise_and/or/xor/not`: Bitwise operations
- `popcount`, `clz`, `ctz`: Bit counting
- `find_nonzero`, `find_byte`: Search operations

## License

Proprietary - BEP Venture UG
