# Windows RCC Benchmark Results

_Generated: 04/30/2026 12:35:33_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2970 |          609 |       3579 |
| RCC -O1 (optimized)   |         1255 |          609 |       1864 |
| TCC (Tiny C Compiler) |         1254 |          498 |       1752 |
| GCC -O0 (no opt)      |         1253 |          469 |       1722 |
| GCC -O2 (optimized)   |         1252 |          118 |       1370 |
| CLANG -O2 (optimized) |         1258 |          188 |       1446 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.37x
- Execute speed : RCC/TCC = 1.22x
- Total : RCC/TCC = 2.04x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
