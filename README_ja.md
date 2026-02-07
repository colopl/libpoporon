# libpoporon

[![CI](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Polynomial Operations Providing Optimal Reed-Solomon Organized Numerics**

[🇬🇧 English README](README.md)

libpoporon は、C99 で書かれた軽量・高性能な前方誤り訂正（FEC）ライブラリです。Reed-Solomon、BCH、LDPC 符号を含む複数の誤り訂正アルゴリズムを提供し、オプションの SIMD アクセラレーションにより最大限のパフォーマンスを実現します。

## 特徴

- **純粋な C99 実装** - 外部依存なし、プラットフォーム間で移植可能
- **複数の FEC アルゴリズム** - Reed-Solomon、BCH（ボーズ・チョードリ・ホッケンゲム）、LDPC（低密度パリティ検査）符号
- **SIMD 高速化** - AVX2（x86_64）、NEON（ARM64）、WASM SIMD128 による自動最適化
- **イレージャー復号** - 既知のエラー位置による Reed-Solomon 誤り訂正をサポート
- **軟判定復号** - LDPC は LLR 入力による硬判定・軟判定復号の両方をサポート
- **バースト誤り耐性** - LDPC はバースト誤り訂正を改善するインターリーバをサポート
- **WebAssembly 対応** - Emscripten を使用して WASM にコンパイル可能
- **メモリ安全** - 適切なリソース管理を備えた慎重に設計された API
- **包括的なテスト** - サニタイザと Valgrind サポートを含む広範なテストスイート

## クイックスタート

### ビルド

```bash
# リポジトリをクローン
git clone https://github.com/zeriyoshi/libpoporon.git
cd libpoporon

# CMake でビルド
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### ビルドオプション

| オプション | デフォルト | 説明 |
|--------|---------|-------------|
| `POPORON_USE_SIMD` | `ON` | SIMD 最適化を有効化 |
| `POPORON_USE_TESTS` | `OFF` | テストスイートをビルド |
| `POPORON_USE_VALGRIND` | `OFF` | Valgrind メモリチェックを有効化 |
| `POPORON_USE_COVERAGE` | `OFF` | コードカバレッジを有効化 |
| `POPORON_USE_ASAN` | `OFF` | AddressSanitizer を有効化 |
| `POPORON_USE_MSAN` | `OFF` | MemorySanitizer を有効化 |
| `POPORON_USE_UBSAN` | `OFF` | UndefinedBehaviorSanitizer を有効化 |

### テストの実行

```bash
git submodule update --init --recursive
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DPOPORON_USE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 使用例

### Reed-Solomon エンコードとデコード

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // RS(255, 223) - 32 パリティシンボル、最大 16 エラーを訂正可能
    poporon_config_t *config = poporon_config_rs_default();
    poporon_t *pprn = poporon_create(config);
    if (!pprn) {
        fprintf(stderr, "poporon インスタンスの作成に失敗しました\n");
        poporon_config_destroy(config);
        return 1;
    }

    // データを準備
    uint8_t data[64];
    uint8_t parity[32];
    memset(data, 0, sizeof(data));
    memcpy(data, "Hello, Reed-Solomon!", 20);

    // エンコード - パリティを生成
    poporon_encode(pprn, data, sizeof(data), parity);

    // エラーをシミュレート
    data[0] ^= 0xFF;
    data[10] ^= 0xAA;

    // デコード - エラーを訂正
    size_t corrected_num = 0;
    if (poporon_decode(pprn, data, sizeof(data), parity, &corrected_num)) {
        printf("%zu 個のエラーを訂正しました\n", corrected_num);
        printf("デコード結果: %s\n", data);
    }

    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### BCH エンコードとデコード

```c
#include <poporon.h>
#include <stdio.h>

int main(void) {
    // t=3 の誤り訂正能力を持つ BCH(15, 5) を作成
    poporon_config_t *config = poporon_config_bch_default();
    poporon_t *pprn = poporon_create(config);
    if (!pprn) {
        fprintf(stderr, "BCH インスタンスの作成に失敗しました\n");
        poporon_config_destroy(config);
        return 1;
    }

    printf("FEC タイプ: %d\n", poporon_get_fec_type(pprn));
    printf("パリティサイズ: %zu バイト\n", poporon_get_parity_size(pprn));
    printf("情報サイズ: %zu バイト\n", poporon_get_info_size(pprn));

    // データをエンコード
    uint8_t data[1] = {21};
    uint8_t parity[4];
    poporon_encode(pprn, data, 1, parity);

    // ビットエラーをシミュレート
    data[0] ^= 0x0A;

    // デコードして訂正
    size_t corrected = 0;
    if (poporon_decode(pprn, data, 1, parity, &corrected)) {
        printf("%zu 個のエラーを訂正\n", corrected);
        printf("復元データ: %u\n", data[0]);
    }

    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### LDPC エンコードとデコード

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // レート 1/2（100% 冗長）の LDPC エンコーダを作成
    poporon_config_t *config = poporon_config_ldpc_default(128, PPRN_LDPC_RATE_1_2);
    poporon_t *pprn = poporon_create(config);
    if (!pprn) {
        fprintf(stderr, "LDPC インスタンスの作成に失敗しました\n");
        poporon_config_destroy(config);
        return 1;
    }

    size_t info_size = poporon_get_info_size(pprn);
    size_t parity_size = poporon_get_parity_size(pprn);

    printf("情報サイズ: %zu バイト\n", info_size);
    printf("パリティサイズ: %zu バイト\n", parity_size);

    // バッファを準備
    uint8_t *data = malloc(info_size);
    uint8_t *parity = malloc(parity_size);

    // データを初期化
    for (size_t i = 0; i < info_size; i++) {
        data[i] = (uint8_t)(i * 17 + 23);
    }

    // エンコード
    poporon_encode(pprn, data, 128, parity);

    // エラーをシミュレート
    data[0] ^= 0x01;
    data[10] ^= 0x80;
    data[20] ^= 0x40;

    // 反復ビリーフプロパゲーションでデコード
    size_t corrected = 0;
    if (poporon_decode(pprn, data, 128, parity, &corrected)) {
        uint32_t iterations = poporon_get_iterations_used(pprn);
        printf("%u 回の反復でデコード成功\n", iterations);
    }

    free(data);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### バースト誤り耐性を持つ LDPC

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // コンビニエンス関数でバースト誤り耐性 LDPC を作成
    poporon_config_t *config = poporon_config_ldpc_burst_resistant(128, PPRN_LDPC_RATE_1_2);
    poporon_t *pprn = poporon_create(config);

    size_t info_size = poporon_get_info_size(pprn);
    size_t parity_size = poporon_get_parity_size(pprn);

    uint8_t *data = malloc(info_size);
    uint8_t *parity = malloc(parity_size);

    // 初期化とエンコード
    for (size_t i = 0; i < info_size; i++) {
        data[i] = (uint8_t)i;
    }
    poporon_encode(pprn, data, 128, parity);

    // バースト誤りをシミュレート（連続バイトの破損）
    for (size_t i = 10; i < 14; i++) {
        data[i] ^= 0xFF;
    }

    // デコード — インターリーブ/デインターリーブは自動的に処理される
    size_t corrected = 0;
    if (poporon_decode(pprn, data, 128, parity, &corrected)) {
        uint32_t iterations = poporon_get_iterations_used(pprn);
        printf("バースト誤りを %u 回の反復で訂正\n", iterations);
    }

    free(data);
    free(parity);
    poporon_destroy(pprn);
    poporon_config_destroy(config);
    return 0;
}
```

### 乱数生成器

```c
#include <poporon/rng.h>
#include <stdio.h>

int main(void) {
    // シード付きで Xoshiro128++ RNG を作成
    uint32_t seed = 12345;
    poporon_rng_t *rng = poporon_rng_create(XOSHIRO128PP, &seed, sizeof(seed));

    // ランダムバイトを生成
    uint8_t buffer[16];
    poporon_rng_next(rng, buffer, sizeof(buffer));

    printf("ランダムバイト: ");
    for (size_t i = 0; i < sizeof(buffer); i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");

    poporon_rng_destroy(rng);
    return 0;
}
```

## API リファレンス

### コア型

```c
typedef struct _poporon_t poporon_t;                 // 統合 FEC コーデックハンドル
typedef struct _poporon_config_t poporon_config_t;   // 不透明な設定オブジェクト
typedef struct _poporon_erasure_t poporon_erasure_t; // イレージャー位置の追跡
typedef struct _poporon_gf_t poporon_gf_t;           // ガロア体演算
typedef uint32_t poporon_buildtime_t;

// FEC アルゴリズムタイプ
typedef enum {
    PPLN_FEC_RS      = 1,    // Reed-Solomon
    PPLN_FEC_LDPC    = 2,    // 低密度パリティ検査
    PPLN_FEC_BCH     = 3,    // ボーズ・チョードリ・ホッケンゲム
    PPLN_FEC_UNKNOWN = 255,
} poporon_fec_type_t;
```

### LDPC 型と定数

```c
// コードレート
typedef enum {
    PPRN_LDPC_RATE_1_3,  // 200% 冗長
    PPRN_LDPC_RATE_1_2,  // 100% 冗長
    PPRN_LDPC_RATE_2_3,  // 50% 冗長
    PPRN_LDPC_RATE_3_4,  // 33% 冗長
    PPRN_LDPC_RATE_4_5,  // 25% 冗長
    PPRN_LDPC_RATE_5_6,  // 20% 冗長
} poporon_ldpc_rate_t;

// 行列構成タイプ
typedef enum {
    PPRN_LDPC_RANDOM,     // ランダムパリティ検査行列
    PPRN_LDPC_QC_RANDOM,  // ランダムシフトを用いた準巡回構造
} poporon_ldpc_matrix_type_t;
```

### 設定関数

```c
// Reed-Solomon 設定
poporon_config_t *poporon_rs_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                           uint16_t first_consecutive_root, uint16_t primitive_element,
                                           uint8_t num_roots, poporon_erasure_t *erasure,
                                           uint16_t *syndrome);
poporon_config_t *poporon_config_rs_default(void);  // RS(255, 223)、32 パリティシンボル

// LDPC 設定
poporon_config_t *poporon_ldpc_config_create(size_t block_size, poporon_ldpc_rate_t rate,
                                             poporon_ldpc_matrix_type_t matrix_type,
                                             uint32_t column_weight, bool use_soft_decode,
                                             bool use_outer_interleave, bool use_inner_interleave,
                                             uint32_t interleave_depth, uint32_t lifting_factor,
                                             uint32_t max_iterations, const int8_t *soft_llr,
                                             size_t soft_llr_size, uint64_t seed);
poporon_config_t *poporon_config_ldpc_default(size_t block_size, poporon_ldpc_rate_t rate);
poporon_config_t *poporon_config_ldpc_burst_resistant(size_t block_size, poporon_ldpc_rate_t rate);

// BCH 設定
poporon_config_t *poporon_bch_config_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                            uint8_t correction_capability);
poporon_config_t *poporon_config_bch_default(void);  // BCH(15, 5)、t=3

// 設定を破棄（poporon_create 後に呼び出しても安全）
void poporon_config_destroy(poporon_config_t *config);
```

### コーデック関数

```c
// コーデックの作成/破棄
poporon_t *poporon_create(const poporon_config_t *config);
void poporon_destroy(poporon_t *pprn);

// データをエンコード
bool poporon_encode(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity);

// データをデコード
bool poporon_decode(poporon_t *pprn, uint8_t *data, size_t size,
                    uint8_t *parity, size_t *corrected_num);
```

### クエリ関数

```c
poporon_fec_type_t poporon_get_fec_type(const poporon_t *pprn);
size_t poporon_get_parity_size(const poporon_t *pprn);
size_t poporon_get_info_size(const poporon_t *pprn);
uint32_t poporon_get_iterations_used(const poporon_t *pprn);  // LDPC 専用（RS/BCH は 0）
```

### ユーティリティ関数

```c
uint32_t poporon_version_id(void);
poporon_buildtime_t poporon_buildtime(void);
```

### イレージャー API

```c
poporon_erasure_t *poporon_erasure_create(uint16_t num_roots, uint32_t initial_capacity);
poporon_erasure_t *poporon_erasure_create_from_positions(uint16_t num_roots,
                                                         const uint32_t *erasure_positions,
                                                         uint32_t erasure_count);
bool poporon_erasure_add_position(poporon_erasure_t *erasure, uint32_t position);
void poporon_erasure_reset(poporon_erasure_t *erasure);
void poporon_erasure_destroy(poporon_erasure_t *eras);
```

### ガロア体 API

```c
poporon_gf_t *poporon_gf_create(uint8_t symbol_size, uint16_t generator_polynomial);
void poporon_gf_destroy(poporon_gf_t *gf);
uint8_t poporon_gf_mod(poporon_gf_t *gf, uint16_t value);
```

### RNG API

```c
typedef enum {
    XOSHIRO128PP  // Xoshiro128++ アルゴリズム
} poporon_rng_type_t;

poporon_rng_t *poporon_rng_create(poporon_rng_type_t type, void *seed, size_t seed_size);
void poporon_rng_destroy(poporon_rng_t *rng);
bool poporon_rng_next(poporon_rng_t *rng, void *dest, size_t size);
```

## アルゴリズム比較

| アルゴリズム | タイプ | 適した用途 | 訂正能力 |
|-----------|------|----------|----------------------|
| Reed-Solomon | ブロック符号 | バースト誤り、ストレージ | 最大 `num_roots/2` シンボル誤り |
| BCH | 2 値ブロック符号 | ランダムビット誤り | 最大 `t` ビット誤り |
| LDPC | スパースグラフ符号 | シャノン限界近傍、軟判定復号 | 反復的、レートに依存 |

## 一般的な設定

### Reed-Solomon

| 名前 | パラメータ | 訂正能力 |
|------|------------|------------|
| RS(255, 223) | symbol_size=8, gen_poly=0x11D, num_roots=32 | 16 シンボル |
| RS(255, 239) | symbol_size=8, gen_poly=0x11D, num_roots=16 | 8 シンボル |
| RS(255, 247) | symbol_size=8, gen_poly=0x11D, num_roots=8 | 4 シンボル |

### BCH

| 名前 | パラメータ | 訂正能力 |
|------|------------|------------|
| BCH(15, 5) | symbol_size=4, gen_poly=0x13, t=3 | 3 ビット |
| BCH(31, 21) | symbol_size=5, gen_poly=0x25, t=2 | 2 ビット |
| BCH(63, 51) | symbol_size=6, gen_poly=0x43, t=2 | 2 ビット |

### LDPC

| レート | 冗長度 | ブロックサイズ |
|------|------------|-------------|
| 1/3 | 200% | 32 - 8192 バイト |
| 1/2 | 100% | 32 - 8192 バイト |
| 2/3 | 50% | 32 - 8192 バイト |
| 3/4 | 33% | 32 - 8192 バイト |
| 4/5 | 25% | 32 - 8192 バイト |
| 5/6 | 20% | 32 - 8192 バイト |

## SIMD サポート

ライブラリはターゲットアーキテクチャに基づいて SIMD 最適化を自動的に検出し有効化します：

| プラットフォーム | SIMD | 状態 |
|----------|------|--------|
| Linux x86_64 | AVX2 | ✅ 完全サポート |
| Linux ARM64 | NEON | ✅ 完全サポート |
| Linux i386 | なし | ✅ サポート（スカラー） |
| Linux s390x | なし | ✅ サポート（スカラー） |
| macOS x86_64 | AVX2 | ✅ 完全サポート |
| macOS ARM64 | NEON | ✅ 完全サポート |
| Windows x86_64 | AVX2 | ✅ 完全サポート |
| WebAssembly | SIMD128 | ✅ 完全サポート |

SIMD 最適化を無効にするには：
```bash
cmake -B build -DPOPORON_USE_SIMD=OFF
```

## コードカバレッジ

カバレッジレポートを生成するには（GCC、`lcov`、`genhtml` が必要）：

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug \
               -DPOPORON_USE_TESTS=ON \
               -DPOPORON_USE_COVERAGE=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 統合

### CMake `add_subdirectory` を使用

プロジェクトに libpoporon をサブディレクトリとして追加：

```cmake
add_subdirectory(path/to/libpoporon)
target_link_libraries(your_target PRIVATE poporon)
```

### 手動統合

ヘッダーをインクルードして静的ライブラリをリンク：

```cmake
target_include_directories(your_target PRIVATE path/to/libpoporon/include)
target_link_libraries(your_target PRIVATE path/to/libpoporon/build/libpoporon.a)
```

## プロジェクト構成

```
libpoporon/
├── include/
│   ├── poporon.h          # メイン公開ヘッダー（統合 API）
│   └── poporon/
│       ├── erasure.h      # イレージャー API
│       ├── gf.h           # ガロア体 API
│       └── rng.h          # 乱数生成器 API
├── src/
│   ├── bch.c              # BCH 実装
│   ├── encode.c           # エンコード実装
│   ├── decode.c           # Berlekamp-Massey によるデコード
│   ├── erasure.c          # イレージャー処理
│   ├── gf.c               # ガロア体実装
│   ├── ldpc.c             # LDPC 実装
│   ├── rng.c              # Xoshiro128++ RNG
│   ├── rs.c               # Reed-Solomon コア
│   ├── poporon.c          # 統合 API 実装
│   └── internal/
│       ├── common.h       # 内部型とマクロ
│       ├── config.h       # 設定の内部構造
│       ├── ldpc.h         # LDPC 内部構造
│       └── simd.h         # SIMD 抽象化
├── tests/                 # Unity を使用したテストスイート
│   ├── test_basic.c       # 基本機能テスト
│   ├── test_bch.c         # BCH テスト
│   ├── test_codec.c       # コーデックテスト
│   ├── test_erasure.c     # イレージャーテスト
│   ├── test_gf.c          # ガロア体テスト
│   ├── test_invalid.c     # 無効入力テスト
│   ├── test_ldpc.c        # LDPC テスト
│   ├── test_rng.c         # RNG テスト
│   ├── test_rs.c          # Reed-Solomon テスト
│   ├── test_unified.c     # 統合 API テスト
│   ├── fec_compat.c       # FEC 互換性テスト
│   └── util.h             # テストユーティリティ
├── cmake/                 # CMake モジュール
│   ├── buildtime.cmake    # ビルドタイムスタンプ
│   ├── emscripten.cmake   # WebAssembly サポート
│   └── test.cmake         # テスト設定
└── third_party/           # 依存関係
    ├── emsdk/             # Emscripten SDK（オプション）
    ├── unity/             # Unity Test フレームワーク
    └── valgrind/          # Valgrind ヘッダー
```

## 依存関係

- **[Unity](https://github.com/ThrowTheSwitch/Unity)** - ユニットテストフレームワーク（サブモジュール、テストのみ）

## ライセンス

MIT License - 詳細は [LICENSE](LICENSE) を参照してください。

## 作者

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)
