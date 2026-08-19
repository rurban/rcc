# Windows RCC Benchmark Results

_Generated: 05/02/2026 11:44:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2504 |          474 |       2978 |
| RCC -O1 (optimized)   |         1201 |          472 |       1673 |
| TCC (Tiny C Compiler) |         1194 |          385 |       1579 |
| GCC -O0 (no opt)      |         1191 |          362 |       1553 |
| GCC -O2 (optimized)   |         1192 |           91 |       1283 |
| CLANG -O2 (optimized) |         1196 |          145 |       1341 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.1x
- Execute speed : RCC/TCC = 1.23x
- Total : RCC/TCC = 1.89x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
