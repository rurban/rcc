# Windows RCC Benchmark Results

_Generated: 05/31/2026 03:56:01_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         7169 |          727 |       7896 |
| RCC -O1 (optimized)   |         1244 |          734 |       1978 |
| TCC (Tiny C Compiler) |         1249 |          498 |       1747 |
| GCC -O0 (no opt)      |         2273 |          465 |       2738 |
| GCC -O2 (optimized)   |         5271 |          120 |       5391 |
| CLANG -O2 (optimized) |         2257 |          192 |       2449 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.74x
- Execute speed : RCC/TCC = 1.46x
- Total : RCC/TCC = 4.52x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
