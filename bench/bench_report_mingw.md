# Windows RCC Benchmark Results

_Generated: 06/03/2026 05:13:22_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2904 |          746 |       3650 |
| RCC -O1 (optimized)   |         1257 |          739 |       1996 |
| TCC (Tiny C Compiler) |         1252 |          498 |       1750 |
| GCC -O0 (no opt)      |         1246 |          468 |       1714 |
| GCC -O2 (optimized)   |         1255 |          118 |       1373 |
| CLANG -O2 (optimized) |         1268 |          188 |       1456 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.32x
- Execute speed : RCC/TCC = 1.5x
- Total : RCC/TCC = 2.09x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
