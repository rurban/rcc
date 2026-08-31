# Are-We-Fast-Yet

A runtime (generated-code-quality) benchmark suite, as opposed to
`bench/run_bench.sh`'s compiler-compile-speed benchmarks.

Source: [rochus-keller/Are-we-fast-yet](https://github.com/rochus-keller/Are-we-fast-yet/tree/main/C),
a C99 port of the ["Are We Fast Yet?"](https://github.com/smarr/are-we-fast-yet)
cross-language benchmark suite. All files here (including `som/`) are
vendored unmodified from upstream: 14 benchmarks --

- **DeltaBlue** -- incremental constraint solver (tagged unions)
- **Richards** -- OS process-scheduler simulation
- **Json** -- JSON parser (virtual-table dispatch)
- **Havlak** -- loop-finding dataflow analysis on a synthetic CFG
- **CD** -- air-traffic-control collision detection
- **Bounce**, **List**, **Permute**, **Queens**, **Sieve**, **Storage**,
  **Towers** -- classic small OO/algorithmic kernels
- **Mandelbrot**, **NBody** -- floating-point-heavy numeric kernels

covering a wide range of C idioms: linked lists, tagged unions,
vtable-style polymorphism, hash maps/dictionaries, red-black trees,
recursive descent, and floating point -- a broad generated-code
correctness and performance cross-check for rcc.

## Usage

```
./bench/awfy/run.sh [rcc-binary]
```

Builds and runs the full suite (`main.c`'s `runAll()`) with rcc at
`-O0`/`-O1`/`-O2`, then with gcc `-O2` as a generated-code-speed
baseline.

## License

MIT, Copyright (c) 2024 Rochus Keller (C99 port); individual
benchmarks derived from the original "Are We Fast Yet?" suite
authors (see upstream's AUTHORS.md).
