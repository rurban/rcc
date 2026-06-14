# Windows RCC Benchmark Results

_Generated: 06/14/2026 13:56:05_

| Compiler              | Compile (ms) | Execute (ms) | Total (ms) |
| :-------------------- | -----------: | -----------: | ---------: |
| RCC (your compiler)   |        10105 |          609 |      10714 |
| RCC -O1 (optimized)   |         1253 |          620 |       1873 |
| TCC (Tiny C Compiler) |         1260 |          442 |       1702 |
| GCC -O0 (no opt)      |         1267 |          417 |       1684 |
| GCC -O2 (optimized)   |         1244 |          117 |       1361 |
| CLANG -O2 (optimized) |         1305 |          178 |       1483 |

## Windows RCC vs TCC Head-to-Head

- Compile speed : RCC/TCC = 8.02x
- Execute speed : RCC/TCC = 1.38x
- Total : RCC/TCC = 6.29x

## Output Correctness

- RCC (your compiler): OK
- RCC -O1 (optimized): OK
- TCC (Tiny C Compiler): OK
- GCC -O0 (no opt): OK
- GCC -O2 (optimized): OK
- CLANG -O2 (optimized): OK
