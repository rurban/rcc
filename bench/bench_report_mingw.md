# Windows RCC Benchmark Results

_Generated: 06/20/2026 08:34:31_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2855 |          605 |       3460 |
| RCC -O1 (optimized)   |         1248 |          603 |       1851 |
| TCC (Tiny C Compiler) |         1246 |          430 |       1676 |
| GCC -O0 (no opt)      |         1249 |          416 |       1665 |
| GCC -O2 (optimized)   |         1249 |          116 |       1365 |
| CLANG -O2 (optimized) |         1264 |          175 |       1439 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.29x
- Execute speed : RCC/TCC = 1.41x
- Total : RCC/TCC = 2.06x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
