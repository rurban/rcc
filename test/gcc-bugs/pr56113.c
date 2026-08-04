/* GCC Bug #56113 - out of memory when compiling a function with many goto labels (> 50k)
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56113
 */


<span class="quote">>     ./goto_gen.py ${n} t.c
// return statement) that consume a significant amount of memory.
// Let's assume we need about 6 times the 131MB to to account for all stmt
// operands and for PHI nodes. That's ~1GB total for the function body.
// * Everything else is relatively small. Per computed goto there are
// initially 7 basic blocks and 8 edges, and 8 label_decls.  On a 64bit
// host, sizeof(struct basic_block_def)=108, sizeof(struct edge_def)=56,
// and sizeof(struct tree_label_decl)=128.
// With alignment that's 128, 64, and 128.
// That's 20000*(7*128+8*64+8*128)=46MB for the CFG and labels.
// Nothing else of any significance lives in GC memory early in the
// compilation pipeline.
// * Function bodies as RTL tend to be much smaller than GIMPLE, accounting
// for maybe 200MB or so in this case (1/5th of the size of the GIMPLE body
// is typical for pre-processed GCC itself).
// * The above estimates are supported by -ftime-report's total allocated
// GC memory counter: 1476435kB. That means the other 4.8GB is allocated
// in non-GC space, and that non-GC memory dominates the memory footprint.
// For n=10000, max. GC=741582kB, and max. resident is 1.7GB.  So here,
// too, ~1GB of on-GC memory dominates the memory footprint.  For n=40000
// peak memory is ~23.4GB so far. So tabulated:
// n      max. mem    max. GC mem    max. non-GC mem
// 10000  1.7GB        741582kB       ~1GB
// 20000  6.3GB       1476435kB       ~4.8GB
// 40000  23.4GB     ~2900000kB       ~20.5GB
// A compiler built with --enable-gather-detailed-mem-stats should be able
// to tell where and for what that memory is allocated. The peak memory
// usage happens pretty early in the compilation process, so I'm guessing
// the memory is allocated for PTA.


