# Windows RCC Benchmark Results

_Generated: 05/30/2026 18:33:11_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3287 |          572 |       3859 |
| RCC -O1 (optimized)   |         1233 |          563 |       1796 |
| TCC (Tiny C Compiler) |         1201 |          385 |       1586 |
| GCC -O0 (no opt)      |         1201 |          360 |       1561 |
| GCC -O2 (optimized)   |         1196 |           91 |       1287 |
| CLANG -O2 (optimized) |         1204 |          146 |       1350 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.74x
- Execute speed : RCC/TCC = 1.49x
- Total : RCC/TCC = 2.43x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
