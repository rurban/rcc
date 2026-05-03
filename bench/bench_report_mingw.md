# Windows RCC Benchmark Results

_Generated: 05/03/2026 04:19:56_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5617 |          558 |       6175 |
| RCC -O1 (optimized)   |         1260 |          554 |       1814 |
| TCC (Tiny C Compiler) |         1248 |          431 |       1679 |
| GCC -O0 (no opt)      |         1247 |          416 |       1663 |
| GCC -O2 (optimized)   |         1261 |          117 |       1378 |
| CLANG -O2 (optimized) |         4080 |          170 |       4250 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.5x
- Execute speed : RCC/TCC = 1.29x
- Total : RCC/TCC = 3.68x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
