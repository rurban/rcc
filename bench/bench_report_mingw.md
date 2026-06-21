# Windows RCC Benchmark Results

_Generated: 06/21/2026 06:11:30_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2621 |          605 |       3226 |
| RCC -O1 (optimized)   |         1244 |          591 |       1835 |
| TCC (Tiny C Compiler) |         1239 |          428 |       1667 |
| GCC -O0 (no opt)      |         1233 |          416 |       1649 |
| GCC -O2 (optimized)   |         1230 |          114 |       1344 |
| CLANG -O2 (optimized) |         1245 |          168 |       1413 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.12x
- Execute speed : RCC/TCC = 1.41x
- Total : RCC/TCC = 1.94x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
