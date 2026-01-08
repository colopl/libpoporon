# libpoporon

[![CI](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Polynomial Operations Providing Optimal Reed-Solomon Organized Numerics**

[🇯🇵 日本語版 README はこちら](README_ja.md)

libpoporon is a lightweight, high-performance Reed-Solomon error correction library written in C99. It provides encoding and decoding capabilities with optional SIMD acceleration for maximum performance.

## Features

- **Pure C99 Implementation** - No external dependencies, portable across platforms
- **SIMD Acceleration** - Automatic optimization using AVX2 (x86_64), NEON (ARM64), or WASM SIMD128
- **Erasure Decoding** - Support for error correction with known error positions
- **WebAssembly Support** - Can be compiled to WASM using Emscripten
- **Memory Safe** - Carefully designed API with proper resource management
- **Extensive Testing** - Comprehensive test suite with sanitizer and Valgrind support

## Quick Start

### Building

```bash
# Clone the repository
git clone https://github.com/zeriyoshi/libpoporon.git
cd libpoporon

# Build with CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `POPORON_USE_SIMD` | `ON` | Enable SIMD optimizations |
| `POPORON_USE_TESTS` | `OFF` | Build test suite |
| `POPORON_USE_VALGRIND` | `OFF` | Enable Valgrind memory checking |
| `POPORON_USE_COVERAGE` | `OFF` | Enable code coverage |

### Running Tests

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DPOPORON_USE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage Example

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // RS(255, 223) - 32 parity symbols, can correct up to 16 errors
    const uint8_t symbol_size = 8;
    const uint16_t generator_polynomial = 0x11D;
    const uint16_t first_consecutive_root = 1;
    const uint16_t primitive_element = 1;
    const uint8_t num_roots = 32;

    // Create Reed-Solomon codec
    poporon_t *pprn = poporon_create(
        symbol_size,
        generator_polynomial,
        first_consecutive_root,
        primitive_element,
        num_roots
    );
    if (!pprn) {
        fprintf(stderr, "Failed to create poporon instance\n");
        return 1;
    }

    // Prepare data
    uint8_t data[64];
    uint8_t parity[32];
    memset(data, 0, sizeof(data));
    memcpy(data, "Hello, Reed-Solomon!", 20);

    // Encode - generate parity
    if (!poporon_encode_u8(pprn, data, sizeof(data), parity)) {
        fprintf(stderr, "Encoding failed\n");
        poporon_destroy(pprn);
        return 1;
    }

    // Simulate errors
    data[0] ^= 0xFF;
    data[10] ^= 0xAA;

    // Decode - correct errors
    size_t corrected_num = 0;
    if (poporon_decode_u8(pprn, data, sizeof(data), parity, &corrected_num)) {
        printf("Corrected %zu errors\n", corrected_num);
        printf("Decoded: %s\n", data);
    } else {
        fprintf(stderr, "Decoding failed - too many errors\n");
    }

    poporon_destroy(pprn);
    return 0;
}
```

## API Reference

### Core Types

```c
typedef struct _poporon_t poporon_t;         // Main Reed-Solomon codec
typedef struct _poporon_erasure_t poporon_erasure_t;  // Erasure position tracking
typedef struct _poporon_gf_t poporon_gf_t;   // Galois Field operations
typedef struct _poporon_rs_t poporon_rs_t;   // Reed-Solomon parameters
```

### Main Functions

#### `poporon_create`

```c
poporon_t *poporon_create(
    uint8_t symbol_size,           // Bits per symbol (typically 8)
    uint16_t generator_polynomial, // Field generator polynomial
    uint16_t first_consecutive_root,
    uint16_t primitive_element,
    uint8_t num_roots              // Number of parity symbols (error correction capacity = num_roots / 2)
);
```

Creates a new Reed-Solomon codec instance.

#### `poporon_destroy`

```c
void poporon_destroy(poporon_t *poporon);
```

Frees all resources associated with the codec.

#### `poporon_encode_u8`

```c
bool poporon_encode_u8(
    poporon_t *pprn,
    uint8_t *data,     // Input data
    size_t size,       // Data size in bytes
    uint8_t *parity    // Output: parity symbols (must be num_roots bytes)
);
```

Encodes data and generates parity symbols.

#### `poporon_decode_u8`

```c
bool poporon_decode_u8(
    poporon_t *pprn,
    uint8_t *data,         // Data to decode (modified in-place)
    size_t size,           // Data size
    uint8_t *parity,       // Parity symbols
    size_t *corrected_num  // Output: number of corrections made
);
```

Decodes and corrects errors in data.

#### `poporon_decode_u8_with_erasure`

```c
bool poporon_decode_u8_with_erasure(
    poporon_t *pprn,
    uint8_t *data,
    size_t size,
    uint8_t *parity,
    poporon_erasure_t *eras,  // Known error positions
    size_t *corrected_num
);
```

Decodes with known erasure positions for improved correction capability.

### Erasure API

```c
// Create erasure tracker
poporon_erasure_t *poporon_erasure_create(
    uint16_t num_roots,
    uint32_t initial_capacity
);

// Create from known positions
poporon_erasure_t *poporon_erasure_create_from_positions(
    uint16_t num_roots,
    const uint32_t *erasure_positions,
    uint32_t erasure_count
);

// Add an erasure position
bool poporon_erasure_add_position(poporon_erasure_t *erasure, uint32_t position);

// Reset erasure list
void poporon_erasure_reset(poporon_erasure_t *erasure);

// Free erasure tracker
void poporon_erasure_destroy(poporon_erasure_t *eras);
```

### Galois Field API

```c
// Create GF(2^symbol_size) field
poporon_gf_t *poporon_gf_create(uint8_t symbol_size, uint16_t generator_polynomial);

// Free GF instance
void poporon_gf_destroy(poporon_gf_t *gf);

// Compute modular reduction
uint8_t poporon_gf_mod(poporon_gf_t *gf, uint16_t value);
```

### Utility Functions

```c
// Get library version as integer
uint32_t poporon_version_id(void);

// Get library build timestamp
poporon_buildtime_t poporon_buildtime(void);
```

## Common Reed-Solomon Configurations

| Name | Parameters | Correction Capability |
|------|------------|----------------------|
| RS(255, 223) | symbol_size=8, gen_poly=0x11D, num_roots=32 | 16 errors |
| RS(255, 239) | symbol_size=8, gen_poly=0x11D, num_roots=16 | 8 errors |
| RS(255, 247) | symbol_size=8, gen_poly=0x11D, num_roots=8 | 4 errors |

## Platform Support

| Platform | SIMD | Status |
|----------|------|--------|
| Linux x86_64 | AVX2 | ✅ Fully supported |
| Linux ARM64 | NEON | ✅ Fully supported |
| Linux i386 | None | ✅ Supported (scalar) |
| Linux s390x | None | ✅ Supported (scalar) |
| macOS x86_64 | AVX2 | ✅ Fully supported |
| macOS ARM64 | NEON | ✅ Fully supported |
| Windows x86_64 | AVX2 | ✅ Fully supported |
| WebAssembly | SIMD128 | ✅ Fully supported |

## Project Structure

```
libpoporon/
├── include/
│   ├── poporon.h          # Main public header
│   └── poporon/
│       ├── erasure.h      # Erasure API
│       ├── gf.h           # Galois Field API
│       └── rs.h           # Reed-Solomon API
├── src/
│   ├── encode.c           # Encoding implementation
│   ├── decode.c           # Decoding with Berlekamp-Massey
│   ├── erasure.c          # Erasure handling
│   ├── gf.c               # Galois Field implementation
│   ├── rs.c               # Reed-Solomon core
│   ├── poporon.c          # Main API implementation
│   └── internal/
│       ├── common.h       # Internal types and macros
│       └── simd.h         # SIMD abstractions
├── tests/                 # Test suite using Unity
├── cmake/                 # CMake modules
└── third_party/           # Dependencies (Unity, Emscripten SDK)
```

## License

MIT License - see [LICENSE](LICENSE) for details.

## Author

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)
