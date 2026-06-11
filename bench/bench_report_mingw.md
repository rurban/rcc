# Windows RCC Benchmark Results

_Generated: 06/11/2026 16:51:27_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2773 |          737 |       3510 |
| RCC -O1 (optimized)   |         1235 |          738 |       1973 |
| TCC (Tiny C Compiler) |         1247 |          500 |       1747 |
| GCC -O0 (no opt)      |         1239 |          467 |       1706 |
| GCC -O2 (optimized)   |         1241 |          118 |       1359 |
| CLANG -O2 (optimized) |         1241 |          187 |       1428 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.22x
- Execute speed : RCC/TCC = 1.47x
- Total : RCC/TCC = 2.01x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
