# Windows RCC Benchmark Results

_Generated: 05/07/2026 06:58:54_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2801 |          609 |       3410 |
| RCC -O1 (optimized)   |         1254 |          605 |       1859 |
| TCC (Tiny C Compiler) |         1258 |          497 |       1755 |
| GCC -O0 (no opt)      |         1251 |          466 |       1717 |
| GCC -O2 (optimized)   |         1270 |          118 |       1388 |
| CLANG -O2 (optimized) |         1263 |          189 |       1452 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.23x
- Execute speed : RCC/TCC = 1.23x
- Total : RCC/TCC = 1.94x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
