# Windows RCC Benchmark Results

_Generated: 06/16/2026 07:42:02_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6966 |          598 |       7564 |
| RCC -O1 (optimized)   |         1244 |          607 |       1851 |
| TCC (Tiny C Compiler) |         1247 |          430 |       1677 |
| GCC -O0 (no opt)      |         1245 |          424 |       1669 |
| GCC -O2 (optimized)   |         2242 |          115 |       2357 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.59x
- Execute speed : RCC/TCC = 1.39x
- Total : RCC/TCC = 4.51x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
