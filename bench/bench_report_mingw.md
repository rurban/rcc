# Windows RCC Benchmark Results

_Generated: 06/16/2026 07:35:03_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3044 |          750 |       3794 |
| RCC -O1 (optimized)   |         1282 |          742 |       2024 |
| TCC (Tiny C Compiler) |         1284 |          508 |       1792 |
| GCC -O0 (no opt)      |         1768 |          465 |       2233 |
| GCC -O2 (optimized)   |         1257 |          118 |       1375 |
| CLANG -O2 (optimized) |         1258 |          188 |       1446 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.37x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 2.12x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
