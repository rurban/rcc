# Windows RCC Benchmark Results

_Generated: 06/19/2026 08:26:04_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6251 |          600 |       6851 |
| RCC -O1 (optimized)   |         1254 |          606 |       1860 |
| TCC (Tiny C Compiler) |         1251 |          428 |       1679 |
| GCC -O0 (no opt)      |         1250 |          416 |       1666 |
| GCC -O2 (optimized)   |         1274 |          115 |       1389 |
| CLANG -O2 (optimized) |         1249 |          170 |       1419 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5x
- Execute speed : RCC/TCC = 1.4x
- Total : RCC/TCC = 4.08x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
