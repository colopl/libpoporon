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
    poporon_t *pprn = poporon_create(
        8,      // symbol_size (bits per symbol)
        0x11D,  // generator_polynomial
        1,      // first_consecutive_root
        1,      // primitive_element
        32      // num_roots (parity symbols)
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
    poporon_encode_u8(pprn, data, sizeof(data), parity);

    // Simulate errors
    data[0] ^= 0xFF;
    data[10] ^= 0xAA;

    // Decode - correct errors
    size_t corrected_num = 0;
    if (poporon_decode_u8(pprn, data, sizeof(data), parity, &corrected_num)) {
        printf("Corrected %zu errors\n", corrected_num);
        printf("Decoded: %s\n", data);
    }

    poporon_destroy(pprn);
    return 0;
}
```

### BCH Encoding and Decoding

```c
#include <poporon/bch.h>
#include <stdio.h>

int main(void) {
    // Create BCH(15, k) with t=3 error correction capability
    poporon_bch_t *bch = poporon_bch_create(
        4,      // symbol_size (m, field GF(2^m))
        0x13,   // generator_polynomial
        3       // correction_capability (t errors)
    );
    if (!bch) {
        fprintf(stderr, "Failed to create BCH instance\n");
        return 1;
    }

    printf("Codeword length: %u\n", poporon_bch_get_codeword_length(bch));
    printf("Data length: %u\n", poporon_bch_get_data_length(bch));
    printf("Correction capability: %u errors\n", poporon_bch_get_correction_capability(bch));

    // Encode data
    uint32_t data = 21;
    uint32_t codeword;
    poporon_bch_encode(bch, data, &codeword);
    printf("Original codeword: 0x%04X\n", codeword);

    // Simulate 2 bit errors
    uint32_t corrupted = codeword ^ (1 << 3) ^ (1 << 7);
    printf("Corrupted: 0x%04X\n", corrupted);

    // Decode and correct
    uint32_t corrected;
    int32_t num_errors;
    if (poporon_bch_decode(bch, corrupted, &corrected, &num_errors)) {
        printf("Corrected %d errors: 0x%04X\n", num_errors, corrected);
        printf("Extracted data: %u\n", poporon_bch_extract_data(bch, corrected));
    }

    poporon_bch_destroy(bch);
    return 0;
}
```

### LDPC Encoding and Decoding

```c
#include <poporon/ldpc.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // Create LDPC encoder with rate 1/2 (50% redundancy)
    poporon_ldpc_t *ldpc = poporon_ldpc_create(
        128,                  // block_size (bytes, must be multiple of 4)
        PPRN_LDPC_RATE_1_2,   // code rate
        NULL                  // config (NULL for default)
    );
    if (!ldpc) {
        fprintf(stderr, "Failed to create LDPC instance\n");
        return 1;
    }

    size_t info_size = poporon_ldpc_info_size(ldpc);
    size_t parity_size = poporon_ldpc_parity_size(ldpc);
    size_t codeword_size = poporon_ldpc_codeword_size(ldpc);

    printf("Info size: %zu bytes\n", info_size);
    printf("Parity size: %zu bytes\n", parity_size);
    printf("Codeword size: %zu bytes\n", codeword_size);

    // Prepare buffers
    uint8_t *info = malloc(info_size);
    uint8_t *parity = malloc(parity_size);
    uint8_t *codeword = malloc(codeword_size);

    // Initialize data
    for (size_t i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    // Encode
    poporon_ldpc_encode(ldpc, info, parity);

    // Create codeword (info + parity)
    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    // Verify codeword is valid
    if (poporon_ldpc_check(ldpc, codeword)) {
        printf("Codeword is valid\n");
    }

    // Simulate errors
    codeword[0] ^= 0x01;
    codeword[10] ^= 0x80;
    codeword[20] ^= 0x40;

    // Decode with iterative belief propagation
    uint32_t iterations;
    if (poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations)) {
        printf("Decoded successfully in %u iterations\n", iterations);
    }

    free(info);
    free(parity);
    free(codeword);
    poporon_ldpc_destroy(ldpc);
    return 0;
}
```

### LDPC with Burst Error Resistance

```c
#include <poporon/ldpc.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // Configure burst-resistant LDPC
    poporon_ldpc_config_t config;
    poporon_ldpc_config_burst_resistant(&config);
    // config now has: column_weight=7, use_interleaver=true

    poporon_ldpc_t *ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);

    size_t info_size = poporon_ldpc_info_size(ldpc);
    size_t parity_size = poporon_ldpc_parity_size(ldpc);
    size_t codeword_size = poporon_ldpc_codeword_size(ldpc);

    uint8_t *info = malloc(info_size);
    uint8_t *parity = malloc(parity_size);
    uint8_t *codeword = malloc(codeword_size);
    uint8_t *interleaved = malloc(codeword_size);

    // Initialize and encode
    for (size_t i = 0; i < info_size; i++) {
        info[i] = (uint8_t)i;
    }
    poporon_ldpc_encode(ldpc, info, parity);

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    // Interleave before transmission
    poporon_ldpc_interleave(ldpc, codeword, interleaved);

    // Simulate burst error (consecutive bytes corrupted)
    for (size_t i = 40; i < 44; i++) {
        interleaved[i] ^= 0xFF;
    }

    // Decode (deinterleaving happens automatically)
    uint32_t iterations;
    if (poporon_ldpc_decode_hard(ldpc, interleaved, 100, &iterations)) {
        printf("Burst error corrected in %u iterations\n", iterations);
    }

    free(info);
    free(parity);
    free(codeword);
    free(interleaved);
    poporon_ldpc_destroy(ldpc);
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

### Reed-Solomon Types

```c
typedef struct _poporon_t poporon_t;         // Main Reed-Solomon codec
typedef struct _poporon_erasure_t poporon_erasure_t;  // Erasure position tracking
typedef struct _poporon_gf_t poporon_gf_t;   // Galois Field operations
typedef struct _poporon_rs_t poporon_rs_t;   // Reed-Solomon parameters
```

### Reed-Solomon Functions

```c
// Create/destroy codec
poporon_t *poporon_create(uint8_t symbol_size, uint16_t generator_polynomial,
                          uint16_t first_consecutive_root, uint16_t primitive_element,
                          uint8_t num_roots);
void poporon_destroy(poporon_t *poporon);

// Encode data
bool poporon_encode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity);

// Decode data
bool poporon_decode_u8(poporon_t *pprn, uint8_t *data, size_t size,
                       uint8_t *parity, size_t *corrected_num);
bool poporon_decode_u8_with_erasure(poporon_t *pprn, uint8_t *data, size_t size,
                                    uint8_t *parity, poporon_erasure_t *eras,
                                    size_t *corrected_num);
bool poporon_decode_u8_with_syndrome(poporon_t *pprn, uint8_t *data, uint8_t *parity,
                                     size_t size, uint16_t *syndrome,
                                     size_t *corrected_num);

// Utility
uint32_t poporon_version_id(void);
poporon_buildtime_t poporon_buildtime(void);
```

### BCH Functions

```c
// Create/destroy BCH codec
poporon_bch_t *poporon_bch_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                  uint8_t correction_capability);
void poporon_bch_destroy(poporon_bch_t *bch);

// Get parameters
uint16_t poporon_bch_get_codeword_length(const poporon_bch_t *bch);
uint16_t poporon_bch_get_data_length(const poporon_bch_t *bch);
uint8_t poporon_bch_get_correction_capability(const poporon_bch_t *bch);

// Encode/decode
bool poporon_bch_encode(poporon_bch_t *bch, uint32_t data, uint32_t *codeword);
bool poporon_bch_decode(poporon_bch_t *bch, uint32_t received,
                        uint32_t *corrected, int32_t *num_errors);
uint32_t poporon_bch_extract_data(const poporon_bch_t *bch, uint32_t codeword);
```

### LDPC Types and Constants

```c
typedef struct _poporon_ldpc_t poporon_ldpc_t;

// Code rates
typedef enum {
    PPRN_LDPC_RATE_1_2,  // 50% redundancy
    PPRN_LDPC_RATE_2_3,  // 33% redundancy
    PPRN_LDPC_RATE_3_4,  // 25% redundancy
    PPRN_LDPC_RATE_4_5,  // 20% redundancy
    PPRN_LDPC_RATE_5_6,  // 17% redundancy
} poporon_ldpc_rate_t;

// Matrix construction types
typedef enum {
    PPRN_LDPC_RANDOM,   // Random parity check matrix
    PPRN_LDPC_QC_RANDOM,   // Quasi-Cyclic with random shifts
} poporon_ldpc_matrix_type_t;

// Configuration
typedef struct {
    poporon_ldpc_matrix_type_t matrix_type;
    uint32_t column_weight;      // Density of parity matrix (3-8)
    bool use_interleaver;        // Enable interleaving for burst resistance
    uint32_t interleave_depth;   // Interleave depth (0 for auto)
    uint32_t lifting_factor;     // QC-LDPC lifting factor (0 for auto)
} poporon_ldpc_config_t;
```

### LDPC Functions

```c
// Configuration helpers
bool poporon_ldpc_config_default(poporon_ldpc_config_t *config);
bool poporon_ldpc_config_burst_resistant(poporon_ldpc_config_t *config);

// Create/destroy
poporon_ldpc_t *poporon_ldpc_create(size_t block_size, poporon_ldpc_rate_t rate,
                                    const poporon_ldpc_config_t *config);
void poporon_ldpc_destroy(poporon_ldpc_t *ldpc);

// Get sizes
size_t poporon_ldpc_info_size(const poporon_ldpc_t *ldpc);
size_t poporon_ldpc_codeword_size(const poporon_ldpc_t *ldpc);
size_t poporon_ldpc_parity_size(const poporon_ldpc_t *ldpc);

// Encode
bool poporon_ldpc_encode(poporon_ldpc_t *ldpc, const uint8_t *info, uint8_t *parity);

// Decode
bool poporon_ldpc_decode_hard(poporon_ldpc_t *ldpc, uint8_t *codeword,
                              uint32_t max_iterations, uint32_t *iterations_used);
bool poporon_ldpc_decode_soft(poporon_ldpc_t *ldpc, const int8_t *llr,
                              uint8_t *codeword, uint32_t max_iterations,
                              uint32_t *iterations_used);

// Validation
bool poporon_ldpc_check(const poporon_ldpc_t *ldpc, const uint8_t *codeword);

// Interleaving
bool poporon_ldpc_has_interleaver(const poporon_ldpc_t *ldpc);
bool poporon_ldpc_interleave(const poporon_ldpc_t *ldpc,
                             const uint8_t *input, uint8_t *output);
bool poporon_ldpc_deinterleave(const poporon_ldpc_t *ldpc,
                               const uint8_t *input, uint8_t *output);
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
| 1/2 | 50% | 32 - 8192 bytes |
| 2/3 | 33% | 32 - 8192 bytes |
| 3/4 | 25% | 32 - 8192 bytes |
| 4/5 | 20% | 32 - 8192 bytes |
| 5/6 | 17% | 32 - 8192 bytes |

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
│   ├── poporon.h          # Main public header
│   └── poporon/
│       ├── bch.h          # BCH codec API
│       ├── erasure.h      # Erasure API
│       ├── gf.h           # Galois Field API
│       ├── ldpc.h         # LDPC codec API
│       ├── rng.h          # Random number generator API
│       └── rs.h           # Reed-Solomon API
├── src/
│   ├── bch.c              # BCH implementation
│   ├── encode.c           # RS encoding implementation
│   ├── decode.c           # RS decoding with Berlekamp-Massey
│   ├── erasure.c          # Erasure handling
│   ├── gf.c               # Galois Field implementation
│   ├── ldpc.c             # LDPC implementation
│   ├── rng.c              # Xoshiro128++ RNG
│   ├── rs.c               # Reed-Solomon core
│   ├── poporon.c          # Main API implementation
│   └── internal/
│       ├── common.h       # Internal types and macros
│       ├── ldpc.h         # LDPC internal structures
│       └── simd.h         # SIMD abstractions
├── tests/                 # Test suite using Unity
│   ├── test_basic.c       # Basic functionality tests
│   ├── test_bch.c         # BCH tests
│   ├── test_codec.c       # Codec tests
│   ├── test_erasure.c     # Erasure tests
│   ├── test_gf.c          # Galois Field tests
│   ├── test_ldpc.c        # LDPC tests
│   ├── test_rng.c         # RNG tests
│   └── test_rs.c          # Reed-Solomon tests
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
