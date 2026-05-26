# Windows RCC Benchmark Results

_Generated: 05/26/2026 15:07:20_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2992 |          561 |       3553 |
| RCC -O1 (optimized)   |         1244 |          563 |       1807 |
| TCC (Tiny C Compiler) |         1262 |          427 |       1689 |
| GCC -O0 (no opt)      |         1246 |          425 |       1671 |
| GCC -O2 (optimized)   |         1254 |          116 |       1370 |
| CLANG -O2 (optimized) |         1253 |          172 |       1425 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.37x
- Execute speed : RCC/TCC = 1.31x
- Total : RCC/TCC = 2.1x

## Output Correctness

- RCC (your compiler): OUTPUT DIFFERS
- RCC -O1 (optimized): OUTPUT DIFFERS
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
