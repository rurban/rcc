# Windows RCC Benchmark Results

_Generated: 06/25/2026 09:24:24_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         3065 |          594 |       3659 |
| RCC -O1 (optimized)   |         1256 |          597 |       1853 |
| TCC (Tiny C Compiler) |         1252 |          429 |       1681 |
| GCC -O0 (no opt)      |         1265 |          419 |       1684 |
| GCC -O2 (optimized)   |         1256 |          116 |       1372 |
| CLANG -O2 (optimized) |         1254 |          169 |       1423 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 2.45x
- Execute speed : RCC/TCC = 1.38x
- Total : RCC/TCC = 2.18x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
