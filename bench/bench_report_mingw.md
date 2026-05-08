# Windows RCC Benchmark Results

_Generated: 05/08/2026 12:49:04_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3213 |          620 |       3833 |
| RCC -O1 (optimized)   |         1269 |          619 |       1888 |
| TCC (Tiny C Compiler) |         1257 |          498 |       1755 |
| GCC -O0 (no opt)      |         1279 |          468 |       1747 |
| GCC -O2 (optimized)   |         3288 |          120 |       3408 |
| CLANG -O2 (optimized) |         6251 |          190 |       6441 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.56x
- Execute speed : RCC/TCC = 1.24x
- Total : RCC/TCC = 2.18x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
