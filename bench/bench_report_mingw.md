# Windows RCC Benchmark Results

_Generated: 06/16/2026 14:40:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5480 |          743 |       6223 |
| RCC -O1 (optimized)   |         1249 |          738 |       1987 |
| TCC (Tiny C Compiler) |         1249 |          500 |       1749 |
| GCC -O0 (no opt)      |         1254 |          468 |       1722 |
| GCC -O2 (optimized)   |         1244 |          118 |       1362 |
| CLANG -O2 (optimized) |         2701 |          192 |       2893 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.39x
- Execute speed : RCC/TCC = 1.49x
- Total : RCC/TCC = 3.56x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
