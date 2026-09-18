/* -O1 sparse switch lowering (codegen.c's switch_emit_bsearch /
 * switch_emit_hash_dispatch, added alongside the dense small-int jump
 * table): a switch with enough non-range cases to skip the dense jump
 * table (sparse values, or too many/too spread out) lowers to a
 * compile-time perfect hash (150-256 cases) or, above that, a sorted
 * binary search -- instead of a linear cmp/jcc chain. The 150-case
 * floor itself is evidence-based: bench/bench_switch.c measured both
 * strategies as a net LOSS against the plain linear chain below
 * roughly 100-130 cases (a balanced tree's/hash's own branches are far
 * less predictable than a long run of heavily-biased "not this one"
 * compares), so these regression cases are deliberately sized at
 * 150-300 to land past that floor and actually exercise the new
 * lowering, not just re-verify the untouched linear chain.
 *
 * Also exercises a real bug found building this pass: ARM64's
 * SUBS-based `cmp` immediate only encodes an unsigned 12-bit value,
 * optionally left-shifted by 12 (a multiple of 4096 up to 0xffffff).
 * asm_cmp_imm() didn't check this and always encoded with shift 0, so
 * a case value like 4096 silently truncated to `cmp w, #0` -- wrong on
 * every input. Predates this pass entirely (see
 * test_switch_case_4096.c for the minimal, non-range repro); the large
 * generated case values below (in the hundreds of thousands to
 * trillions) exercise it at scale too.
 */
#include <stdlib.h>

/* 200 sparse int cases (hash tier: 150 <= n <= 256). */
static int sparse_hash200(int x) {
    switch (x) {
    case -495474: return 1; case -479636: return 2; case -457547: return 3;
    case -452175: return 4; case -438674: return 5; case -431371: return 6;
    case -425711: return 7; case -421701: return 8; case -414606: return 9;
    case -397250: return 10; case -394502: return 11; case -382524: return 12;
    case -369785: return 13; case -361578: return 14; case -357638: return 15;
    case -357285: return 16; case -354284: return 17; case -351735: return 18;
    case -344422: return 19; case -342023: return 20; case -321936: return 21;
    case -317823: return 22; case -315327: return 23; case -309468: return 24;
    case -295066: return 25; case -294850: return 26; case -290074: return 27;
    case -289304: return 28; case -289082: return 29; case -287043: return 30;
    case -285576: return 31; case -283593: return 32; case -283039: return 33;
    case -277062: return 34; case -275545: return 35; case -271278: return 36;
    case -270691: return 37; case -265920: return 38; case -263841: return 39;
    case -260354: return 40; case -259769: return 41; case -258196: return 42;
    case -252867: return 43; case -252452: return 44; case -249092: return 45;
    case -242925: return 46; case -237828: return 47; case -228547: return 48;
    case -225344: return 49; case -223569: return 50; case -221896: return 51;
    case -208051: return 52; case -201953: return 53; case -183987: return 54;
    case -182805: return 55; case -181494: return 56; case -175436: return 57;
    case -168174: return 58; case -165270: return 59; case -164510: return 60;
    case -158707: return 61; case -156993: return 62; case -153720: return 63;
    case -153104: return 64; case -152788: return 65; case -152069: return 66;
    case -148202: return 67; case -146358: return 68; case -137931: return 69;
    case -133581: return 70; case -132700: return 71; case -128624: return 72;
    case -128164: return 73; case -116525: return 74; case -113308: return 75;
    case -104732: return 76; case -93917: return 77; case -92525: return 78;
    case -90735: return 79; case -73256: return 80; case -69991: return 81;
    case -68081: return 82; case -65487: return 83; case -64941: return 84;
    case -63927: return 85; case -63195: return 86; case -58498: return 87;
    case -52461: return 88; case -42803: return 89; case -40579: return 90;
    case -16434: return 91; case -13184: return 92; case -12454: return 93;
    case -11488: return 94; case -8708: return 95; case -7075: return 96;
    case 5461: return 97; case 17306: return 98; case 19275: return 99;
    case 22506: return 100; case 30202: return 101; case 40297: return 102;
    case 44578: return 103; case 49137: return 104; case 52928: return 105;
    case 52944: return 106; case 57431: return 107; case 59080: return 108;
    case 70249: return 109; case 70592: return 110; case 91962: return 111;
    case 95264: return 112; case 97864: return 113; case 106446: return 114;
    case 106977: return 115; case 109244: return 116; case 124413: return 117;
    case 130932: return 118; case 136169: return 119; case 143496: return 120;
    case 145557: return 121; case 153937: return 122; case 158608: return 123;
    case 159209: return 124; case 167049: return 125; case 184217: return 126;
    case 184435: return 127; case 193742: return 128; case 196115: return 129;
    case 210956: return 130; case 217303: return 131; case 219924: return 132;
    case 222702: return 133; case 228804: return 134; case 229378: return 135;
    case 236672: return 136; case 238029: return 137; case 238260: return 138;
    case 239410: return 139; case 242945: return 140; case 243476: return 141;
    case 247376: return 142; case 251424: return 143; case 255459: return 144;
    case 258660: return 145; case 260549: return 146; case 263036: return 147;
    case 263110: return 148; case 263640: return 149; case 266835: return 150;
    case 266886: return 151; case 270957: return 152; case 277412: return 153;
    case 279620: return 154; case 286336: return 155; case 286565: return 156;
    case 289148: return 157; case 293999: return 158; case 295476: return 159;
    case 298928: return 160; case 299865: return 161; case 305119: return 162;
    case 308126: return 163; case 309925: return 164; case 311989: return 165;
    case 318276: return 166; case 322416: return 167; case 332211: return 168;
    case 338627: return 169; case 342016: return 170; case 343454: return 171;
    case 352053: return 172; case 371895: return 173; case 377148: return 174;
    case 383665: return 175; case 384533: return 176; case 384954: return 177;
    case 391470: return 178; case 392696: return 179; case 403017: return 180;
    case 403070: return 181; case 407450: return 182; case 409223: return 183;
    case 409598: return 184; case 409954: return 185; case 414260: return 186;
    case 422257: return 187; case 430399: return 188; case 434294: return 189;
    case 434711: return 190; case 443581: return 191; case 444683: return 192;
    case 449005: return 193; case 469432: return 194; case 469587: return 195;
    case 477308: return 196; case 481605: return 197; case 485715: return 198;
    case 486846: return 199; case 496785: return 200;
    default: return -1;
    }
}

/* 300 sparse int cases: above the perfect-hash search's practical
 * budget (SWITCH_HASH_MAX_N=256) -> always binary search. */
static int sparse_bsearch300(int x) {
    switch (x) {
    case 1000459: return 1; case 1003895: return 2; case 1024522: return 3;
    case 1089345: return 4; case 1124918: return 5; case 1212120: return 6;
    case 1219952: return 7; case 1242596: return 8; case 1253145: return 9;
    case 1260791: return 10; case 1312775: return 11; case 1315341: return 12;
    case 1419026: return 13; case 1451412: return 14; case 1456168: return 15;
    case 1467802: return 16; case 1515748: return 17; case 1566331: return 18;
    case 1615454: return 19; case 1728032: return 20; case 1737135: return 21;
    case 1781266: return 22; case 1799201: return 23; case 1861477: return 24;
    case 1870724: return 25; case 1874909: return 26; case 1901692: return 27;
    case 1905629: return 28; case 1906427: return 29; case 1934342: return 30;
    case 1986787: return 31; case 1994474: return 32; case 2000302: return 33;
    case 2000341: return 34; case 2019919: return 35; case 2030661: return 36;
    case 2043735: return 37; case 2051218: return 38; case 2052547: return 39;
    case 2117581: return 40; case 2122199: return 41; case 2125890: return 42;
    case 2129134: return 43; case 2153191: return 44; case 2322494: return 45;
    case 2336985: return 46; case 2342298: return 47; case 2345329: return 48;
    case 2353874: return 49; case 2359253: return 50; case 2384706: return 51;
    case 2449547: return 52; case 2457595: return 53; case 2467619: return 54;
    case 2505133: return 55; case 2508532: return 56; case 2515470: return 57;
    case 2528804: return 58; case 2541984: return 59; case 2561606: return 60;
    case 2594637: return 61; case 2626390: return 62; case 2652016: return 63;
    case 2667806: return 64; case 2743617: return 65; case 2757190: return 66;
    case 2757390: return 67; case 2763062: return 68; case 2763629: return 69;
    case 2781388: return 70; case 2796687: return 71; case 2822684: return 72;
    case 2847629: return 73; case 2870156: return 74; case 2888259: return 75;
    case 2903400: return 76; case 2914137: return 77; case 2925591: return 78;
    case 2937018: return 79; case 2937191: return 80; case 2981826: return 81;
    case 2996812: return 82; case 3074544: return 83; case 3075296: return 84;
    case 3115719: return 85; case 3117472: return 86; case 3144787: return 87;
    case 3205024: return 88; case 3230926: return 89; case 3287631: return 90;
    case 3293632: return 91; case 3318746: return 92; case 3356864: return 93;
    case 3359188: return 94; case 3362755: return 95; case 3364566: return 96;
    case 3389640: return 97; case 3401747: return 98; case 3448076: return 99;
    case 3488742: return 100; case 3579044: return 101; case 3588754: return 102;
    case 3614302: return 103; case 3631190: return 104; case 3640226: return 105;
    case 3653473: return 106; case 3664191: return 107; case 3669480: return 108;
    case 3737554: return 109; case 3785372: return 110; case 3813095: return 111;
    case 3836469: return 112; case 3852022: return 113; case 3992048: return 114;
    case 3993477: return 115; case 4003577: return 116; case 4044209: return 117;
    case 4048597: return 118; case 4060036: return 119; case 4169573: return 120;
    case 4192550: return 121; case 4266670: return 122; case 4337826: return 123;
    case 4339793: return 124; case 4357150: return 125; case 4392758: return 126;
    case 4413752: return 127; case 4469171: return 128; case 4509979: return 129;
    case 4516675: return 130; case 4533067: return 131; case 4571093: return 132;
    case 4615447: return 133; case 4618693: return 134; case 4620236: return 135;
    case 4639443: return 136; case 4640967: return 137; case 4654297: return 138;
    case 4683604: return 139; case 4738636: return 140; case 4764244: return 141;
    case 4774187: return 142; case 4807485: return 143; case 4819571: return 144;
    case 4866942: return 145; case 4888572: return 146; case 4897489: return 147;
    case 4911353: return 148; case 4920897: return 149; case 4928824: return 150;
    case 4967626: return 151; case 5024609: return 152; case 5094058: return 153;
    case 5126675: return 154; case 5152453: return 155; case 5164199: return 156;
    case 5205696: return 157; case 5229034: return 158; case 5238086: return 159;
    case 5238527: return 160; case 5252069: return 161; case 5276597: return 162;
    case 5314379: return 163; case 5343084: return 164; case 5345400: return 165;
    case 5349445: return 166; case 5379489: return 167; case 5490987: return 168;
    case 5511749: return 169; case 5537977: return 170; case 5558434: return 171;
    case 5565795: return 172; case 5590358: return 173; case 5630617: return 174;
    case 5639264: return 175; case 5651910: return 176; case 5688086: return 177;
    case 5697898: return 178; case 5711681: return 179; case 5757656: return 180;
    case 5778469: return 181; case 5805393: return 182; case 5806898: return 183;
    case 5807920: return 184; case 5858365: return 185; case 5897329: return 186;
    case 5898410: return 187; case 5918494: return 188; case 5961424: return 189;
    case 5984260: return 190; case 5986046: return 191; case 5994452: return 192;
    case 6013091: return 193; case 6017923: return 194; case 6069470: return 195;
    case 6070667: return 196; case 6095798: return 197; case 6104585: return 198;
    case 6144705: return 199; case 6158543: return 200; case 6182373: return 201;
    case 6199795: return 202; case 6242015: return 203; case 6297496: return 204;
    case 6313570: return 205; case 6320897: return 206; case 6326646: return 207;
    case 6329692: return 208; case 6340776: return 209; case 6347376: return 210;
    case 6406886: return 211; case 6408168: return 212; case 6408214: return 213;
    case 6411385: return 214; case 6434850: return 215; case 6462215: return 216;
    case 6472679: return 217; case 6524537: return 218; case 6533432: return 219;
    case 6587662: return 220; case 6649273: return 221; case 6659823: return 222;
    case 6665652: return 223; case 6748708: return 224; case 6759870: return 225;
    case 6772789: return 226; case 6857900: return 227; case 6857983: return 228;
    case 6897458: return 229; case 6914849: return 230; case 6921090: return 231;
    case 7005824: return 232; case 7139122: return 233; case 7145477: return 234;
    case 7149474: return 235; case 7159083: return 236; case 7198267: return 237;
    case 7309162: return 238; case 7313499: return 239; case 7382822: return 240;
    case 7407207: return 241; case 7410512: return 242; case 7418699: return 243;
    case 7461448: return 244; case 7482944: return 245; case 7539541: return 246;
    case 7614582: return 247; case 7617086: return 248; case 7672588: return 249;
    case 7701893: return 250; case 7754384: return 251; case 7755756: return 252;
    case 7759707: return 253; case 7759754: return 254; case 7772964: return 255;
    case 7796924: return 256; case 7806403: return 257; case 7815591: return 258;
    case 7837572: return 259; case 7901168: return 260; case 7912308: return 261;
    case 7933327: return 262; case 7939222: return 263; case 7958728: return 264;
    case 7959629: return 265; case 7983877: return 266; case 8001311: return 267;
    case 8099777: return 268; case 8191962: return 269; case 8225255: return 270;
    case 8245399: return 271; case 8270377: return 272; case 8293904: return 273;
    case 8368613: return 274; case 8401987: return 275; case 8405565: return 276;
    case 8464516: return 277; case 8475023: return 278; case 8494231: return 279;
    case 8540608: return 280; case 8549887: return 281; case 8605711: return 282;
    case 8643096: return 283; case 8646950: return 284; case 8687500: return 285;
    case 8708005: return 286; case 8748336: return 287; case 8749456: return 288;
    case 8758981: return 289; case 8768833: return 290; case 8869653: return 291;
    case 8880668: return 292; case 8883380: return 293; case 8901040: return 294;
    case 8912752: return 295; case 8915467: return 296; case 8921736: return 297;
    case 8928883: return 298; case 8939398: return 299; case 8946086: return 300;
    default: return -1;
    }
}

/* 180 sparse 8-byte cases (hash tier): exercises the 64-bit
 * sign-extension path, including negative and >32-bit values. */
static long long sparse_long180(long long x) {
    switch (x) {
    case -1942605073660LL: return 1; case -1940797503532LL: return 2; case -1934764433686LL: return 3;
    case -1934074036522LL: return 4; case -1932130358930LL: return 5; case -1922210203227LL: return 6;
    case -1891540020681LL: return 7; case -1859784502982LL: return 8; case -1826787800859LL: return 9;
    case -1815987644732LL: return 10; case -1783434500346LL: return 11; case -1778676992367LL: return 12;
    case -1767423585885LL: return 13; case -1736847965099LL: return 14; case -1715853031791LL: return 15;
    case -1698209076654LL: return 16; case -1693505811596LL: return 17; case -1635454483596LL: return 18;
    case -1567733074335LL: return 19; case -1557043932045LL: return 20; case -1526823271908LL: return 21;
    case -1524604197529LL: return 22; case -1516051957313LL: return 23; case -1494331276772LL: return 24;
    case -1489604836624LL: return 25; case -1452555012766LL: return 26; case -1440784622701LL: return 27;
    case -1427618549606LL: return 28; case -1424576589677LL: return 29; case -1411017625913LL: return 30;
    case -1381495885277LL: return 31; case -1331043820599LL: return 32; case -1321069061629LL: return 33;
    case -1295693033422LL: return 34; case -1280898418509LL: return 35; case -1263695988974LL: return 36;
    case -1247232526508LL: return 37; case -1200290182703LL: return 38; case -1178820378314LL: return 39;
    case -1178442944264LL: return 40; case -1161090766925LL: return 41; case -1160152611419LL: return 42;
    case -979120741022LL: return 43; case -966699583208LL: return 44; case -961683392174LL: return 45;
    case -955337828986LL: return 46; case -909482630098LL: return 47; case -899140663672LL: return 48;
    case -890351398636LL: return 49; case -880823280488LL: return 50; case -866558395764LL: return 51;
    case -850927208820LL: return 52; case -820810423912LL: return 53; case -783855092402LL: return 54;
    case -750442837499LL: return 55; case -702213956277LL: return 56; case -641733385571LL: return 57;
    case -613796583267LL: return 58; case -610058680283LL: return 59; case -559252334144LL: return 60;
    case -532405925580LL: return 61; case -530661298257LL: return 62; case -496225187517LL: return 63;
    case -482390910675LL: return 64; case -473306736671LL: return 65; case -419545411605LL: return 66;
    case -417509301549LL: return 67; case -416390259215LL: return 68; case -412417685300LL: return 69;
    case -375999912081LL: return 70; case -347190576165LL: return 71; case -319470493718LL: return 72;
    case -304563001981LL: return 73; case -304312505969LL: return 74; case -272291827924LL: return 75;
    case -200588114049LL: return 76; case -187292922488LL: return 77; case -186069837774LL: return 78;
    case -180990967392LL: return 79; case -175711994621LL: return 80; case -150882744788LL: return 81;
    case -136053440852LL: return 82; case -55346694158LL: return 83; case -30407241155LL: return 84;
    case -3670521136LL: return 85; case 50587567023LL: return 86; case 87498688900LL: return 87;
    case 111711067321LL: return 88; case 118669379196LL: return 89; case 129412918014LL: return 90;
    case 136759818588LL: return 91; case 144488666248LL: return 92; case 181402747077LL: return 93;
    case 183545537726LL: return 94; case 271585117470LL: return 95; case 292861623045LL: return 96;
    case 302459065496LL: return 97; case 315890645433LL: return 98; case 373945987777LL: return 99;
    case 413335638608LL: return 100; case 430551795500LL: return 101; case 458311053349LL: return 102;
    case 504564559027LL: return 103; case 548175516635LL: return 104; case 607475652656LL: return 105;
    case 609661205168LL: return 106; case 618527446832LL: return 107; case 650081104965LL: return 108;
    case 688022810723LL: return 109; case 690468513131LL: return 110; case 712864719919LL: return 111;
    case 733000990161LL: return 112; case 736495692823LL: return 113; case 760439396517LL: return 114;
    case 767370220603LL: return 115; case 789636369898LL: return 116; case 806689718738LL: return 117;
    case 815353717341LL: return 118; case 816038356983LL: return 119; case 827266065846LL: return 120;
    case 862092603643LL: return 121; case 883268976516LL: return 122; case 914726179671LL: return 123;
    case 947346250881LL: return 124; case 947470588253LL: return 125; case 960138884831LL: return 126;
    case 965684826819LL: return 127; case 968383068969LL: return 128; case 976652300106LL: return 129;
    case 980221003985LL: return 130; case 990156385461LL: return 131; case 1088648348957LL: return 132;
    case 1114201628744LL: return 133; case 1133067077335LL: return 134; case 1140448107476LL: return 135;
    case 1147898504660LL: return 136; case 1184319502386LL: return 137; case 1197400661318LL: return 138;
    case 1234140217468LL: return 139; case 1240585312106LL: return 140; case 1257425759405LL: return 141;
    case 1288474872246LL: return 142; case 1320284928409LL: return 143; case 1320966096571LL: return 144;
    case 1325514659818LL: return 145; case 1333221618675LL: return 146; case 1349244149288LL: return 147;
    case 1375831211075LL: return 148; case 1404363180839LL: return 149; case 1428558852213LL: return 150;
    case 1430594600318LL: return 151; case 1453581829610LL: return 152; case 1485519281947LL: return 153;
    case 1515456282203LL: return 154; case 1521243291970LL: return 155; case 1543488897135LL: return 156;
    case 1551205116033LL: return 157; case 1573952123645LL: return 158; case 1591184075947LL: return 159;
    case 1592849738099LL: return 160; case 1598754043289LL: return 161; case 1623006793832LL: return 162;
    case 1631275830377LL: return 163; case 1653252944807LL: return 164; case 1667753370763LL: return 165;
    case 1695599698132LL: return 166; case 1718365397931LL: return 167; case 1728221526197LL: return 168;
    case 1738921505513LL: return 169; case 1776349228080LL: return 170; case 1781612973496LL: return 171;
    case 1812334083392LL: return 172; case 1866406897707LL: return 173; case 1909825740728LL: return 174;
    case 1937242034518LL: return 175; case 1945571341946LL: return 176; case 1948225989769LL: return 177;
    case 1962212698263LL: return 178; case 1965360032451LL: return 179; case 1971653815318LL: return 180;
    default: return -1;
    }
}

/* 160 sparse unsigned cases (hash tier): the zero-extension path,
 * including a value above INT32_MAX. */
static unsigned sparse_uns160(unsigned x) {
    switch (x) {
    case 12327482u: return 1; case 13059675u: return 2; case 85472617u: return 3;
    case 155030746u: return 4; case 248988942u: return 5; case 291303929u: return 6;
    case 336548954u: return 7; case 344525783u: return 8; case 344716889u: return 9;
    case 349381288u: return 10; case 368708211u: return 11; case 452161708u: return 12;
    case 505104247u: return 13; case 507166750u: return 14; case 525169357u: return 15;
    case 526841197u: return 16; case 539279833u: return 17; case 553072596u: return 18;
    case 603356654u: return 19; case 609456109u: return 20; case 616408824u: return 21;
    case 623918916u: return 22; case 631561975u: return 23; case 666337756u: return 24;
    case 677413253u: return 25; case 694198655u: return 26; case 743082774u: return 27;
    case 823053184u: return 28; case 841411072u: return 29; case 850313664u: return 30;
    case 897613707u: return 31; case 901829114u: return 32; case 954740483u: return 33;
    case 984632075u: return 34; case 992908906u: return 35; case 1020842737u: return 36;
    case 1028565266u: return 37; case 1040576499u: return 38; case 1041395335u: return 39;
    case 1042752212u: return 40; case 1124247458u: return 41; case 1143496220u: return 42;
    case 1144033222u: return 43; case 1160041823u: return 44; case 1183688128u: return 45;
    case 1373781947u: return 46; case 1385049560u: return 47; case 1399888758u: return 48;
    case 1444093240u: return 49; case 1489339769u: return 50; case 1513091774u: return 51;
    case 1550642744u: return 52; case 1550677189u: return 53; case 1575280990u: return 54;
    case 1617290769u: return 55; case 1649041964u: return 56; case 1655947890u: return 57;
    case 1712808230u: return 58; case 1713706256u: return 59; case 1754906076u: return 60;
    case 1759591141u: return 61; case 1771874762u: return 62; case 1793598610u: return 63;
    case 1860286404u: return 64; case 1872180821u: return 65; case 1880470135u: return 66;
    case 1884251857u: return 67; case 1910461011u: return 68; case 1912628875u: return 69;
    case 1974851248u: return 70; case 1977415446u: return 71; case 1977532561u: return 72;
    case 1997643350u: return 73; case 2004824105u: return 74; case 2022940589u: return 75;
    case 2061105402u: return 76; case 2091846420u: return 77; case 2119993363u: return 78;
    case 2181485425u: return 79; case 2242395775u: return 80; case 2242861688u: return 81;
    case 2250982381u: return 82; case 2267811700u: return 83; case 2309826635u: return 84;
    case 2332097344u: return 85; case 2348045119u: return 86; case 2423599159u: return 87;
    case 2456762412u: return 88; case 2458145412u: return 89; case 2462445831u: return 90;
    case 2477052061u: return 91; case 2493436470u: return 92; case 2494911406u: return 93;
    case 2533231636u: return 94; case 2598874529u: return 95; case 2608121963u: return 96;
    case 2634893928u: return 97; case 2639986298u: return 98; case 2642989375u: return 99;
    case 2667519815u: return 100; case 2670178719u: return 101; case 2704037167u: return 102;
    case 2709950269u: return 103; case 2717562291u: return 104; case 2735716185u: return 105;
    case 2807931277u: return 106; case 2863100809u: return 107; case 2904597392u: return 108;
    case 2918108570u: return 109; case 2923775272u: return 110; case 2932670280u: return 111;
    case 2986265853u: return 112; case 3012228574u: return 113; case 3034412643u: return 114;
    case 3070520864u: return 115; case 3094888876u: return 116; case 3108062044u: return 117;
    case 3115476620u: return 118; case 3125141491u: return 119; case 3157352527u: return 120;
    case 3185319402u: return 121; case 3231715762u: return 122; case 3233216643u: return 123;
    case 3251145722u: return 124; case 3257791462u: return 125; case 3260966270u: return 126;
    case 3278081258u: return 127; case 3294691905u: return 128; case 3308803369u: return 129;
    case 3313528582u: return 130; case 3345598897u: return 131; case 3360244302u: return 132;
    case 3365978635u: return 133; case 3467074975u: return 134; case 3471127257u: return 135;
    case 3483795316u: return 136; case 3523634798u: return 137; case 3525509181u: return 138;
    case 3537909049u: return 139; case 3558709355u: return 140; case 3561687278u: return 141;
    case 3583963144u: return 142; case 3609189863u: return 143; case 3616668349u: return 144;
    case 3725839487u: return 145; case 3780214823u: return 146; case 3811884095u: return 147;
    case 3819987291u: return 148; case 3848855601u: return 149; case 3858469586u: return 150;
    case 3882453155u: return 151; case 3906793684u: return 152; case 3916156045u: return 153;
    case 3984577983u: return 154; case 3990804894u: return 155; case 4081461674u: return 156;
    case 4128696016u: return 157; case 4157826400u: return 158; case 4192419735u: return 159;
    case 4282762876u: return 160;
    default: return 0;
    }
}

/* 160 exact cases (well past every sparse-lowering threshold) plus one
 * case-range: a case-range must still disqualify both sparse
 * strategies regardless of how many exact cases would otherwise
 * qualify, falling back to the linear/range-aware chain -- and that
 * chain must still dispatch correctly at this size. */
static int mixedrange(int x) {
    switch (x) {
    case 12: return 1; case 17: return 2; case 28: return 3; case 30: return 4;
    case 33: return 5; case 40: return 6; case 45: return 7; case 46: return 8;
    case 47: return 9; case 80: return 10; case 81: return 11; case 83: return 12;
    case 86: return 13; case 89: return 14; case 94: return 15; case 96: return 16;
    case 105: return 17; case 106: return 18; case 111: return 19; case 122: return 20;
    case 128: return 21; case 144: return 22; case 150: return 23; case 153: return 24;
    case 155: return 25; case 157: return 26; case 161: return 27; case 167: return 28;
    case 168: return 29; case 173: return 30; case 191: return 31; case 192: return 32;
    case 194: return 33; case 197: return 34; case 205: return 35; case 206: return 36;
    case 209: return 37; case 213: return 38; case 224: return 39; case 225: return 40;
    case 230: return 41; case 237: return 42; case 238: return 43; case 247: return 44;
    case 264: return 45; case 277: return 46; case 281: return 47; case 287: return 48;
    case 290: return 49; case 292: return 50; case 295: return 51; case 307: return 52;
    case 308: return 53; case 313: return 54; case 319: return 55; case 326: return 56;
    case 329: return 57; case 335: return 58; case 346: return 59; case 347: return 60;
    case 350: return 61; case 351: return 62; case 354: return 63; case 362: return 64;
    case 373: return 65; case 376: return 66; case 380: return 67; case 397: return 68;
    case 399: return 69; case 409: return 70; case 417: return 71; case 424: return 72;
    case 425: return 73; case 429: return 74; case 438: return 75; case 448: return 76;
    case 449: return 77; case 452: return 78; case 458: return 79; case 462: return 80;
    case 466: return 81; case 471: return 82; case 472: return 83; case 473: return 84;
    case 488: return 85; case 489: return 86; case 495: return 87; case 504: return 88;
    case 509: return 89; case 515: return 90; case 522: return 91; case 527: return 92;
    case 529: return 93; case 532: return 94; case 535: return 95; case 549: return 96;
    case 554: return 97; case 555: return 98; case 564: return 99; case 572: return 100;
    case 573: return 101; case 582: return 102; case 591: return 103; case 607: return 104;
    case 610: return 105; case 623: return 106; case 633: return 107; case 637: return 108;
    case 643: return 109; case 646: return 110; case 650: return 111; case 659: return 112;
    case 660: return 113; case 662: return 114; case 668: return 115; case 673: return 116;
    case 675: return 117; case 680: return 118; case 690: return 119; case 691: return 120;
    case 699: return 121; case 703: return 122; case 707: return 123; case 713: return 124;
    case 716: return 125; case 726: return 126; case 737: return 127; case 743: return 128;
    case 744: return 129; case 745: return 130; case 752: return 131; case 762: return 132;
    case 763: return 133; case 765: return 134; case 767: return 135; case 773: return 136;
    case 776: return 137; case 782: return 138; case 785: return 139; case 801: return 140;
    case 803: return 141; case 806: return 142; case 810: return 143; case 814: return 144;
    case 822: return 145; case 823: return 146; case 824: return 147; case 836: return 148;
    case 852: return 149; case 855: return 150; case 860: return 151; case 864: return 152;
    case 867: return 153; case 872: return 154; case 874: return 155; case 876: return 156;
    case 877: return 157; case 880: return 158; case 884: return 159; case 891: return 160;
    case 100000 ... 100200: return 9999;
    default: return -1;
    }
}

static int hash200_vals[200] = {
        -495474, -479636, -457547, -452175, -438674, -431371, -425711, -421701, -414606, -397250,
        -394502, -382524, -369785, -361578, -357638, -357285, -354284, -351735, -344422, -342023,
        -321936, -317823, -315327, -309468, -295066, -294850, -290074, -289304, -289082, -287043,
        -285576, -283593, -283039, -277062, -275545, -271278, -270691, -265920, -263841, -260354,
        -259769, -258196, -252867, -252452, -249092, -242925, -237828, -228547, -225344, -223569,
        -221896, -208051, -201953, -183987, -182805, -181494, -175436, -168174, -165270, -164510,
        -158707, -156993, -153720, -153104, -152788, -152069, -148202, -146358, -137931, -133581,
        -132700, -128624, -128164, -116525, -113308, -104732, -93917, -92525, -90735, -73256,
        -69991, -68081, -65487, -64941, -63927, -63195, -58498, -52461, -42803, -40579,
        -16434, -13184, -12454, -11488, -8708, -7075, 5461, 17306, 19275, 22506,
        30202, 40297, 44578, 49137, 52928, 52944, 57431, 59080, 70249, 70592,
        91962, 95264, 97864, 106446, 106977, 109244, 124413, 130932, 136169, 143496,
        145557, 153937, 158608, 159209, 167049, 184217, 184435, 193742, 196115, 210956,
        217303, 219924, 222702, 228804, 229378, 236672, 238029, 238260, 239410, 242945,
        243476, 247376, 251424, 255459, 258660, 260549, 263036, 263110, 263640, 266835,
        266886, 270957, 277412, 279620, 286336, 286565, 289148, 293999, 295476, 298928,
        299865, 305119, 308126, 309925, 311989, 318276, 322416, 332211, 338627, 342016,
        343454, 352053, 371895, 377148, 383665, 384533, 384954, 391470, 392696, 403017,
        403070, 407450, 409223, 409598, 409954, 414260, 422257, 430399, 434294, 434711,
        443581, 444683, 449005, 469432, 469587, 477308, 481605, 485715, 486846, 496785,
};

static int bsearch300_vals[300] = {
        1000459, 1003895, 1024522, 1089345, 1124918, 1212120, 1219952, 1242596, 1253145, 1260791,
        1312775, 1315341, 1419026, 1451412, 1456168, 1467802, 1515748, 1566331, 1615454, 1728032,
        1737135, 1781266, 1799201, 1861477, 1870724, 1874909, 1901692, 1905629, 1906427, 1934342,
        1986787, 1994474, 2000302, 2000341, 2019919, 2030661, 2043735, 2051218, 2052547, 2117581,
        2122199, 2125890, 2129134, 2153191, 2322494, 2336985, 2342298, 2345329, 2353874, 2359253,
        2384706, 2449547, 2457595, 2467619, 2505133, 2508532, 2515470, 2528804, 2541984, 2561606,
        2594637, 2626390, 2652016, 2667806, 2743617, 2757190, 2757390, 2763062, 2763629, 2781388,
        2796687, 2822684, 2847629, 2870156, 2888259, 2903400, 2914137, 2925591, 2937018, 2937191,
        2981826, 2996812, 3074544, 3075296, 3115719, 3117472, 3144787, 3205024, 3230926, 3287631,
        3293632, 3318746, 3356864, 3359188, 3362755, 3364566, 3389640, 3401747, 3448076, 3488742,
        3579044, 3588754, 3614302, 3631190, 3640226, 3653473, 3664191, 3669480, 3737554, 3785372,
        3813095, 3836469, 3852022, 3992048, 3993477, 4003577, 4044209, 4048597, 4060036, 4169573,
        4192550, 4266670, 4337826, 4339793, 4357150, 4392758, 4413752, 4469171, 4509979, 4516675,
        4533067, 4571093, 4615447, 4618693, 4620236, 4639443, 4640967, 4654297, 4683604, 4738636,
        4764244, 4774187, 4807485, 4819571, 4866942, 4888572, 4897489, 4911353, 4920897, 4928824,
        4967626, 5024609, 5094058, 5126675, 5152453, 5164199, 5205696, 5229034, 5238086, 5238527,
        5252069, 5276597, 5314379, 5343084, 5345400, 5349445, 5379489, 5490987, 5511749, 5537977,
        5558434, 5565795, 5590358, 5630617, 5639264, 5651910, 5688086, 5697898, 5711681, 5757656,
        5778469, 5805393, 5806898, 5807920, 5858365, 5897329, 5898410, 5918494, 5961424, 5984260,
        5986046, 5994452, 6013091, 6017923, 6069470, 6070667, 6095798, 6104585, 6144705, 6158543,
        6182373, 6199795, 6242015, 6297496, 6313570, 6320897, 6326646, 6329692, 6340776, 6347376,
        6406886, 6408168, 6408214, 6411385, 6434850, 6462215, 6472679, 6524537, 6533432, 6587662,
        6649273, 6659823, 6665652, 6748708, 6759870, 6772789, 6857900, 6857983, 6897458, 6914849,
        6921090, 7005824, 7139122, 7145477, 7149474, 7159083, 7198267, 7309162, 7313499, 7382822,
        7407207, 7410512, 7418699, 7461448, 7482944, 7539541, 7614582, 7617086, 7672588, 7701893,
        7754384, 7755756, 7759707, 7759754, 7772964, 7796924, 7806403, 7815591, 7837572, 7901168,
        7912308, 7933327, 7939222, 7958728, 7959629, 7983877, 8001311, 8099777, 8191962, 8225255,
        8245399, 8270377, 8293904, 8368613, 8401987, 8405565, 8464516, 8475023, 8494231, 8540608,
        8549887, 8605711, 8643096, 8646950, 8687500, 8708005, 8748336, 8749456, 8758981, 8768833,
        8869653, 8880668, 8883380, 8901040, 8912752, 8915467, 8921736, 8928883, 8939398, 8946086,
};

static long long long180_vals[180] = {
        -1942605073660LL, -1940797503532LL, -1934764433686LL, -1934074036522LL, -1932130358930LL, -1922210203227LL, -1891540020681LL, -1859784502982LL, -1826787800859LL, -1815987644732LL,
        -1783434500346LL, -1778676992367LL, -1767423585885LL, -1736847965099LL, -1715853031791LL, -1698209076654LL, -1693505811596LL, -1635454483596LL, -1567733074335LL, -1557043932045LL,
        -1526823271908LL, -1524604197529LL, -1516051957313LL, -1494331276772LL, -1489604836624LL, -1452555012766LL, -1440784622701LL, -1427618549606LL, -1424576589677LL, -1411017625913LL,
        -1381495885277LL, -1331043820599LL, -1321069061629LL, -1295693033422LL, -1280898418509LL, -1263695988974LL, -1247232526508LL, -1200290182703LL, -1178820378314LL, -1178442944264LL,
        -1161090766925LL, -1160152611419LL, -979120741022LL, -966699583208LL, -961683392174LL, -955337828986LL, -909482630098LL, -899140663672LL, -890351398636LL, -880823280488LL,
        -866558395764LL, -850927208820LL, -820810423912LL, -783855092402LL, -750442837499LL, -702213956277LL, -641733385571LL, -613796583267LL, -610058680283LL, -559252334144LL,
        -532405925580LL, -530661298257LL, -496225187517LL, -482390910675LL, -473306736671LL, -419545411605LL, -417509301549LL, -416390259215LL, -412417685300LL, -375999912081LL,
        -347190576165LL, -319470493718LL, -304563001981LL, -304312505969LL, -272291827924LL, -200588114049LL, -187292922488LL, -186069837774LL, -180990967392LL, -175711994621LL,
        -150882744788LL, -136053440852LL, -55346694158LL, -30407241155LL, -3670521136LL, 50587567023LL, 87498688900LL, 111711067321LL, 118669379196LL, 129412918014LL,
        136759818588LL, 144488666248LL, 181402747077LL, 183545537726LL, 271585117470LL, 292861623045LL, 302459065496LL, 315890645433LL, 373945987777LL, 413335638608LL,
        430551795500LL, 458311053349LL, 504564559027LL, 548175516635LL, 607475652656LL, 609661205168LL, 618527446832LL, 650081104965LL, 688022810723LL, 690468513131LL,
        712864719919LL, 733000990161LL, 736495692823LL, 760439396517LL, 767370220603LL, 789636369898LL, 806689718738LL, 815353717341LL, 816038356983LL, 827266065846LL,
        862092603643LL, 883268976516LL, 914726179671LL, 947346250881LL, 947470588253LL, 960138884831LL, 965684826819LL, 968383068969LL, 976652300106LL, 980221003985LL,
        990156385461LL, 1088648348957LL, 1114201628744LL, 1133067077335LL, 1140448107476LL, 1147898504660LL, 1184319502386LL, 1197400661318LL, 1234140217468LL, 1240585312106LL,
        1257425759405LL, 1288474872246LL, 1320284928409LL, 1320966096571LL, 1325514659818LL, 1333221618675LL, 1349244149288LL, 1375831211075LL, 1404363180839LL, 1428558852213LL,
        1430594600318LL, 1453581829610LL, 1485519281947LL, 1515456282203LL, 1521243291970LL, 1543488897135LL, 1551205116033LL, 1573952123645LL, 1591184075947LL, 1592849738099LL,
        1598754043289LL, 1623006793832LL, 1631275830377LL, 1653252944807LL, 1667753370763LL, 1695599698132LL, 1718365397931LL, 1728221526197LL, 1738921505513LL, 1776349228080LL,
        1781612973496LL, 1812334083392LL, 1866406897707LL, 1909825740728LL, 1937242034518LL, 1945571341946LL, 1948225989769LL, 1962212698263LL, 1965360032451LL, 1971653815318LL,
};

static unsigned uns160_vals[160] = {
        12327482u, 13059675u, 85472617u, 155030746u, 248988942u, 291303929u, 336548954u, 344525783u, 344716889u, 349381288u,
        368708211u, 452161708u, 505104247u, 507166750u, 525169357u, 526841197u, 539279833u, 553072596u, 603356654u, 609456109u,
        616408824u, 623918916u, 631561975u, 666337756u, 677413253u, 694198655u, 743082774u, 823053184u, 841411072u, 850313664u,
        897613707u, 901829114u, 954740483u, 984632075u, 992908906u, 1020842737u, 1028565266u, 1040576499u, 1041395335u, 1042752212u,
        1124247458u, 1143496220u, 1144033222u, 1160041823u, 1183688128u, 1373781947u, 1385049560u, 1399888758u, 1444093240u, 1489339769u,
        1513091774u, 1550642744u, 1550677189u, 1575280990u, 1617290769u, 1649041964u, 1655947890u, 1712808230u, 1713706256u, 1754906076u,
        1759591141u, 1771874762u, 1793598610u, 1860286404u, 1872180821u, 1880470135u, 1884251857u, 1910461011u, 1912628875u, 1974851248u,
        1977415446u, 1977532561u, 1997643350u, 2004824105u, 2022940589u, 2061105402u, 2091846420u, 2119993363u, 2181485425u, 2242395775u,
        2242861688u, 2250982381u, 2267811700u, 2309826635u, 2332097344u, 2348045119u, 2423599159u, 2456762412u, 2458145412u, 2462445831u,
        2477052061u, 2493436470u, 2494911406u, 2533231636u, 2598874529u, 2608121963u, 2634893928u, 2639986298u, 2642989375u, 2667519815u,
        2670178719u, 2704037167u, 2709950269u, 2717562291u, 2735716185u, 2807931277u, 2863100809u, 2904597392u, 2918108570u, 2923775272u,
        2932670280u, 2986265853u, 3012228574u, 3034412643u, 3070520864u, 3094888876u, 3108062044u, 3115476620u, 3125141491u, 3157352527u,
        3185319402u, 3231715762u, 3233216643u, 3251145722u, 3257791462u, 3260966270u, 3278081258u, 3294691905u, 3308803369u, 3313528582u,
        3345598897u, 3360244302u, 3365978635u, 3467074975u, 3471127257u, 3483795316u, 3523634798u, 3525509181u, 3537909049u, 3558709355u,
        3561687278u, 3583963144u, 3609189863u, 3616668349u, 3725839487u, 3780214823u, 3811884095u, 3819987291u, 3848855601u, 3858469586u,
        3882453155u, 3906793684u, 3916156045u, 3984577983u, 3990804894u, 4081461674u, 4128696016u, 4157826400u, 4192419735u, 4282762876u,
};

static int mixed_vals[160] = {
        12, 17, 28, 30, 33, 40, 45, 46, 47, 80,
        81, 83, 86, 89, 94, 96, 105, 106, 111, 122,
        128, 144, 150, 153, 155, 157, 161, 167, 168, 173,
        191, 192, 194, 197, 205, 206, 209, 213, 224, 225,
        230, 237, 238, 247, 264, 277, 281, 287, 290, 292,
        295, 307, 308, 313, 319, 326, 329, 335, 346, 347,
        350, 351, 354, 362, 373, 376, 380, 397, 399, 409,
        417, 424, 425, 429, 438, 448, 449, 452, 458, 462,
        466, 471, 472, 473, 488, 489, 495, 504, 509, 515,
        522, 527, 529, 532, 535, 549, 554, 555, 564, 572,
        573, 582, 591, 607, 610, 623, 633, 637, 643, 646,
        650, 659, 660, 662, 668, 673, 675, 680, 690, 691,
        699, 703, 707, 713, 716, 726, 737, 743, 744, 745,
        752, 762, 763, 765, 767, 773, 776, 782, 785, 801,
        803, 806, 810, 814, 822, 823, 824, 836, 852, 855,
        860, 864, 867, 872, 874, 876, 877, 880, 884, 891,
};

int main(void) {
    int i;

    for (i = 0; i < 200; i++)
        if (sparse_hash200(hash200_vals[i]) != i + 1) abort();
    if (sparse_hash200(999999) != -1) abort();
    if (sparse_hash200(-999999) != -1) abort();
    if (sparse_hash200(0) != -1) abort();

    for (i = 0; i < 300; i++)
        if (sparse_bsearch300((int)bsearch300_vals[i]) != i + 1) abort();
    if (sparse_bsearch300(999999) != -1) abort();
    if (sparse_bsearch300(50000000) != -1) abort();
    if (sparse_bsearch300(0) != -1) abort();

    for (i = 0; i < 180; i++)
        if (sparse_long180((long long)long180_vals[i]) != i + 1) abort();
    if (sparse_long180(0LL) != -1) abort();
    if (sparse_long180(9999999999999LL) != -1) abort();

    for (i = 0; i < 160; i++)
        if (sparse_uns160(uns160_vals[i]) != (unsigned)(i + 1)) abort();
    if (sparse_uns160(0u) != 0) abort();
    if (sparse_uns160(4294967295u) != 0) abort();

    for (i = 0; i < 160; i++)
        if (mixedrange(mixed_vals[i]) != i + 1) abort();
    if (mixedrange(100000) != 9999) abort();
    if (mixedrange(100100) != 9999) abort();
    if (mixedrange(100200) != 9999) abort();
    if (mixedrange(999999) != -1) abort();
    if (mixedrange(100201) != -1) abort();
    if (mixedrange(99999) != -1) abort();

    return 0;
}
