# Windows RCC Benchmark Results

_Generated: 06/13/2026 19:52:42_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6846 |          601 |       7447 |
| RCC -O1 (optimized)   |         1242 |          604 |       1846 |
| TCC (Tiny C Compiler) |         1260 |          430 |       1690 |
| GCC -O0 (no opt)      |         1254 |          418 |       1672 |
| GCC -O2 (optimized)   |         1245 |          116 |       1361 |
| CLANG -O2 (optimized) |         2268 |          169 |       2437 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 5.43x
- Execute speed : RCC/TCC = 1.4x
- Total : RCC/TCC = 4.41x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
