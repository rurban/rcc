# Windows RCC Benchmark Results

_Generated: 05/01/2026 14:42:32_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         9680 |          558 |      10238 |
| RCC -O1 (optimized)   |         1260 |          548 |       1808 |
| TCC (Tiny C Compiler) |         1244 |          427 |       1671 |
| GCC -O0 (no opt)      |         1240 |          414 |       1654 |
| GCC -O2 (optimized)   |         1237 |          117 |       1354 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 7.78x
- Execute speed : RCC/TCC = 1.31x
- Total : RCC/TCC = 6.13x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
