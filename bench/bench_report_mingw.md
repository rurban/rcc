# Windows RCC Benchmark Results

_Generated: 06/25/2026 08:10:03_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2781 |          739 |       3520 |
| RCC -O1 (optimized)   |         1257 |          733 |       1990 |
| TCC (Tiny C Compiler) |         1253 |          498 |       1751 |
| GCC -O0 (no opt)      |         1261 |          466 |       1727 |
| GCC -O2 (optimized)   |         1250 |          118 |       1368 |
| CLANG -O2 (optimized) |         1257 |          187 |       1444 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.22x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 2.01x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
