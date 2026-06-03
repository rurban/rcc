# Windows RCC Benchmark Results

_Generated: 06/04/2026 11:54:52_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2891 |          611 |       3502 |
| RCC -O1 (optimized)   |         1252 |          604 |       1856 |
| TCC (Tiny C Compiler) |         1244 |          427 |       1671 |
| GCC -O0 (no opt)      |         1248 |          417 |       1665 |
| GCC -O2 (optimized)   |         1250 |          115 |       1365 |
| CLANG -O2 (optimized) |         1257 |          169 |       1426 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.32x
- Execute speed : RCC/TCC = 1.43x
- Total : RCC/TCC = 2.1x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
