# Windows RCC Benchmark Results

_Generated: 05/02/2026 19:57:13_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         5010 |          550 |       5560 |
| RCC -O1 (optimized)   |         1252 |          552 |       1804 |
| TCC (Tiny C Compiler) |         1251 |          431 |       1682 |
| GCC -O0 (no opt)      |         1254 |          418 |       1672 |
| GCC -O2 (optimized)   |         1307 |          116 |       1423 |
| CLANG -O2 (optimized) |         1263 |          169 |       1432 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4x
- Execute speed : RCC/TCC = 1.28x
- Total : RCC/TCC = 3.31x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
