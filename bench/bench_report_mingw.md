# Windows RCC Benchmark Results

_Generated: 05/14/2026 20:47:20_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2648 |          737 |       3385 |
| RCC -O1 (optimized)   |         1260 |          738 |       1998 |
| TCC (Tiny C Compiler) |         1281 |          504 |       1785 |
| GCC -O2 (optimized)   |         2324 |          118 |       2442 |
| CLANG -O2 (optimized) |         1254 |          187 |       1441 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.07x
- Execute speed : RCC/TCC = 1.46x
- Total : RCC/TCC = 1.9x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
