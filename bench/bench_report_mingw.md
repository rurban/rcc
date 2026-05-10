# Windows RCC Benchmark Results

_Generated: 05/10/2026 14:23:32_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         9063 |          654 |       9717 |
| RCC -O1 (optimized)   |         1279 |          655 |       1934 |
| TCC (Tiny C Compiler) |         1257 |          672 |       1929 |
| GCC -O0 (no opt)      |         1234 |          557 |       1791 |
| GCC -O2 (optimized)   |         1233 |          135 |       1368 |
| CLANG -O2 (optimized) |         1247 |          309 |       1556 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 7.21x
- Execute speed : RCC/TCC = 0.97x
- Total : RCC/TCC = 5.04x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
