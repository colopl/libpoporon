# libpoporon

[![CI](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml/badge.svg)](https://github.com/zeriyoshi/libpoporon/actions/workflows/ci.yaml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

**Polynomial Operations Providing Optimal Reed-Solomon Organized Numerics**

[🇬🇧 English README](README.md)

libpoporon は、C99 で書かれた軽量・高性能なリード・ソロモン誤り訂正ライブラリです。エンコードとデコード機能を提供し、SIMD による高速化オプションで最大限のパフォーマンスを実現します。

## 特徴

- **純粋な C99 実装** - 外部依存なし、プラットフォーム間で移植可能
- **SIMD 高速化** - AVX2 (x86_64)、NEON (ARM64)、WASM SIMD128 による自動最適化
- **イレージャー復号** - 既知のエラー位置による誤り訂正をサポート
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

### テストの実行

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DPOPORON_USE_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## 使用例

```c
#include <poporon.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    // RS(255, 223) - 32 パリティシンボル、最大 16 エラーを訂正可能
    const uint8_t symbol_size = 8;
    const uint16_t generator_polynomial = 0x11D;
    const uint16_t first_consecutive_root = 1;
    const uint16_t primitive_element = 1;
    const uint8_t num_roots = 32;

    // リード・ソロモンコーデックを作成
    poporon_t *pprn = poporon_create(
        symbol_size,
        generator_polynomial,
        first_consecutive_root,
        primitive_element,
        num_roots
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
    if (!poporon_encode_u8(pprn, data, sizeof(data), parity)) {
        fprintf(stderr, "エンコードに失敗しました\n");
        poporon_destroy(pprn);
        return 1;
    }

    // エラーをシミュレート
    data[0] ^= 0xFF;
    data[10] ^= 0xAA;

    // デコード - エラーを訂正
    size_t corrected_num = 0;
    if (poporon_decode_u8(pprn, data, sizeof(data), parity, &corrected_num)) {
        printf("%zu 個のエラーを訂正しました\n", corrected_num);
        printf("デコード結果: %s\n", data);
    } else {
        fprintf(stderr, "デコードに失敗しました - エラーが多すぎます\n");
    }

    poporon_destroy(pprn);
    return 0;
}
```

## API リファレンス

### コア型

```c
typedef struct _poporon_t poporon_t;         // メインのリード・ソロモンコーデック
typedef struct _poporon_erasure_t poporon_erasure_t;  // イレージャー位置の追跡
typedef struct _poporon_gf_t poporon_gf_t;   // ガロア体演算
typedef struct _poporon_rs_t poporon_rs_t;   // リード・ソロモンパラメータ
```

### メイン関数

#### `poporon_create`

```c
poporon_t *poporon_create(
    uint8_t symbol_size,           // シンボルあたりのビット数（通常 8）
    uint16_t generator_polynomial, // 体の生成多項式
    uint16_t first_consecutive_root,
    uint16_t primitive_element,
    uint8_t num_roots              // パリティシンボル数（誤り訂正能力 = num_roots / 2）
);
```

新しいリード・ソロモンコーデックインスタンスを作成します。

#### `poporon_destroy`

```c
void poporon_destroy(poporon_t *poporon);
```

コーデックに関連付けられたすべてのリソースを解放します。

#### `poporon_encode_u8`

```c
bool poporon_encode_u8(
    poporon_t *pprn,
    uint8_t *data,     // 入力データ
    size_t size,       // データサイズ（バイト）
    uint8_t *parity    // 出力: パリティシンボル（num_roots バイト必要）
);
```

データをエンコードしてパリティシンボルを生成します。

#### `poporon_decode_u8`

```c
bool poporon_decode_u8(
    poporon_t *pprn,
    uint8_t *data,         // デコードするデータ（その場で変更）
    size_t size,           // データサイズ
    uint8_t *parity,       // パリティシンボル
    size_t *corrected_num  // 出力: 訂正した数
);
```

データのエラーをデコードして訂正します。

#### `poporon_decode_u8_with_erasure`

```c
bool poporon_decode_u8_with_erasure(
    poporon_t *pprn,
    uint8_t *data,
    size_t size,
    uint8_t *parity,
    poporon_erasure_t *eras,  // 既知のエラー位置
    size_t *corrected_num
);
```

既知のイレージャー位置を使用してデコードし、訂正能力を向上させます。

### イレージャー API

```c
// イレージャートラッカーを作成
poporon_erasure_t *poporon_erasure_create(
    uint16_t num_roots,
    uint32_t initial_capacity
);

// 既知の位置から作成
poporon_erasure_t *poporon_erasure_create_from_positions(
    uint16_t num_roots,
    const uint32_t *erasure_positions,
    uint32_t erasure_count
);

// イレージャー位置を追加
bool poporon_erasure_add_position(poporon_erasure_t *erasure, uint32_t position);

// イレージャーリストをリセット
void poporon_erasure_reset(poporon_erasure_t *erasure);

// イレージャートラッカーを解放
void poporon_erasure_destroy(poporon_erasure_t *eras);
```

### ガロア体 API

```c
// GF(2^symbol_size) 体を作成
poporon_gf_t *poporon_gf_create(uint8_t symbol_size, uint16_t generator_polynomial);

// GF インスタンスを解放
void poporon_gf_destroy(poporon_gf_t *gf);

// モジュラーリダクションを計算
uint8_t poporon_gf_mod(poporon_gf_t *gf, uint16_t value);
```

### ユーティリティ関数

```c
// ライブラリバージョンを整数で取得
uint32_t poporon_version_id(void);

// ライブラリのビルドタイムスタンプを取得
poporon_buildtime_t poporon_buildtime(void);
```

## 一般的なリード・ソロモン設定

| 名前 | パラメータ | 訂正能力 |
|------|------------|----------------------|
| RS(255, 223) | symbol_size=8, gen_poly=0x11D, num_roots=32 | 16 エラー |
| RS(255, 239) | symbol_size=8, gen_poly=0x11D, num_roots=16 | 8 エラー |
| RS(255, 247) | symbol_size=8, gen_poly=0x11D, num_roots=8 | 4 エラー |

## プラットフォームサポート

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

## プロジェクト構成

```
libpoporon/
├── include/
│   ├── poporon.h          # メイン公開ヘッダー
│   └── poporon/
│       ├── erasure.h      # イレージャー API
│       ├── gf.h           # ガロア体 API
│       └── rs.h           # リード・ソロモン API
├── src/
│   ├── encode.c           # エンコード実装
│   ├── decode.c           # Berlekamp-Massey によるデコード
│   ├── erasure.c          # イレージャー処理
│   ├── gf.c               # ガロア体実装
│   ├── rs.c               # リード・ソロモンコア
│   ├── poporon.c          # メイン API 実装
│   └── internal/
│       ├── common.h       # 内部型とマクロ
│       └── simd.h         # SIMD 抽象化
├── tests/                 # Unity を使用したテストスイート
├── cmake/                 # CMake モジュール
└── third_party/           # 依存関係（Unity、Emscripten SDK）
```

## ライセンス

MIT License - 詳細は [LICENSE](LICENSE) を参照してください。

## 作者

**Go Kudo** ([@zeriyoshi](https://github.com/zeriyoshi)) - [zeriyoshi@gmail.com](mailto:zeriyoshi@gmail.com)
