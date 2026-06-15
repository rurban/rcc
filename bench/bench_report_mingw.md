# Windows RCC Benchmark Results

_Generated: 06/15/2026 20:47:06_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2800 |          740 |       3540 |
| RCC -O1 (optimized)   |         1262 |          737 |       1999 |
| TCC (Tiny C Compiler) |         1243 |          497 |       1740 |
| GCC -O0 (no opt)      |         1246 |          468 |       1714 |
| GCC -O2 (optimized)   |         1240 |          118 |       1358 |
| CLANG -O2 (optimized) |         1251 |          188 |       1439 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.25x
- Execute speed : RCC/TCC = 1.49x
- Total : RCC/TCC = 2.03x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
