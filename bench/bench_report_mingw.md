# Windows RCC Benchmark Results

_Generated: 06/14/2026 10:22:42_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3191 |          741 |       3932 |
| RCC -O1 (optimized)   |         1260 |          746 |       2006 |
| TCC (Tiny C Compiler) |         1283 |          535 |       1818 |
| GCC -O0 (no opt)      |         1270 |          474 |       1744 |
| GCC -O2 (optimized)   |         1277 |          119 |       1396 |
| CLANG -O2 (optimized) |         5022 |          188 |       5210 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.49x
- Execute speed : RCC/TCC = 1.39x
- Total : RCC/TCC = 2.16x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
