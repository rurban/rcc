# Windows RCC Benchmark Results

_Generated: 05/03/2026 18:14:13_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |        15136 |          611 |      15747 |
| RCC -O1 (optimized)   |         1249 |          614 |       1863 |
| TCC (Tiny C Compiler) |         1254 |          503 |       1757 |
| GCC -O0 (no opt)      |         5273 |          468 |       5741 |
| GCC -O2 (optimized)   |         4277 |          118 |       4395 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 12.07x
- Execute speed : RCC/TCC = 1.21x
- Total : RCC/TCC = 8.96x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
