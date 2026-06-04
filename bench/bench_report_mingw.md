# Windows RCC Benchmark Results

_Generated: 06/04/2026 17:26:41_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5041 |          738 |       5779 |
| RCC -O1 (optimized)   |         1249 |          739 |       1988 |
| TCC (Tiny C Compiler) |         1251 |          496 |       1747 |
| GCC -O0 (no opt)      |         2244 |          465 |       2709 |
| GCC -O2 (optimized)   |         2264 |          118 |       2382 |
| CLANG -O2 (optimized) |         1249 |          188 |       1437 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.03x
- Execute speed : RCC/TCC = 1.49x
- Total : RCC/TCC = 3.31x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
