# libpoporon

[![CI](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Polynomial Operations Providing Optimal Reed-Solomon Organized Numerics**

[🇯🇵 日本語版 README はこちら](README_ja.md)

libpoporon is a lightweight, high-performance forward error correction (FEC) library written in C99. It provides multiple error correction algorithms including Reed-Solomon, BCH, and LDPC codes, with optional SIMD acceleration for maximum performance.

## Features

- **Pure C99 Implementation** - No external dependencies, portable across platforms
- **Multiple FEC Algorithms** - Reed-Solomon, BCH (Bose-Chaudhuri-Hocquenghem), and LDPC (Low-Density Parity-Check) codes
- **SIMD Acceleration** - Automatic optimization using AVX2 (x86_64), NEON (ARM64), or WASM SIMD128
- **Erasure Decoding** - Support for Reed-Solomon error correction with known error positions
- **Soft Decision Decoding** - LDPC supports both hard and soft decision decoding with LLR input
- **Burst Error Resistance** - LDPC includes interleaver support for improved burst error correction
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
| `POPORON_USE_ASAN` | `OFF` | Enable AddressSanitizer |
| `POPORON_USE_MSAN` | `OFF` | Enable MemorySanitizer |
| `POPORON_USE_UBSAN` | `OFF` | Enable UndefinedBehaviorSanitizer |

### Running Tests

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DPOPORON_USE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Usage Examples

### Reed-Solomon Encoding and Decoding

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // RS(255, 223) - 32 parity symbols, can correct up to 16 errors
    poporon_config_t *config = poporon_config_rs_default();
    poporon_t *pprn = poporon_create(config);
    if (!pprn) {
        fprintf(stderr, "Failed to create poporon instance\n");
        poporon_config_destroy(config);
        return 1;
    }

    // Prepare data
    uint8_t data[64];
    uint8_t parity[32];
    memset(data, 0, sizeof(data));
    memcpy(data, "Hello, Reed-Solomon!", 20);

    // Encode - generate parity
    poporon_encode(pprn, data, sizeof(data), parity);

    // Simulate errors
    data[0] ^= 0xFF;
    data[10] ^= 0xAA;

    // Decode - correct errors
    size_t corrected_num = 0;
    if (poporon_decode(pprn, data, sizeof(data), parity, &corrected_num)) {
        printf("Corrected %zu errors\n", corrected_num);
        printf("Decoded: %s\n", data);
    }

    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### BCH Encoding and Decoding

```c
#include <poporon.h>
#include <stdio.h>

int main(void) {
    // Create BCH(15, 5) with t=3 error correction capability
    poporon_config_t *config = poporon_config_bch_default();
    poporon_t *pprn = poporon_create(config);
    if (!pprn) {
        fprintf(stderr, "Failed to create BCH instance\n");
        poporon_config_destroy(config);
        return 1;
    }

    printf("FEC type: %d\n", poporon_get_fec_type(pprn));
    printf("Parity size: %zu bytes\n", poporon_get_parity_size(pprn));
    printf("Info size: %zu bytes\n", poporon_get_info_size(pprn));

    // Encode data
    uint8_t data[1] = {21};
    uint8_t parity[4];
    poporon_encode(pprn, data, 1, parity);

    // Simulate bit errors in data
    data[0] ^= 0x0A;

    // Decode and correct
    size_t corrected = 0;
    if (poporon_decode(pprn, data, 1, parity, &corrected)) {
        printf("Corrected %zu errors\n", corrected);
        printf("Recovered data: %u\n", data[0]);
    }

    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### LDPC Encoding and Decoding

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // Create LDPC encoder with rate 1/2 (100% redundancy)
    poporon_config_t *config = poporon_config_ldpc_default(128, PPRN_LDPC_RATE_1_2);
    poporon_t *pprn = poporon_create(config);
    if (!pprn) {
        fprintf(stderr, "Failed to create LDPC instance\n");
        poporon_config_destroy(config);
        return 1;
    }

    size_t info_size = poporon_get_info_size(pprn);
    size_t parity_size = poporon_get_parity_size(pprn);

    printf("Info size: %zu bytes\n", info_size);
    printf("Parity size: %zu bytes\n", parity_size);

    // Prepare buffers
    uint8_t *data = malloc(info_size);
    uint8_t *parity = malloc(parity_size);

    // Initialize data
    for (size_t i = 0; i < info_size; i++) {
        data[i] = (uint8_t)(i * 17 + 23);
    }

    // Encode
    poporon_encode(pprn, data, 128, parity);

    // Simulate errors
    data[0] ^= 0x01;
    data[10] ^= 0x80;
    data[20] ^= 0x40;

    // Decode with iterative belief propagation
    size_t corrected = 0;
    if (poporon_decode(pprn, data, 128, parity, &corrected)) {
        uint32_t iterations = poporon_get_iterations_used(pprn);
        printf("Decoded successfully in %u iterations\n", iterations);
    }

    free(data);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### LDPC with Burst Error Resistance

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // Create burst-resistant LDPC with a single convenience function
    poporon_config_t *config = poporon_config_ldpc_burst_resistant(128, PPRN_LDPC_RATE_1_2);
    poporon_t *pprn = poporon_create(config);

    size_t info_size = poporon_get_info_size(pprn);
    size_t parity_size = poporon_get_parity_size(pprn);

    uint8_t *data = malloc(info_size);
    uint8_t *parity = malloc(parity_size);

    // Initialize and encode
    for (size_t i = 0; i < info_size; i++) {
        data[i] = (uint8_t)i;
    }
    poporon_encode(pprn, data, 128, parity);

    // Simulate burst error (consecutive bytes corrupted)
    for (size_t i = 10; i < 14; i++) {
        data[i] ^= 0xFF;
    }

    // Decode — interleaving/deinterleaving is handled automatically
    size_t corrected = 0;
    if (poporon_decode(pprn, data, 128, parity, &corrected)) {
        uint32_t iterations = poporon_get_iterations_used(pprn);
        printf("Burst error corrected in %u iterations\n", iterations);
    }

    free(data);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### Random Number Generator

```c
#include <poporon/rng.h>
#include <stdio.h>

int main(void) {
    // Create Xoshiro128++ RNG with seed
    uint32_t seed = 12345;
    poporon_rng_t *rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));

    // Generate random bytes
    uint8_t buffer[16];
    poporon_rng_next(rng, buffer, sizeof(buffer));

    printf("Random bytes: ");
    for (size_t i = 0; i < sizeof(buffer); i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");

    poporon_rng_destroy(rng);
    return 0;
}
```

## API Reference

### Core Types

```c
typedef struct _poporon_t poporon_t;                 // Unified FEC codec handle
typedef struct _poporon_config_t poporon_config_t;   // Opaque configuration object
typedef struct _poporon_erasure_t poporon_erasure_t; // Erasure position tracking
typedef struct _poporon_gf_t poporon_gf_t;           // Galois Field operations
typedef uint32_t poporon_buildtime_t;

// FEC algorithm type
typedef enum {
    PPLN_FEC_RS      = 1,    // Reed-Solomon
    PPLN_FEC_LDPC    = 2,    // Low-Density Parity-Check
    PPLN_FEC_BCH     = 3,    // Bose-Chaudhuri-Hocquenghem
    PPLN_FEC_UNKNOWN = 255,
} poporon_fec_type_t;
```

### LDPC Types and Constants

```c
// Code rates
typedef enum {
    PPRN_LDPC_RATE_1_3,  // 200% redundancy
    PPRN_LDPC_RATE_1_2,  // 100% redundancy
    PPRN_LDPC_RATE_2_3,  // 50% redundancy
    PPRN_LDPC_RATE_3_4,  // 33% redundancy
    PPRN_LDPC_RATE_4_5,  // 25% redundancy
    PPRN_LDPC_RATE_5_6,  // 20% redundancy
} poporon_ldpc_rate_t;

// Matrix construction types
typedef enum {
    PPRN_LDPC_RANDOM,     // Random parity check matrix
    PPRN_LDPC_QC_RANDOM,  // Quasi-Cyclic with random shifts
} poporon_ldpc_matrix_type_t;
```

### Configuration Functions

```c
// Reed-Solomon configuration
poporon_config_t *poporon_rs_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                           uint16_t first_consecutive_root, uint16_t primitive_element,
                                           uint8_t num_roots, poporon_erasure_t *erasure,
                                           uint16_t *syndrome);
poporon_config_t *poporon_config_rs_default(void);  // RS(255, 223), 32 parity symbols

// LDPC configuration
poporon_config_t *poporon_ldpc_config_create(size_t block_size, poporon_ldpc_rate_t rate,
                                             poporon_ldpc_matrix_type_t matrix_type,
                                             uint32_t column_weight, bool use_soft_decode,
                                             bool use_outer_interleave, bool use_inner_interleave,
                                             uint32_t interleave_depth, uint32_t lifting_factor,
                                             uint32_t max_iterations, const int8_t *soft_llr,
                                             size_t soft_llr_size, uint64_t seed);
poporon_config_t *poporon_config_ldpc_default(size_t block_size, poporon_ldpc_rate_t rate);
poporon_config_t *poporon_config_ldpc_burst_resistant(size_t block_size, poporon_ldpc_rate_t rate);

// BCH configuration
poporon_config_t *poporon_bch_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                            uint8_t correction_capability);
poporon_config_t *poporon_config_bch_default(void);  // BCH(15, 5), t=3

// Destroy configuration (safe to call after poporon_create)
void poporon_config_destroy(poporon_config_t *config);
```

### Codec Functions

```c
// Create/destroy codec
poporon_t *poporon_create(const poporon_config_t *config);
void poporon_destroy(poporon_t *pprn);

// Encode data
bool poporon_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity);

// Decode data
bool poporon_decode(poporon_t *pprn, uint8_t *data, size_t size,
                    uint8_t *parity, size_t *corrected_num);
```

### Query Functions

```c
poporon_fec_type_t poporon_get_fec_type(const poporon_t *pprn);
size_t poporon_get_parity_size(const poporon_t *pprn);
size_t poporon_get_info_size(const poporon_t *pprn);
uint32_t poporon_get_iterations_used(const poporon_t *pprn);  // LDPC only (0 for RS/BCH)
```

### Utility Functions

```c
uint32_t poporon_version_id(void);
poporon_buildtime_t poporon_buildtime(void);
```

### Erasure API

```c
poporon_erasure_t *poporon_erasure_create(uint16_t num_roots, uint32_t initial_capacity);
poporon_erasure_t *poporon_erasure_create_from_positions(uint16_t num_roots,
                                                         const uint32_t *erasure_positions,
                                                         uint32_t erasure_count);
bool poporon_erasure_add_position(poporon_erasure_t *erasure, uint32_t position);
void poporon_erasure_reset(poporon_erasure_t *erasure);
void poporon_erasure_destroy(poporon_erasure_t *eras);
```

### Galois Field API

```c
poporon_gf_t *poporon_gf_create(uint8_t symbol_size, uint16_t generator_polynomial);
void poporon_gf_destroy(poporon_gf_t *gf);
uint8_t poporon_gf_mod(poporon_gf_t *gf, uint16_t value);
```

### RNG API

```c
typedef enum {
    XOSHIRO128PP  // Xoshiro128++ algorithm
} poporon_rng_type_t;

poporon_rng_t *poporon_rng_create(poporon_rng_type_t type, void *seed, size_t seed_size);
void poporon_rng_destroy(poporon_rng_t *rng);
bool poporon_rng_next(poporon_rng_t *rng, void *dest, size_t size);
```

## Algorithm Comparison

| Algorithm | Type | Best For | Correction Capability |
|-----------|------|----------|----------------------|
| Reed-Solomon | Block code | Burst errors, storage | Up to `num_roots/2` symbol errors |
| BCH | Binary block code | Random bit errors | Up to `t` bit errors |
| LDPC | Sparse graph code | Near Shannon limit, soft decoding | Iterative, depends on rate |

## Common Configurations

### Reed-Solomon

| Name | Parameters | Correction |
|------|------------|------------|
| RS(255, 223) | symbol_size=8, gen_poly=0x11D, num_roots=32 | 16 symbols |
| RS(255, 239) | symbol_size=8, gen_poly=0x11D, num_roots=16 | 8 symbols |
| RS(255, 247) | symbol_size=8, gen_poly=0x11D, num_roots=8 | 4 symbols |

### BCH

| Name | Parameters | Correction |
|------|------------|------------|
| BCH(15, 5) | symbol_size=4, gen_poly=0x13, t=3 | 3 bits |
| BCH(31, 21) | symbol_size=5, gen_poly=0x25, t=2 | 2 bits |
| BCH(63, 51) | symbol_size=6, gen_poly=0x43, t=2 | 2 bits |

### LDPC

| Rate | Redundancy | Block Sizes |
|------|------------|-------------|
| 1/3 | 200% | 32 - 8192 bytes |
| 1/2 | 100% | 32 - 8192 bytes |
| 2/3 | 50% | 32 - 8192 bytes |
| 3/4 | 33% | 32 - 8192 bytes |
| 4/5 | 25% | 32 - 8192 bytes |
| 5/6 | 20% | 32 - 8192 bytes |

## SIMD Support

The library automatically detects and enables SIMD optimizations based on the target architecture:

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

To disable SIMD optimizations:
```bash
cmake -B build -DPOPORON_USE_SIMD=OFF
```

## Code Coverage

To generate coverage reports (requires GCC, `lcov`, and `genhtml`):

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DPOPORON_USE_TESTS=ON \
               -DPOPORON_USE_COVERAGE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Integration

### Using CMake `add_subdirectory`

Add libpoporon as a subdirectory in your project:

```cmake
add_subdirectory(path/to/libpoporon)
target_link_libraries(your_target PRIVATE poporon)
```

### Manual Integration

Include the headers and link the static library:

```cmake
target_include_directories(your_target PRIVATE path/to/libpoporon/include)
target_link_libraries(your_target PRIVATE path/to/libpoporon/build/libpoporon.a)
```

## Project Structure

```
libpoporon/
├── include/
│   ├── poporon.h          # Main public header (unified API)
│   └── poporon/
│       ├── erasure.h      # Erasure API
│       ├── gf.h           # Galois Field API
│       └── rng.h          # Random number generator API
├── src/
│   ├── bch.c              # BCH implementation
│   ├── encode.c           # Encoding implementation
│   ├── decode.c           # Decoding with Berlekamp-Massey
│   ├── erasure.c          # Erasure handling
│   ├── gf.c               # Galois Field implementation
│   ├── ldpc.c             # LDPC implementation
│   ├── rng.c              # Xoshiro128++ RNG
│   ├── rs.c               # Reed-Solomon core
│   ├── poporon.c          # Unified API implementation
│   └── internal/
│       ├── common.h       # Internal types and macros
│       ├── config.h       # Configuration internals
│       ├── ldpc.h         # LDPC internal structures
│       └── simd.h         # SIMD abstractions
├── tests/                 # Test suite using Unity
│   ├── test_basic.c       # Basic functionality tests
│   ├── test_bch.c         # BCH tests
│   ├── test_codec.c       # Codec tests
│   ├── test_erasure.c     # Erasure tests
│   ├── test_gf.c          # Galois Field tests
│   ├── test_invalid.c     # Invalid input tests
│   ├── test_ldpc.c        # LDPC tests
│   ├── test_rng.c         # RNG tests
│   ├── test_rs.c          # Reed-Solomon tests
│   ├── test_unified.c     # Unified API tests
│   ├── fec_compat.c       # FEC compatibility tests
│   └── util.h             # Test utilities
├── cmake/                 # CMake modules
│   ├── buildtime.cmake    # Build timestamp
│   ├── emscripten.cmake   # WebAssembly support
│   └── test.cmake         # Test configuration
└── third_party/           # Dependencies
    ├── emsdk/             # Emscripten SDK (optional)
    ├── unity/             # Unity Test framework
    └── valgrind/          # Valgrind headers
```

## Dependencies

- **[Unity](https://github.com/ThrowTheSwitch/Unity)** - Unit testing framework (submodule, tests only)

## License

MIT License - see [LICENSE](LICENSE) for details.

## Author

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)
