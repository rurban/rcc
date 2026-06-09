# Windows RCC Benchmark Results

_Generated: 06/09/2026 12:55:06_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5699 |          595 |       6294 |
| RCC -O1 (optimized)   |         1244 |          602 |       1846 |
| TCC (Tiny C Compiler) |         1249 |          429 |       1678 |
| GCC -O0 (no opt)      |         1248 |          417 |       1665 |
| GCC -O2 (optimized)   |         1249 |          115 |       1364 |
| CLANG -O2 (optimized) |         2292 |          168 |       2460 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.56x
- Execute speed : RCC/TCC = 1.39x
- Total : RCC/TCC = 3.75x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
