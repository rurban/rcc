# Windows RCC Benchmark Results

_Generated: 05/06/2026 14:06:20_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |        13301 |          556 |      13857 |
| RCC -O1 (optimized)   |         1289 |          565 |       1854 |
| TCC (Tiny C Compiler) |         1277 |          430 |       1707 |
| GCC -O0 (no opt)      |         1273 |          422 |       1695 |
| GCC -O2 (optimized)   |         1269 |          117 |       1386 |
| CLANG -O2 (optimized) |         1248 |          169 |       1417 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 10.42x
- Execute speed : RCC/TCC = 1.29x
- Total : RCC/TCC = 8.12x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
