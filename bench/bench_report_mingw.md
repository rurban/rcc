# Windows RCC Benchmark Results

_Generated: 06/01/2026 05:19:10_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2726 |          725 |       3451 |
| RCC -O1 (optimized)   |         1280 |          727 |       2007 |
| TCC (Tiny C Compiler) |         1247 |          501 |       1748 |
| GCC -O0 (no opt)      |         9304 |          470 |       9774 |
| GCC -O2 (optimized)   |         4448 |          136 |       4584 |
| CLANG -O2 (optimized) |         1815 |          188 |       2003 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.19x
- Execute speed : RCC/TCC = 1.45x
- Total : RCC/TCC = 1.97x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
