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
    poporon_t *pprn = poporon_create(
        8,      // symbol_size（シンボルあたりのビット数）
        0x11D,  // generator_polynomial
        1,      // first_consecutive_root
        1,      // primitive_element
        32      // num_roots（パリティシンボル数）
    );
    if (!pprn) {
        fprintf(stderr, "poporon インスタンスの作成に失敗しました\n");
        return 1;
    }

    // データを準備
    uint8_t data[64];
    uint8_t parity[32];
    memset(data, 0, sizeof(data));
    memcpy(data, "Hello, Reed-Solomon!", 20);

    // エンコード - パリティを生成
    poporon_encode_u8(pprn, data, sizeof(data), parity);

    // エラーをシミュレート
    data[0] ^= 0xFF;
    data[10] ^= 0xAA;

    // デコード - エラーを訂正
    size_t corrected_num = 0;
    if (poporon_decode_u8(pprn, data, sizeof(data), parity, &corrected_num)) {
        printf("%zu 個のエラーを訂正しました\n", corrected_num);
        printf("デコード結果: %s\n", data);
    }

    poporon_destroy(pprn);
    return 0;
}
```

### BCH エンコードとデコード

```c
#include <poporon/bch.h>
#include <stdio.h>

int main(void) {
    // t=3 の誤り訂正能力を持つ BCH(15, k) を作成
    poporon_bch_t *bch = poporon_bch_create(
        4,      // symbol_size（m、体 GF(2^m)）
        0x13,   // generator_polynomial
        3       // correction_capability（t エラー）
    );
    if (!bch) {
        fprintf(stderr, "BCH インスタンスの作成に失敗しました\n");
        return 1;
    }

    printf("コードワード長: %u\n", poporon_bch_get_codeword_length(bch));
    printf("データ長: %u\n", poporon_bch_get_data_length(bch));
    printf("訂正能力: %u エラー\n", poporon_bch_get_correction_capability(bch));

    // データをエンコード
    uint32_t data = 21;
    uint32_t codeword;
    poporon_bch_encode(bch, data, &codeword);
    printf("元のコードワード: 0x%04X\n", codeword);

    // 2 ビットエラーをシミュレート
    uint32_t corrupted = codeword ^ (1 << 3) ^ (1 << 7);
    printf("破損: 0x%04X\n", corrupted);

    // デコードして訂正
    uint32_t corrected;
    int32_t num_errors;
    if (poporon_bch_decode(bch, corrupted, &corrected, &num_errors)) {
        printf("%d 個のエラーを訂正: 0x%04X\n", num_errors, corrected);
        printf("抽出データ: %u\n", poporon_bch_extract_data(bch, corrected));
    }

    poporon_bch_destroy(bch);
    return 0;
}
```

### LDPC エンコードとデコード

```c
#include <poporon/ldpc.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // レート 1/2（50% 冗長）の LDPC エンコーダを作成
    poporon_ldpc_t *ldpc = poporon_ldpc_create(
        128,                  // block_size（バイト、4 の倍数である必要あり）
        PPRN_LDPC_RATE_1_2,   // コードレート
        NULL                  // config（デフォルトは NULL）
    );
    if (!ldpc) {
        fprintf(stderr, "LDPC インスタンスの作成に失敗しました\n");
        return 1;
    }

    size_t info_size = poporon_ldpc_info_size(ldpc);
    size_t parity_size = poporon_ldpc_parity_size(ldpc);
    size_t codeword_size = poporon_ldpc_codeword_size(ldpc);

    printf("情報サイズ: %zu バイト\n", info_size);
    printf("パリティサイズ: %zu バイト\n", parity_size);
    printf("コードワードサイズ: %zu バイト\n", codeword_size);

    // バッファを準備
    uint8_t *info = malloc(info_size);
    uint8_t *parity = malloc(parity_size);
    uint8_t *codeword = malloc(codeword_size);

    // データを初期化
    for (size_t i = 0; i < info_size; i++) {
        info[i] = (uint8_t)(i * 17 + 23);
    }

    // エンコード
    poporon_ldpc_encode(ldpc, info, parity);

    // コードワードを作成（info + parity）
    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    // コードワードが有効か検証
    if (poporon_ldpc_check(ldpc, codeword)) {
        printf("コードワードは有効です\n");
    }

    // エラーをシミュレート
    codeword[0] ^= 0x01;
    codeword[10] ^= 0x80;
    codeword[20] ^= 0x40;

    // 反復ビリーフプロパゲーションでデコード
    uint32_t iterations;
    if (poporon_ldpc_decode_hard(ldpc, codeword, 50, &iterations)) {
        printf("%u 回の反復でデコード成功\n", iterations);
    }

    free(info);
    free(parity);
    free(codeword);
    poporon_ldpc_destroy(ldpc);
    return 0;
}
```

### バースト誤り耐性を持つ LDPC

```c
#include <poporon/ldpc.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // バースト誤り耐性 LDPC を設定
    poporon_ldpc_config_t config;
    poporon_ldpc_config_burst_resistant(&config);
    // config は column_weight=7, use_interleaver=true を持つ

    poporon_ldpc_t *ldpc = poporon_ldpc_create(128, PPRN_LDPC_RATE_1_2, &config);

    size_t info_size = poporon_ldpc_info_size(ldpc);
    size_t parity_size = poporon_ldpc_parity_size(ldpc);
    size_t codeword_size = poporon_ldpc_codeword_size(ldpc);

    uint8_t *info = malloc(info_size);
    uint8_t *parity = malloc(parity_size);
    uint8_t *codeword = malloc(codeword_size);
    uint8_t *interleaved = malloc(codeword_size);

    // 初期化とエンコード
    for (size_t i = 0; i < info_size; i++) {
        info[i] = (uint8_t)i;
    }
    poporon_ldpc_encode(ldpc, info, parity);

    memcpy(codeword, info, info_size);
    memcpy(codeword + info_size, parity, parity_size);

    // 送信前にインターリーブ
    poporon_ldpc_interleave(ldpc, codeword, interleaved);

    // バースト誤りをシミュレート（連続バイトの破損）
    for (size_t i = 40; i < 44; i++) {
        interleaved[i] ^= 0xFF;
    }

    // デコード（デインターリーブは自動的に行われる）
    uint32_t iterations;
    if (poporon_ldpc_decode_hard(ldpc, interleaved, 100, &iterations)) {
        printf("バースト誤りを %u 回の反復で訂正\n", iterations);
    }

    free(info);
    free(parity);
    free(codeword);
    free(interleaved);
    poporon_ldpc_destroy(ldpc);
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

### Reed-Solomon 型

```c
typedef struct _poporon_t poporon_t;         // メインの Reed-Solomon コーデック
typedef struct _poporon_erasure_t poporon_erasure_t;  // イレージャー位置の追跡
typedef struct _poporon_gf_t poporon_gf_t;   // ガロア体演算
typedef struct _poporon_rs_t poporon_rs_t;   // Reed-Solomon パラメータ
```

### Reed-Solomon 関数

```c
// コーデックの作成/破棄
poporon_t *poporon_create(uint8_t symbol_size, uint16_t generator_polynomial,
                          uint16_t first_consecutive_root, uint16_t primitive_element,
                          uint8_t num_roots);
void poporon_destroy(poporon_t *poporon);

// データをエンコード
bool poporon_encode_u8(poporon_t *pprn, uint8_t *data, size_t size, uint8_t *parity);

// データをデコード
bool poporon_decode_u8(poporon_t *pprn, uint8_t *data, size_t size,
                       uint8_t *parity, size_t *corrected_num);
bool poporon_decode_u8_with_erasure(poporon_t *pprn, uint8_t *data, size_t size,
                                    uint8_t *parity, poporon_erasure_t *eras,
                                    size_t *corrected_num);
bool poporon_decode_u8_with_syndrome(poporon_t *pprn, uint8_t *data, uint8_t *parity,
                                     size_t size, uint16_t *syndrome,
                                     size_t *corrected_num);

// ユーティリティ
uint32_t poporon_version_id(void);
poporon_buildtime_t poporon_buildtime(void);
```

### BCH 関数

```c
// BCH コーデックの作成/破棄
poporon_bch_t *poporon_bch_create(uint8_t symbol_size, uint16_t generator_polynomial,
                                  uint8_t correction_capability);
void poporon_bch_destroy(poporon_bch_t *bch);

// パラメータを取得
uint16_t poporon_bch_get_codeword_length(const poporon_bch_t *bch);
uint16_t poporon_bch_get_data_length(const poporon_bch_t *bch);
uint8_t poporon_bch_get_correction_capability(const poporon_bch_t *bch);

// エンコード/デコード
bool poporon_bch_encode(poporon_bch_t *bch, uint32_t data, uint32_t *codeword);
bool poporon_bch_decode(poporon_bch_t *bch, uint32_t received,
                        uint32_t *corrected, int32_t *num_errors);
uint32_t poporon_bch_extract_data(const poporon_bch_t *bch, uint32_t codeword);
```

### LDPC 型と定数

```c
typedef struct _poporon_ldpc_t poporon_ldpc_t;

// コードレート
typedef enum {
    PPRN_LDPC_RATE_1_2,  // 50% 冗長
    PPRN_LDPC_RATE_2_3,  // 33% 冗長
    PPRN_LDPC_RATE_3_4,  // 25% 冗長
    PPRN_LDPC_RATE_4_5,  // 20% 冗長
    PPRN_LDPC_RATE_5_6,  // 17% 冗長
} poporon_ldpc_rate_t;

// 行列構成タイプ
typedef enum {
    PPRN_LDPC_RANDOM,   // ランダムパリティ検査行列
    PPRN_LDPC_QC_PEG,   // 準巡回プログレッシブエッジグロース
} poporon_ldpc_matrix_type_t;

// 設定
typedef struct {
    poporon_ldpc_matrix_type_t matrix_type;
    uint32_t column_weight;      // パリティ行列の密度（3-8）
    bool use_interleaver;        // バースト耐性のためインターリーブを有効化
    uint32_t interleave_depth;   // インターリーブ深度（0 で自動）
    uint32_t lifting_factor;     // QC-LDPC リフティング係数（0 で自動）
} poporon_ldpc_config_t;
```

### LDPC 関数

```c
// 設定ヘルパー
bool poporon_ldpc_config_default(poporon_ldpc_config_t *config);
bool poporon_ldpc_config_burst_resistant(poporon_ldpc_config_t *config);

// 作成/破棄
poporon_ldpc_t *poporon_ldpc_create(size_t block_size, poporon_ldpc_rate_t rate,
                                    const poporon_ldpc_config_t *config);
void poporon_ldpc_destroy(poporon_ldpc_t *ldpc);

// サイズを取得
size_t poporon_ldpc_info_size(const poporon_ldpc_t *ldpc);
size_t poporon_ldpc_codeword_size(const poporon_ldpc_t *ldpc);
size_t poporon_ldpc_parity_size(const poporon_ldpc_t *ldpc);

// エンコード
bool poporon_ldpc_encode(poporon_ldpc_t *ldpc, const uint8_t *info, uint8_t *parity);

// デコード
bool poporon_ldpc_decode_hard(poporon_ldpc_t *ldpc, uint8_t *codeword,
                              uint32_t max_iterations, uint32_t *iterations_used);
bool poporon_ldpc_decode_soft(poporon_ldpc_t *ldpc, const int8_t *llr,
                              uint8_t *codeword, uint32_t max_iterations,
                              uint32_t *iterations_used);

// 検証
bool poporon_ldpc_check(const poporon_ldpc_t *ldpc, const uint8_t *codeword);

// インターリーブ
bool poporon_ldpc_has_interleaver(const poporon_ldpc_t *ldpc);
bool poporon_ldpc_interleave(const poporon_ldpc_t *ldpc,
                             const uint8_t *input, uint8_t *output);
bool poporon_ldpc_deinterleave(const poporon_ldpc_t *ldpc,
                               const uint8_t *input, uint8_t *output);
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
| 1/2 | 50% | 32 - 8192 バイト |
| 2/3 | 33% | 32 - 8192 バイト |
| 3/4 | 25% | 32 - 8192 バイト |
| 4/5 | 20% | 32 - 8192 バイト |
| 5/6 | 17% | 32 - 8192 バイト |

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
│   ├── poporon.h          # メイン公開ヘッダー
│   └── poporon/
│       ├── bch.h          # BCH コーデック API
│       ├── erasure.h      # イレージャー API
│       ├── gf.h           # ガロア体 API
│       ├── ldpc.h         # LDPC コーデック API
│       ├── rng.h          # 乱数生成器 API
│       └── rs.h           # Reed-Solomon API
├── src/
│   ├── bch.c              # BCH 実装
│   ├── encode.c           # RS エンコード実装
│   ├── decode.c           # Berlekamp-Massey による RS デコード
│   ├── erasure.c          # イレージャー処理
│   ├── gf.c               # ガロア体実装
│   ├── ldpc.c             # LDPC 実装
│   ├── rng.c              # Xoshiro128++ RNG
│   ├── rs.c               # Reed-Solomon コア
│   ├── poporon.c          # メイン API 実装
│   └── internal/
│       ├── common.h       # 内部型とマクロ
│       ├── ldpc.h         # LDPC 内部構造
│       └── simd.h         # SIMD 抽象化
├── tests/                 # Unity を使用したテストスイート
│   ├── test_basic.c       # 基本機能テスト
│   ├── test_bch.c         # BCH テスト
│   ├── test_codec.c       # コーデックテスト
│   ├── test_erasure.c     # イレージャーテスト
│   ├── test_gf.c          # ガロア体テスト
│   ├── test_ldpc.c        # LDPC テスト
│   ├── test_rng.c         # RNG テスト
│   └── test_rs.c          # Reed-Solomon テスト
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
