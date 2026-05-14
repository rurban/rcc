# Windows RCC Benchmark Results

_Generated: 05/14/2026 11:35:49_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         2884 |          741 |       3625 |
| RCC -O1 (optimized)   |         1248 |          741 |       1989 |
| TCC (Tiny C Compiler) |         1248 |          498 |       1746 |
| GCC -O0 (no opt)      |         1247 |          467 |       1714 |
| GCC -O2 (optimized)   |         1246 |          118 |       1364 |
| CLANG -O2 (optimized) |         1254 |          189 |       1443 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.31x
- Execute speed : RCC/TCC = 1.49x
- Total : RCC/TCC = 2.08x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
