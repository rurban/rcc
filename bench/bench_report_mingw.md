# Windows RCC Benchmark Results

_Generated: 06/16/2026 11:53:58_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3049 |          614 |       3663 |
| RCC -O1 (optimized)   |         1277 |          611 |       1888 |
| TCC (Tiny C Compiler) |         1261 |          447 |       1708 |
| GCC -O0 (no opt)      |         1260 |          425 |       1685 |
| GCC -O2 (optimized)   |         1261 |          116 |       1377 |
| CLANG -O2 (optimized) |         1280 |          169 |       1449 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.42x
- Execute speed : RCC/TCC = 1.37x
- Total : RCC/TCC = 2.14x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
