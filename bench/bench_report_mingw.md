# Windows RCC Benchmark Results

_Generated: 06/26/2026 05:08:00_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2802 |          592 |       3394 |
| RCC -O1 (optimized)   |         1238 |          599 |       1837 |
| TCC (Tiny C Compiler) |         1238 |          428 |       1666 |
| GCC -O0 (no opt)      |         1239 |          419 |       1658 |
| GCC -O2 (optimized)   |         1250 |          117 |       1367 |
| CLANG -O2 (optimized) |         1248 |          170 |       1418 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.26x
- Execute speed : RCC/TCC = 1.38x
- Total : RCC/TCC = 2.04x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
