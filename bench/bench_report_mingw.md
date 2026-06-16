# Windows RCC Benchmark Results

_Generated: 06/16/2026 13:28:01_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3121 |          739 |       3860 |
| RCC -O1 (optimized)   |         1248 |          738 |       1986 |
| TCC (Tiny C Compiler) |         1242 |          498 |       1740 |
| GCC -O0 (no opt)      |         1241 |          470 |       1711 |
| GCC -O2 (optimized)   |         1238 |          119 |       1357 |
| CLANG -O2 (optimized) |         2258 |          187 |       2445 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.51x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 2.22x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
