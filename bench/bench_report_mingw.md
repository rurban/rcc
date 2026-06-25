# Windows RCC Benchmark Results

_Generated: 06/25/2026 09:51:47_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2817 |          595 |       3412 |
| RCC -O1 (optimized)   |         1248 |          595 |       1843 |
| TCC (Tiny C Compiler) |         1260 |          435 |       1695 |
| GCC -O0 (no opt)      |         1249 |          416 |       1665 |
| GCC -O2 (optimized)   |         1246 |          116 |       1362 |
| CLANG -O2 (optimized) |         1244 |          169 |       1413 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.24x
- Execute speed : RCC/TCC = 1.37x
- Total : RCC/TCC = 2.01x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
