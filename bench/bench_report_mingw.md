# Windows RCC Benchmark Results

_Generated: 06/25/2026 11:47:46_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3062 |          733 |       3795 |
| RCC -O1 (optimized)   |         1293 |          779 |       2072 |
| TCC (Tiny C Compiler) |         1281 |          500 |       1781 |
| GCC -O0 (no opt)      |         1264 |          467 |       1731 |
| GCC -O2 (optimized)   |         1263 |          119 |       1382 |
| CLANG -O2 (optimized) |         1262 |          188 |       1450 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.39x
- Execute speed : RCC/TCC = 1.47x
- Total : RCC/TCC = 2.13x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
