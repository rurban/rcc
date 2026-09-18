#define _POSIX_C_SOURCE 200809L
/* Benchmark: switch-dispatch lowering strategies.
 * Isolates dense-jump-table, sparse-binary-search, and sparse-perfect-
 * hash lowering, each driven with a realistic ~90% hit rate (dispatch
 * mostly lands on a real case, like a typical opcode/enum dispatch)
 * plus ~10% misses, so the branch predictor sees the same mixed
 * pattern real code would produce -- a benchmark that mostly misses
 * unfairly favors a trivially-predicted linear scan over any
 * tree-shaped dispatch, hit or miss, regardless of instruction count.
 *
 * Run with: rcc -O1 -o bench_switch bench_switch.c && ./bench_switch
 * Compare -O0 (always linear chain) against -O1 (dense/bsearch/hash) on
 * both an old and a new rcc binary to isolate this pass's own effect.
 */
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define N 20000000

/* 5 dense, contiguous cases -> qualifies for the x86 jump table. */
static int dense5(int x) {
    switch (x % 5) {
    case 0: return 10;
    case 1: return 11;
    case 2: return 12;
    case 3: return 13;
    case 4: return 14;
    default: return -1;
    }
}

/* 10 sparse exact cases, no case qualifies for the dense table
 * (span 100150 >> 10*3) -> binary search at -O1. */
static int sparse10(int x) {
    switch (x) {
    case 3: return 1;
    case 17: return 2;
    case 42: return 3;
    case 99: return 4;
    case 150: return 5;
    case 777: return 6;
    case 1000: return 7;
    case 5000: return 8;
    case 33000: return 9;
    case 100150: return 10;
    default: return -1;
    }
}

/* 40 sparse exact cases -> perfect hash at -O1 (>= 17, <= 256). */
static int sparse40(int x) {
    switch (x) {
    case 2: return 1;
    case 9: return 2;
    case 23: return 3;
    case 47: return 4;
    case 88: return 5;
    case 133: return 6;
    case 256: return 7;
    case 512: return 8;
    case 1024: return 9;
    case 2048: return 10;
    case 4096: return 11;
    case 8192: return 12;
    case 16384: return 13;
    case 32768: return 14;
    case 65536: return 15;
    case 131: return 16;
    case 271: return 17;
    case 401: return 18;
    case 541: return 19;
    case 683: return 20;
    case 811: return 21;
    case 947: return 22;
    case 1087: return 23;
    case 1229: return 24;
    case 1361: return 25;
    case 1499: return 26;
    case 1637: return 27;
    case 1777: return 28;
    case 1913: return 29;
    case 2053: return 30;
    case 3001: return 31;
    case 4001: return 32;
    case 5003: return 33;
    case 6007: return 34;
    case 7001: return 35;
    case 8009: return 36;
    case 9001: return 37;
    case 10007: return 38;
    case 11003: return 39;
    case 12007: return 40;
    default: return -1;
    }
}

/* 300 sparse exact cases: above the perfect-hash search's practical
 * budget (SWITCH_HASH_MAX_N=256) -> falls back to binary search at -O1
 * even on the new compiler too. */
static int sparse300(int x) {
    switch (x) {
    case 100599: return 1;
    case 103402: return 2;
    case 106814: return 3;
    case 107540: return 4;
    case 112038: return 5;
    case 120422: return 6;
    case 122056: return 7;
    case 126225: return 8;
    case 127824: return 9;
    case 131244: return 10;
    case 132938: return 11;
    case 133326: return 12;
    case 133659: return 13;
    case 145561: return 14;
    case 148050: return 15;
    case 149405: return 16;
    case 156802: return 17;
    case 158655: return 18;
    case 160738: return 19;
    case 161733: return 20;
    case 163556: return 21;
    case 166613: return 22;
    case 167136: return 23;
    case 169403: return 24;
    case 170674: return 25;
    case 172574: return 26;
    case 172933: return 27;
    case 174299: return 28;
    case 174441: return 29;
    case 174870: return 30;
    case 182582: return 31;
    case 182627: return 32;
    case 183667: return 33;
    case 189814: return 34;
    case 191161: return 35;
    case 195325: return 36;
    case 197251: return 37;
    case 198246: return 38;
    case 198907: return 39;
    case 201414: return 40;
    case 201639: return 41;
    case 203198: return 42;
    case 205907: return 43;
    case 207175: return 44;
    case 207473: return 45;
    case 211579: return 46;
    case 214576: return 47;
    case 214975: return 48;
    case 216739: return 49;
    case 216970: return 50;
    case 217301: return 51;
    case 220116: return 52;
    case 226882: return 53;
    case 230889: return 54;
    case 231869: return 55;
    case 234628: return 56;
    case 238739: return 57;
    case 245051: return 58;
    case 246316: return 59;
    case 246413: return 60;
    case 249811: return 61;
    case 260263: return 62;
    case 260265: return 63;
    case 263032: return 64;
    case 265840: return 65;
    case 267414: return 66;
    case 267753: return 67;
    case 269396: return 68;
    case 270555: return 69;
    case 271339: return 70;
    case 273148: return 71;
    case 279451: return 72;
    case 287330: return 73;
    case 299448: return 74;
    case 299659: return 75;
    case 300896: return 76;
    case 301629: return 77;
    case 308496: return 78;
    case 308573: return 79;
    case 310922: return 80;
    case 319684: return 81;
    case 321231: return 82;
    case 322086: return 83;
    case 322955: return 84;
    case 324643: return 85;
    case 325772: return 86;
    case 329258: return 87;
    case 329974: return 88;
    case 330283: return 89;
    case 330914: return 90;
    case 331148: return 91;
    case 334053: return 92;
    case 335612: return 93;
    case 338968: return 94;
    case 340062: return 95;
    case 340174: return 96;
    case 341292: return 97;
    case 343962: return 98;
    case 344098: return 99;
    case 349565: return 100;
    case 351083: return 101;
    case 352572: return 102;
    case 354801: return 103;
    case 356702: return 104;
    case 356787: return 105;
    case 358607: return 106;
    case 359947: return 107;
    case 360735: return 108;
    case 363626: return 109;
    case 375504: return 110;
    case 376183: return 111;
    case 377370: return 112;
    case 377746: return 113;
    case 377932: return 114;
    case 379786: return 115;
    case 379946: return 116;
    case 380746: return 117;
    case 383060: return 118;
    case 388389: return 119;
    case 391369: return 120;
    case 391476: return 121;
    case 391704: return 122;
    case 392004: return 123;
    case 403445: return 124;
    case 407419: return 125;
    case 407757: return 126;
    case 412942: return 127;
    case 422451: return 128;
    case 426858: return 129;
    case 429963: return 130;
    case 430776: return 131;
    case 439902: return 132;
    case 440035: return 133;
    case 446479: return 134;
    case 452944: return 135;
    case 454508: return 136;
    case 455784: return 137;
    case 456699: return 138;
    case 456778: return 139;
    case 460663: return 140;
    case 471507: return 141;
    case 472528: return 142;
    case 476417: return 143;
    case 479201: return 144;
    case 479580: return 145;
    case 480612: return 146;
    case 482554: return 147;
    case 488162: return 148;
    case 491559: return 149;
    case 492077: return 150;
    case 496922: return 151;
    case 497887: return 152;
    case 498382: return 153;
    case 498591: return 154;
    case 500156: return 155;
    case 503457: return 156;
    case 514850: return 157;
    case 518373: return 158;
    case 518801: return 159;
    case 520651: return 160;
    case 522179: return 161;
    case 526833: return 162;
    case 531071: return 163;
    case 539898: return 164;
    case 542417: return 165;
    case 542666: return 166;
    case 543143: return 167;
    case 543692: return 168;
    case 544154: return 169;
    case 549245: return 170;
    case 551989: return 171;
    case 555884: return 172;
    case 559381: return 173;
    case 571029: return 174;
    case 573417: return 175;
    case 575435: return 176;
    case 575763: return 177;
    case 581141: return 178;
    case 581741: return 179;
    case 584714: return 180;
    case 589710: return 181;
    case 590785: return 182;
    case 595631: return 183;
    case 595948: return 184;
    case 598369: return 185;
    case 608993: return 186;
    case 609597: return 187;
    case 612340: return 188;
    case 617488: return 189;
    case 623481: return 190;
    case 624902: return 191;
    case 629903: return 192;
    case 632323: return 193;
    case 632342: return 194;
    case 634277: return 195;
    case 639131: return 196;
    case 642717: return 197;
    case 653306: return 198;
    case 654816: return 199;
    case 656116: return 200;
    case 658582: return 201;
    case 660086: return 202;
    case 662275: return 203;
    case 662336: return 204;
    case 663054: return 205;
    case 665158: return 206;
    case 665427: return 207;
    case 665492: return 208;
    case 665579: return 209;
    case 671412: return 210;
    case 671858: return 211;
    case 676510: return 212;
    case 678856: return 213;
    case 680099: return 214;
    case 680828: return 215;
    case 684004: return 216;
    case 688508: return 217;
    case 688637: return 218;
    case 694731: return 219;
    case 694916: return 220;
    case 698782: return 221;
    case 704201: return 222;
    case 705397: return 223;
    case 711878: return 224;
    case 712982: return 225;
    case 716886: return 226;
    case 717024: return 227;
    case 717889: return 228;
    case 719176: return 229;
    case 724834: return 230;
    case 725380: return 231;
    case 728038: return 232;
    case 731262: return 233;
    case 733052: return 234;
    case 736059: return 235;
    case 738551: return 236;
    case 738720: return 237;
    case 748564: return 238;
    case 755674: return 239;
    case 757924: return 240;
    case 759176: return 241;
    case 761759: return 242;
    case 765822: return 243;
    case 766563: return 244;
    case 769987: return 245;
    case 770487: return 246;
    case 771088: return 247;
    case 772097: return 248;
    case 774079: return 249;
    case 777568: return 250;
    case 779514: return 251;
    case 781446: return 252;
    case 781453: return 253;
    case 785197: return 254;
    case 787277: return 255;
    case 790993: return 256;
    case 791798: return 257;
    case 793384: return 258;
    case 801474: return 259;
    case 802729: return 260;
    case 804314: return 261;
    case 806073: return 262;
    case 809570: return 263;
    case 813328: return 264;
    case 813536: return 265;
    case 814825: return 266;
    case 816751: return 267;
    case 817870: return 268;
    case 821590: return 269;
    case 823378: return 270;
    case 832052: return 271;
    case 835392: return 272;
    case 835911: return 273;
    case 838797: return 274;
    case 842225: return 275;
    case 847581: return 276;
    case 850800: return 277;
    case 852787: return 278;
    case 854639: return 279;
    case 855731: return 280;
    case 857168: return 281;
    case 858490: return 282;
    case 863587: return 283;
    case 864491: return 284;
    case 864544: return 285;
    case 865179: return 286;
    case 865388: return 287;
    case 867460: return 288;
    case 872246: return 289;
    case 876646: return 290;
    case 877572: return 291;
    case 881177: return 292;
    case 883300: return 293;
    case 887352: return 294;
    case 891952: return 295;
    case 892495: return 296;
    case 895667: return 297;
    case 897549: return 298;
    case 898975: return 299;
    case 899550: return 300;
    default: return -1;
    }
}

static int dense5_vals[5] = {
    0,
    1,
    2,
    3,
    4,
};

static int sparse10_vals[10] = {
    3,
    17,
    42,
    99,
    150,
    777,
    1000,
    5000,
    33000,
    100150,
};

static int sparse40_vals[40] = {
    2,
    9,
    23,
    47,
    88,
    133,
    256,
    512,
    1024,
    2048,
    4096,
    8192,
    16384,
    32768,
    65536,
    131,
    271,
    401,
    541,
    683,
    811,
    947,
    1087,
    1229,
    1361,
    1499,
    1637,
    1777,
    1913,
    2053,
    3001,
    4001,
    5003,
    6007,
    7001,
    8009,
    9001,
    10007,
    11003,
    12007,
};

static int sparse300_vals[300] = {
    100599,
    103402,
    106814,
    107540,
    112038,
    120422,
    122056,
    126225,
    127824,
    131244,
    132938,
    133326,
    133659,
    145561,
    148050,
    149405,
    156802,
    158655,
    160738,
    161733,
    163556,
    166613,
    167136,
    169403,
    170674,
    172574,
    172933,
    174299,
    174441,
    174870,
    182582,
    182627,
    183667,
    189814,
    191161,
    195325,
    197251,
    198246,
    198907,
    201414,
    201639,
    203198,
    205907,
    207175,
    207473,
    211579,
    214576,
    214975,
    216739,
    216970,
    217301,
    220116,
    226882,
    230889,
    231869,
    234628,
    238739,
    245051,
    246316,
    246413,
    249811,
    260263,
    260265,
    263032,
    265840,
    267414,
    267753,
    269396,
    270555,
    271339,
    273148,
    279451,
    287330,
    299448,
    299659,
    300896,
    301629,
    308496,
    308573,
    310922,
    319684,
    321231,
    322086,
    322955,
    324643,
    325772,
    329258,
    329974,
    330283,
    330914,
    331148,
    334053,
    335612,
    338968,
    340062,
    340174,
    341292,
    343962,
    344098,
    349565,
    351083,
    352572,
    354801,
    356702,
    356787,
    358607,
    359947,
    360735,
    363626,
    375504,
    376183,
    377370,
    377746,
    377932,
    379786,
    379946,
    380746,
    383060,
    388389,
    391369,
    391476,
    391704,
    392004,
    403445,
    407419,
    407757,
    412942,
    422451,
    426858,
    429963,
    430776,
    439902,
    440035,
    446479,
    452944,
    454508,
    455784,
    456699,
    456778,
    460663,
    471507,
    472528,
    476417,
    479201,
    479580,
    480612,
    482554,
    488162,
    491559,
    492077,
    496922,
    497887,
    498382,
    498591,
    500156,
    503457,
    514850,
    518373,
    518801,
    520651,
    522179,
    526833,
    531071,
    539898,
    542417,
    542666,
    543143,
    543692,
    544154,
    549245,
    551989,
    555884,
    559381,
    571029,
    573417,
    575435,
    575763,
    581141,
    581741,
    584714,
    589710,
    590785,
    595631,
    595948,
    598369,
    608993,
    609597,
    612340,
    617488,
    623481,
    624902,
    629903,
    632323,
    632342,
    634277,
    639131,
    642717,
    653306,
    654816,
    656116,
    658582,
    660086,
    662275,
    662336,
    663054,
    665158,
    665427,
    665492,
    665579,
    671412,
    671858,
    676510,
    678856,
    680099,
    680828,
    684004,
    688508,
    688637,
    694731,
    694916,
    698782,
    704201,
    705397,
    711878,
    712982,
    716886,
    717024,
    717889,
    719176,
    724834,
    725380,
    728038,
    731262,
    733052,
    736059,
    738551,
    738720,
    748564,
    755674,
    757924,
    759176,
    761759,
    765822,
    766563,
    769987,
    770487,
    771088,
    772097,
    774079,
    777568,
    779514,
    781446,
    781453,
    785197,
    787277,
    790993,
    791798,
    793384,
    801474,
    802729,
    804314,
    806073,
    809570,
    813328,
    813536,
    814825,
    816751,
    817870,
    821590,
    823378,
    832052,
    835392,
    835911,
    838797,
    842225,
    847581,
    850800,
    852787,
    854639,
    855731,
    857168,
    858490,
    863587,
    864491,
    864544,
    865179,
    865388,
    867460,
    872246,
    876646,
    877572,
    881177,
    883300,
    887352,
    891952,
    892495,
    895667,
    897549,
    898975,
    899550,
};

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Drives f() with ~90% real case values (cycled from vals[]) and ~10%
 * random misses (a value never fed to any case), matching a typical
 * opcode/enum dispatch's hit rate far better than an all-miss stream. */
static void run(const char *name, int (*f)(int), const int *vals, int nvals, int miss_base) {
    double t0 = now_sec();
    long sum = 0;
    unsigned s = 12345;
    for (int i = 0; i < N; i++) {
        s = s * 1103515245u + 12345u;
        int x;
        if (s % 10u < 9u)
            x = vals[(s / 10u) % (unsigned)nvals];
        else
            x = miss_base + (int)(s % 97u); /* never a real case value */
        sum += f(x);
    }
    double t1 = now_sec();
    printf("%-10s sum=%-12ld total=%.4fs  ns/call=%.2f\n",
           name, sum, t1 - t0, (t1 - t0) * 1e9 / N);
}

int main(void) {
    run("dense5", dense5, dense5_vals, 5, 1000);
    run("sparse10", sparse10, sparse10_vals, 10, -1000);
    run("sparse40", sparse40, sparse40_vals, 40, -1000);
    run("sparse300", sparse300, sparse300_vals, 300, -1000);
    return 0;
}
