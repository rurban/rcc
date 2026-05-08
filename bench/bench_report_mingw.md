# Windows RCC Benchmark Results

_Generated: 05/08/2026 07:03:44_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3095 |          538 |       3633 |
| RCC -O1 (optimized)   |         1245 |          539 |       1784 |
| TCC (Tiny C Compiler) |         1239 |          439 |       1678 |
| GCC -O0 (no opt)      |         1271 |          426 |       1697 |
| GCC -O2 (optimized)   |         4267 |          116 |       4383 |
| CLANG -O2 (optimized) |         1834 |          169 |       2003 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.5x
- Execute speed : RCC/TCC = 1.23x
- Total : RCC/TCC = 2.17x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
