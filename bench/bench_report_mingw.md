# Windows RCC Benchmark Results

_Generated: 06/05/2026 04:57:32_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         9820 |          749 |      10569 |
| RCC -O1 (optimized)   |         1224 |          743 |       1967 |
| TCC (Tiny C Compiler) |         1227 |          672 |       1899 |
| GCC -O0 (no opt)      |         1231 |          556 |       1787 |
| GCC -O2 (optimized)   |         1280 |          133 |       1413 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 8x
- Execute speed : RCC/TCC = 1.11x
- Total : RCC/TCC = 5.57x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
