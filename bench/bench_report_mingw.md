# Windows RCC Benchmark Results

_Generated: 06/05/2026 07:55:18_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3075 |          616 |       3691 |
| RCC -O1 (optimized)   |         1255 |          600 |       1855 |
| TCC (Tiny C Compiler) |         1258 |          432 |       1690 |
| GCC -O0 (no opt)      |         1277 |          417 |       1694 |
| GCC -O2 (optimized)   |         1265 |          121 |       1386 |
| CLANG -O2 (optimized) |         1260 |          169 |       1429 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.44x
- Execute speed : RCC/TCC = 1.43x
- Total : RCC/TCC = 2.18x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
