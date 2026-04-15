# RCC — Rising C Compiler (Release Build)

A fast C compiler for x86-64 Windows. **Generates faster code than TCC.**

## Quick Start

```bash
# Compile a C program
rcc.exe source.c -o output.exe

# Output assembly
rcc.exe source.c -S -o output.s

# Run the benchmark
powershell -File bench/run_bench.ps1
```

## Requirements

- **GCC (MinGW-w64)** must be installed and on PATH (used as assembler/linker backend).
- Windows x86-64.

## Benchmark

| Compiler | Execute (ms) | Total (ms) |
|---|---:|---:|
| **RCC** | **349** | **1391** |
| TCC 0.9.27 | 400 | 1413 |
| GCC -O0 | 298 | 1319 |
| GCC -O2 | 132 | 1152 |

RCC generates code **13% faster** than TCC (0.87x execution time).

## Contents

| Path | Description |
|---|---|
| `rcc.exe` | Pre-built compiler binary |
| `include/` | C standard library headers (stdio.h, math.h, etc.) |
| `test/` | Sample test programs |
| `bench/` | Benchmark suite and runner script |

## License

LGPL-2.1 — see LICENSE file.
