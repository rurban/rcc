# Windows RCC Benchmark Results

_Generated: 05/07/2026 04:08:58_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3186 |          610 |       3796 |
| RCC -O1 (optimized)   |         1246 |          622 |       1868 |
| TCC (Tiny C Compiler) |         1252 |          497 |       1749 |
| GCC -O0 (no opt)      |         1248 |          467 |       1715 |
| GCC -O2 (optimized)   |         1242 |          118 |       1360 |
| CLANG -O2 (optimized) |         1258 |          188 |       1446 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.54x
- Execute speed : RCC/TCC = 1.23x
- Total : RCC/TCC = 2.17x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
