# Windows RCC Benchmark Results

_Generated: 06/15/2026 21:00:28_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2907 |          599 |       3506 |
| RCC -O1 (optimized)   |         1248 |          603 |       1851 |
| TCC (Tiny C Compiler) |         1268 |          427 |       1695 |
| GCC -O0 (no opt)      |         1243 |          418 |       1661 |
| GCC -O2 (optimized)   |         1237 |          116 |       1353 |
| CLANG -O2 (optimized) |         1250 |          170 |       1420 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.29x
- Execute speed : RCC/TCC = 1.4x
- Total : RCC/TCC = 2.07x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
