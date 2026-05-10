# Windows RCC Benchmark Results

_Generated: 05/10/2026 14:38:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         4136 |          618 |       4754 |
| RCC -O1 (optimized)   |         1247 |          616 |       1863 |
| TCC (Tiny C Compiler) |         1247 |          497 |       1744 |
| GCC -O0 (no opt)      |         1245 |          466 |       1711 |
| GCC -O2 (optimized)   |         1243 |          118 |       1361 |
| CLANG -O2 (optimized) |         1245 |          188 |       1433 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 3.32x
- Execute speed : RCC/TCC = 1.24x
- Total : RCC/TCC = 2.73x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
