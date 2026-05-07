# Windows RCC Benchmark Results

_Generated: 05/07/2026 16:34:15_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6988 |          613 |       7601 |
| RCC -O1 (optimized)   |         1267 |          608 |       1875 |
| TCC (Tiny C Compiler) |         1266 |          499 |       1765 |
| GCC -O0 (no opt)      |         2270 |          466 |       2736 |
| GCC -O2 (optimized)   |         2264 |          119 |       2383 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.52x
- Execute speed : RCC/TCC = 1.23x
- Total : RCC/TCC = 4.31x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
