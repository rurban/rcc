# Windows RCC Benchmark Results

_Generated: 05/31/2026 06:20:51_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |        10336 |          726 |      11062 |
| RCC -O1 (optimized)   |         1265 |          725 |       1990 |
| TCC (Tiny C Compiler) |         1255 |          498 |       1753 |
| GCC -O0 (no opt)      |         5271 |          465 |       5736 |
| GCC -O2 (optimized)   |         4258 |          118 |       4376 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 8.24x
- Execute speed : RCC/TCC = 1.46x
- Total : RCC/TCC = 6.31x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
