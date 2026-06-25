# Windows RCC Benchmark Results

_Generated: 06/25/2026 17:12:51_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |         6113 |          735 |       6848 |
| RCC -O1 (optimized)   |         1256 |          735 |       1991 |
| TCC (Tiny C Compiler) |         1239 |          498 |       1737 |
| GCC -O0 (no opt)      |         1241 |          467 |       1708 |
| GCC -O2 (optimized)   |         1245 |          118 |       1363 |
| CLANG -O2 (optimized) |         2707 |          188 |       2895 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 4.93x
- Execute speed : RCC/TCC = 1.48x
- Total : RCC/TCC = 3.94x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
