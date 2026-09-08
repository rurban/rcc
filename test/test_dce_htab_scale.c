// Regression test: eliminate_unused_static_inline()'s per-name Function
// lookup (opt.c dce_lookup()) was rewritten from an O(n) strcmp linear
// scan over every static-inline candidate to a content-hashed table
// (dce_htab_build()/dce_lookup()) to fix a compile-time quadratic blowup
// on large single-TU files -- compiling the sqlite3.c amalgamation spent
// ~16% of total compile time in strcmp calls from this exact path (found
// via `perf record`; see bench/bench_report.md).
//
// This exercises the new hash table at a scale (128 distinctly-named
// `static inline` candidates, chained by call, interleaved with unused
// ones so live fns[] indices are scattered, not clustered at one end) too
// large to pass by accident: a mis-hashed bucket, a corrupted chain link,
// or a broken fns[]-index tie-break on a bucket collision drops a live
// function from the chain, which fails to link ("undefined reference")
// rather than silently miscomputing -- so this is a strong regression
// check for the rewrite even though DCE itself is invisible at runtime.
#include <assert.h>

static inline int chain_127(void) { return 0; }
static inline int chain_126(void) { return chain_127() + 1; }
static inline int chain_125(void) { return chain_126() + 1; }
static inline int chain_124(void) { return chain_125() + 1; }
static inline int dead_124(void) { return chain_124() * -1 - 1; }
static inline int chain_123(void) { return chain_124() + 1; }
static inline int chain_122(void) { return chain_123() + 1; }
static inline int chain_121(void) { return chain_122() + 1; }
static inline int chain_120(void) { return chain_121() + 1; }
static inline int dead_120(void) { return chain_120() * -1 - 1; }
static inline int chain_119(void) { return chain_120() + 1; }
static inline int chain_118(void) { return chain_119() + 1; }
static inline int chain_117(void) { return chain_118() + 1; }
static inline int chain_116(void) { return chain_117() + 1; }
static inline int dead_116(void) { return chain_116() * -1 - 1; }
static inline int chain_115(void) { return chain_116() + 1; }
static inline int chain_114(void) { return chain_115() + 1; }
static inline int chain_113(void) { return chain_114() + 1; }
static inline int chain_112(void) { return chain_113() + 1; }
static inline int dead_112(void) { return chain_112() * -1 - 1; }
static inline int chain_111(void) { return chain_112() + 1; }
static inline int chain_110(void) { return chain_111() + 1; }
static inline int chain_109(void) { return chain_110() + 1; }
static inline int chain_108(void) { return chain_109() + 1; }
static inline int dead_108(void) { return chain_108() * -1 - 1; }
static inline int chain_107(void) { return chain_108() + 1; }
static inline int chain_106(void) { return chain_107() + 1; }
static inline int chain_105(void) { return chain_106() + 1; }
static inline int chain_104(void) { return chain_105() + 1; }
static inline int dead_104(void) { return chain_104() * -1 - 1; }
static inline int chain_103(void) { return chain_104() + 1; }
static inline int chain_102(void) { return chain_103() + 1; }
static inline int chain_101(void) { return chain_102() + 1; }
static inline int chain_100(void) { return chain_101() + 1; }
static inline int dead_100(void) { return chain_100() * -1 - 1; }
static inline int chain_099(void) { return chain_100() + 1; }
static inline int chain_098(void) { return chain_099() + 1; }
static inline int chain_097(void) { return chain_098() + 1; }
static inline int chain_096(void) { return chain_097() + 1; }
static inline int dead_096(void) { return chain_096() * -1 - 1; }
static inline int chain_095(void) { return chain_096() + 1; }
static inline int chain_094(void) { return chain_095() + 1; }
static inline int chain_093(void) { return chain_094() + 1; }
static inline int chain_092(void) { return chain_093() + 1; }
static inline int dead_092(void) { return chain_092() * -1 - 1; }
static inline int chain_091(void) { return chain_092() + 1; }
static inline int chain_090(void) { return chain_091() + 1; }
static inline int chain_089(void) { return chain_090() + 1; }
static inline int chain_088(void) { return chain_089() + 1; }
static inline int dead_088(void) { return chain_088() * -1 - 1; }
static inline int chain_087(void) { return chain_088() + 1; }
static inline int chain_086(void) { return chain_087() + 1; }
static inline int chain_085(void) { return chain_086() + 1; }
static inline int chain_084(void) { return chain_085() + 1; }
static inline int dead_084(void) { return chain_084() * -1 - 1; }
static inline int chain_083(void) { return chain_084() + 1; }
static inline int chain_082(void) { return chain_083() + 1; }
static inline int chain_081(void) { return chain_082() + 1; }
static inline int chain_080(void) { return chain_081() + 1; }
static inline int dead_080(void) { return chain_080() * -1 - 1; }
static inline int chain_079(void) { return chain_080() + 1; }
static inline int chain_078(void) { return chain_079() + 1; }
static inline int chain_077(void) { return chain_078() + 1; }
static inline int chain_076(void) { return chain_077() + 1; }
static inline int dead_076(void) { return chain_076() * -1 - 1; }
static inline int chain_075(void) { return chain_076() + 1; }
static inline int chain_074(void) { return chain_075() + 1; }
static inline int chain_073(void) { return chain_074() + 1; }
static inline int chain_072(void) { return chain_073() + 1; }
static inline int dead_072(void) { return chain_072() * -1 - 1; }
static inline int chain_071(void) { return chain_072() + 1; }
static inline int chain_070(void) { return chain_071() + 1; }
static inline int chain_069(void) { return chain_070() + 1; }
static inline int chain_068(void) { return chain_069() + 1; }
static inline int dead_068(void) { return chain_068() * -1 - 1; }
static inline int chain_067(void) { return chain_068() + 1; }
static inline int chain_066(void) { return chain_067() + 1; }
static inline int chain_065(void) { return chain_066() + 1; }
static inline int chain_064(void) { return chain_065() + 1; }
static inline int dead_064(void) { return chain_064() * -1 - 1; }
static inline int chain_063(void) { return chain_064() + 1; }
static inline int chain_062(void) { return chain_063() + 1; }
static inline int chain_061(void) { return chain_062() + 1; }
static inline int chain_060(void) { return chain_061() + 1; }
static inline int dead_060(void) { return chain_060() * -1 - 1; }
static inline int chain_059(void) { return chain_060() + 1; }
static inline int chain_058(void) { return chain_059() + 1; }
static inline int chain_057(void) { return chain_058() + 1; }
static inline int chain_056(void) { return chain_057() + 1; }
static inline int dead_056(void) { return chain_056() * -1 - 1; }
static inline int chain_055(void) { return chain_056() + 1; }
static inline int chain_054(void) { return chain_055() + 1; }
static inline int chain_053(void) { return chain_054() + 1; }
static inline int chain_052(void) { return chain_053() + 1; }
static inline int dead_052(void) { return chain_052() * -1 - 1; }
static inline int chain_051(void) { return chain_052() + 1; }
static inline int chain_050(void) { return chain_051() + 1; }
static inline int chain_049(void) { return chain_050() + 1; }
static inline int chain_048(void) { return chain_049() + 1; }
static inline int dead_048(void) { return chain_048() * -1 - 1; }
static inline int chain_047(void) { return chain_048() + 1; }
static inline int chain_046(void) { return chain_047() + 1; }
static inline int chain_045(void) { return chain_046() + 1; }
static inline int chain_044(void) { return chain_045() + 1; }
static inline int dead_044(void) { return chain_044() * -1 - 1; }
static inline int chain_043(void) { return chain_044() + 1; }
static inline int chain_042(void) { return chain_043() + 1; }
static inline int chain_041(void) { return chain_042() + 1; }
static inline int chain_040(void) { return chain_041() + 1; }
static inline int dead_040(void) { return chain_040() * -1 - 1; }
static inline int chain_039(void) { return chain_040() + 1; }
static inline int chain_038(void) { return chain_039() + 1; }
static inline int chain_037(void) { return chain_038() + 1; }
static inline int chain_036(void) { return chain_037() + 1; }
static inline int dead_036(void) { return chain_036() * -1 - 1; }
static inline int chain_035(void) { return chain_036() + 1; }
static inline int chain_034(void) { return chain_035() + 1; }
static inline int chain_033(void) { return chain_034() + 1; }
static inline int chain_032(void) { return chain_033() + 1; }
static inline int dead_032(void) { return chain_032() * -1 - 1; }
static inline int chain_031(void) { return chain_032() + 1; }
static inline int chain_030(void) { return chain_031() + 1; }
static inline int chain_029(void) { return chain_030() + 1; }
static inline int chain_028(void) { return chain_029() + 1; }
static inline int dead_028(void) { return chain_028() * -1 - 1; }
static inline int chain_027(void) { return chain_028() + 1; }
static inline int chain_026(void) { return chain_027() + 1; }
static inline int chain_025(void) { return chain_026() + 1; }
static inline int chain_024(void) { return chain_025() + 1; }
static inline int dead_024(void) { return chain_024() * -1 - 1; }
static inline int chain_023(void) { return chain_024() + 1; }
static inline int chain_022(void) { return chain_023() + 1; }
static inline int chain_021(void) { return chain_022() + 1; }
static inline int chain_020(void) { return chain_021() + 1; }
static inline int dead_020(void) { return chain_020() * -1 - 1; }
static inline int chain_019(void) { return chain_020() + 1; }
static inline int chain_018(void) { return chain_019() + 1; }
static inline int chain_017(void) { return chain_018() + 1; }
static inline int chain_016(void) { return chain_017() + 1; }
static inline int dead_016(void) { return chain_016() * -1 - 1; }
static inline int chain_015(void) { return chain_016() + 1; }
static inline int chain_014(void) { return chain_015() + 1; }
static inline int chain_013(void) { return chain_014() + 1; }
static inline int chain_012(void) { return chain_013() + 1; }
static inline int dead_012(void) { return chain_012() * -1 - 1; }
static inline int chain_011(void) { return chain_012() + 1; }
static inline int chain_010(void) { return chain_011() + 1; }
static inline int chain_009(void) { return chain_010() + 1; }
static inline int chain_008(void) { return chain_009() + 1; }
static inline int dead_008(void) { return chain_008() * -1 - 1; }
static inline int chain_007(void) { return chain_008() + 1; }
static inline int chain_006(void) { return chain_007() + 1; }
static inline int chain_005(void) { return chain_006() + 1; }
static inline int chain_004(void) { return chain_005() + 1; }
static inline int dead_004(void) { return chain_004() * -1 - 1; }
static inline int chain_003(void) { return chain_004() + 1; }
static inline int chain_002(void) { return chain_003() + 1; }
static inline int chain_001(void) { return chain_002() + 1; }
static inline int chain_000(void) { return chain_001() + 1; }

int main(void) {
    assert(chain_000() == 127);
    return 0;
}
