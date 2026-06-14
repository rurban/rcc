# Windows RCC Benchmark Results

_Generated: 06/14/2026 13:04:43_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |        14036 |          569 |      14605 |
| RCC -O1 (optimized)   |         1200 |          570 |       1770 |
| TCC (Tiny C Compiler) |         1212 |          386 |       1598 |
| GCC -O0 (no opt)      |         1198 |          364 |       1562 |
| GCC -O2 (optimized)   |         2199 |           93 |       2292 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 11.58x
- Execute speed : RCC/TCC = 1.47x
- Total : RCC/TCC = 9.14x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
