# Windows RCC Benchmark Results

_Generated: 06/11/2026 09:16:17_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2748 |          610 |       3358 |
| RCC -O1 (optimized)   |         1242 |          596 |       1838 |
| TCC (Tiny C Compiler) |         1244 |          428 |       1672 |
| GCC -O0 (no opt)      |         1243 |          415 |       1658 |
| GCC -O2 (optimized)   |         1234 |          115 |       1349 |
| CLANG -O2 (optimized) |         1237 |          168 |       1405 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.21x
- Execute speed : RCC/TCC = 1.43x
- Total : RCC/TCC = 2.01x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
