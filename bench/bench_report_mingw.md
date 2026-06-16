# Windows RCC Benchmark Results

_Generated: 06/17/2026 06:58:48_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3111 |          603 |       3714 |
| RCC -O1 (optimized)   |         1246 |          602 |       1848 |
| TCC (Tiny C Compiler) |         1239 |          428 |       1667 |
| GCC -O0 (no opt)      |         1239 |          417 |       1656 |
| GCC -O2 (optimized)   |         1250 |          117 |       1367 |
| CLANG -O2 (optimized) |         1244 |          169 |       1413 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.51x
- Execute speed : RCC/TCC = 1.41x
- Total : RCC/TCC = 2.23x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
