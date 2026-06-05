# Windows RCC Benchmark Results

_Generated: 06/05/2026 13:20:41_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5886 |          599 |       6485 |
| RCC -O1 (optimized)   |         1249 |          601 |       1850 |
| TCC (Tiny C Compiler) |         1298 |          430 |       1728 |
| GCC -O0 (no opt)      |         2258 |          417 |       2675 |
| GCC -O2 (optimized)   |         2262 |          115 |       2377 |
| CLANG -O2 (optimized) |         1254 |          169 |       1423 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.53x
- Execute speed : RCC/TCC = 1.39x
- Total : RCC/TCC = 3.75x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
