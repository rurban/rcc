// rcc `_BitInt(N)` runtime helpers for N > 64 (multi-limb values that do
// not fit in one GP register).
//
// SPDX-License-Identifier: MIT (see below)
// Ported from slimcc's bitint.c (same file layout, functions renamed
// __slimcc_bitint_* -> __rcc_bitint_*). slimcc is a derivative of
// chibicc; this file keeps the original authors' copyright.
//
//   Copyright (c) 2019 Rui Ueyama
//   Copyright (c) 2023-2026 Hsiang-Ying Fu
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// All values are little-endian limb arrays (unsigned long long limbs, or unsigned
// where noted). `bits` is the _BitInt width; the value occupies
// ceil(bits/64) 64-bit limbs (the top limb's unused high bits are
// expected to be zero for unsigned / sign-extended for signed values, as
// the type's storage guarantees).

static int rcc_bitint_first_set(int bits, void *lp) {
    int idx = (bits - 1) / 64;
    unsigned long long *lh = lp;

    int clr_shft = 63 - (bits - 1) % 64;
    lh[idx] = (lh[idx] << clr_shft) >> clr_shft;

    for (; idx >= 0; idx--)
        if (lh[idx])
            for (int shft = 0; shft < 64; shft++)
                if (0 > (long long)(lh[idx] << shft))
                    return (64 - shft) + idx * 64;
    return 0;
}

static _Bool rcc_bitint_overflow(int bits, void *sp, int chk_bits, _Bool is_unsigned) {
    long long *val = sp;
    int idx = (chk_bits - 1) / 64;
    int shft = (chk_bits - 1) % 64;

    long long chk = val[idx++] >> shft;
    if (chk != 0)
        if (is_unsigned || chk != -1)
            return 1;

    int top = (bits - 1) / 64;
    while (idx <= top)
        if (val[idx++] != chk)
            return 1;

    return 0;
}

static _Bool rcc_bitint_to_bool(int bits, void *lp) {
    int idx = (bits - 1) / 64;
    unsigned long long *lh = lp;

    int clr_shft = 63 - (bits - 1) % 64;
    lh[idx] = (lh[idx] << clr_shft) >> clr_shft;

    while (idx >= 0)
        if (lh[idx--])
            return 1;
    return 0;
}

// 0 = equal, 1 = lp < rp, 2 = lp > rp.
static int rcc_bitint_cmp(int bits, int is_unsigned, void *lp, void *rp) {
    int idx = (bits - 1) / 64;
    unsigned long long *lh = lp, *rh = rp;

    int clr_shft = 63 - (bits - 1) % 64;
    unsigned long long signbit = is_unsigned ? 0 : 1ULL << 63;
    unsigned long long lv = (lh[idx] << clr_shft) + signbit;
    unsigned long long rv = (rh[idx] << clr_shft) + signbit;

    for (;;) {
        if (lv > rv)
            return 2;
        else if (lv < rv)
            return 1;

        if (--idx < 0)
            break;
        lv = lh[idx];
        rv = rh[idx];
    }
    return 0;
}

// Extend lp (stored in `bits` bits) in place to `bits2` bits, filling the
// new high limbs with the sign (signed) or zeros (unsigned).
static void rcc_bitint_sign_ext(int bits, int is_unsigned, void *lp, int bits2) {
    int idx = (bits - 1) / 64;
    unsigned long long *lh = lp;

    unsigned long long fill = lh[idx];
    int shft = bits % 64;
    if (shft) {
        fill <<= 64 - shft;
        if (is_unsigned)
            lh[idx] = (unsigned long long)fill >> (64 - shft);
        else
            lh[idx] = (long long)fill >> (64 - shft);
    }
    // Sign-fill word: all ones if the source's sign bit is set, else zero.
    // (Unsigned right shift + test, avoiding implementation-defined signed
    // right-shift of a negative value that cppcheck flags as UB.)
    fill = is_unsigned ? 0 : (fill >> 63 ? ~0ULL : 0);

    int top = (bits2 - 1) / 64;
    while (++idx <= top)
        lh[idx] = fill;
}

static void rcc_bitint_neg(int bits, void *lp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp;

    unsigned long long borrow = 0;
    for (int i = 0; i < cnt; i++) {
        unsigned long long tmp;
        borrow = (tmp = lh[i] + borrow) < borrow;
        borrow |= (lh[i] = 0 - tmp) > 0;
    }
}

static void rcc_bitint_bitnot(int bits, void *lp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp;

    for (int i = 0; i < cnt; i++)
        lh[i] = ~lh[i];
}

static void rcc_bitint_bitand(int bits, void *lp, void *rp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp, *rh = rp;

    for (int i = 0; i < cnt; i++)
        rh[i] &= lh[i];
}

static void rcc_bitint_bitor(int bits, void *lp, void *rp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp, *rh = rp;

    for (int i = 0; i < cnt; i++)
        rh[i] |= lh[i];
}

static void rcc_bitint_bitxor(int bits, void *lp, void *rp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp, *rh = rp;

    for (int i = 0; i < cnt; i++)
        rh[i] ^= lh[i];
}

static void rcc_bitint_shl(int bits, void *sp, void *dp, int amount) {
    if (amount < 0 || amount >= bits)
        return;

    unsigned long long *src = sp, *dst = dp;
    int idx = (bits - 1) / 64;
    int src_idx = idx - amount / 64;
    int shft = amount % 64;

    unsigned long long hi = src[src_idx--];
    for (; idx >= 0; idx--) {
        unsigned long long lo = src_idx >= 0 ? src[src_idx--] : 0;
        dst[idx] = !shft ? hi : (hi << shft) | (lo >> (64 - shft));
        hi = lo;
    }
}

static void rcc_bitint_shr(int bits, int flags, void *sp, void *dp) {
    int amount = flags >> 1;
    _Bool is_unsigned = flags & 1;
    if (amount < 0 || amount >= bits)
        return;

    unsigned long long *src = sp, *dst = dp;
    int src_idx = amount / 64;
    int src_top = (bits - 1) / 64;
    int top = (bits - 1 - amount) / 64;
    int shft = amount % 64;

    unsigned long long lo = src[src_idx++];
    for (int idx = 0; idx <= top; idx++) {
        unsigned long long hi = src_idx <= src_top ? src[src_idx++] : 0;
        dst[idx] = !shft ? lo : (hi << (64 - shft)) | (lo >> shft);
        lo = hi;
    }
    rcc_bitint_sign_ext(bits - amount, is_unsigned, dp, bits);
}

void *rcc_bitint_bitfield_load(int bits, void *sp, void *dp, int width,
                               int ofs, _Bool is_unsigned) {
    char *src = sp, *dst = dp;
    int sz = (bits + 7) / 8;

    for (int i = 0; i < sz; i++)
        dst[i] = src[i];

    rcc_bitint_shl(bits, dp, dp, bits - width - ofs);
    rcc_bitint_shr(bits, ((bits - width) << 1) | is_unsigned, dp, dp);
    return sp;
}

static void rcc_bitint_bitfield_save(int bits, void *sp, void *dp, int width,
                                     int ofs) {
    int cnt = (bits + 63) / 64;
    int full_bits = cnt * 64;

    unsigned long long mb[cnt], sb[cnt];
    for (int i = 0; i < cnt; i++)
        mb[i] = -1;

    rcc_bitint_shl(full_bits, mb, mb, full_bits - width);
    rcc_bitint_shl(full_bits, sp, sb, full_bits - width);

    rcc_bitint_shr(full_bits, ((full_bits - width - ofs) << 1) | 1, mb, mb);
    rcc_bitint_shr(full_bits, ((full_bits - width - ofs) << 1) | 1, sb, sb);

    rcc_bitint_bitnot(full_bits, mb);

    char *dst = dp, *src = (char *)sb, *msk = (char *)mb;
    int top = (width + ofs - 1) / 8;

    for (int j = ofs / 8; j <= top; j++)
        dst[j] = (dst[j] & msk[j]) | src[j];
}

// rp += lp (result in rp, matching the in-place calling convention).
static void rcc_bitint_add(int bits, void *lp, void *rp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp, *rh = rp;

    unsigned long long carry = 0;
    for (int i = 0; i < cnt; i++) {
        unsigned long long tmp;
        carry = (tmp = rh[i] + carry) < carry;
        carry |= (rh[i] = lh[i] + tmp) < tmp;
    }
}

// rp -= lp (result in rp).
static void rcc_bitint_sub(int bits, void *lp, void *rp) {
    int cnt = (bits + 63) / 64;
    unsigned long long *lh = lp, *rh = rp;

    unsigned long long borrow = 0;
    for (int i = 0; i < cnt; i++) {
        unsigned long long tmp;
        borrow = (tmp = rh[i] + borrow) < borrow;
        borrow |= (rh[i] = lh[i] - tmp) > lh[i];
    }
}

// rp *= lp (result in rp).
static void rcc_bitint_mul(int bits, void *lp, void *rp) {
    int cnt = (bits + 31) / 32;
    unsigned *lh = lp, *rh = rp;

    unsigned buf[cnt];

    unsigned long long accum = 0;
    for (int i = 0; i < cnt; i++) {
        unsigned long long ovf_cnt = 0;
        for (int j = 0; j <= i; j++) {
            unsigned long long prod = (unsigned long long)lh[j] * rh[i - j];
            ovf_cnt += (accum += prod) < prod;
        }
        buf[i] = (unsigned)accum;
        accum = ovf_cnt << 32 | accum >> 32;
    }
    for (int j = 0; j < cnt; j++)
        rh[j] = buf[j];
}

// rp = rp / lp (is_div) or rp % lp (else), unsigned or signed.
static void rcc_bitint_div(int bits, int flags, void *lp, void *rp) {
    int cnt = (bits + 63) / 64 * 2;
    _Bool is_unsigned = (flags & 1) != 0;
    _Bool is_div = (flags & 2) != 0;
    unsigned *lh = lp, *rh = rp;

    unsigned r_buf[cnt + 2];
    unsigned *q_buf = lh;

    for (int i = 0; i < cnt; i++) {
        r_buf[i] = lh[i];
        lh[i] = 0;
    }
    r_buf[cnt] = r_buf[cnt + 1] = 0;

    int msl = (bits - 1) / 32;
    int sign_shft = (bits - 1) % 32;

    _Bool l_neg = is_unsigned ? 0 : (r_buf[msl] >> sign_shft) & 1;
    _Bool r_neg = is_unsigned ? 0 : (rh[msl] >> sign_shft) & 1;

    if (l_neg)
        rcc_bitint_neg(bits, r_buf);
    if (r_neg)
        rcc_bitint_neg(bits, rh);

    int l_fsb = rcc_bitint_first_set(bits, r_buf);
    int r_fsb = rcc_bitint_first_set(bits, rh);

    int r_ofs = r_fsb % 32;
    int r_shft = r_ofs ? 32 - r_ofs : 0;
    int ln = (l_fsb + 31) / 32;
    int rn = (r_fsb + 31) / 32;

    if (r_shft) {
        rcc_bitint_shl(bits + 64, r_buf, r_buf, r_shft);
        rcc_bitint_shl(rn * 32, rh, rh, r_shft);
    }

    for (int qi = ln - rn; qi >= 0; qi--) {
        unsigned quo;
        if (r_buf[qi + rn] == rh[rn - 1]) {
            quo = -1;
        } else {
            unsigned long long top = (unsigned long long)r_buf[qi + rn] << 32 | r_buf[qi + rn - 1];
            unsigned long long q = top / rh[rn - 1];
            unsigned long long r = top % rh[rn - 1];

            if (rn > 1) {
                r = r << 32 | r_buf[qi + rn - 2];
                unsigned long long r_inc = (unsigned long long)rh[rn - 1] << 32;
                while (r < q * rh[rn - 2]) {
                    q -= 1;
                    if ((r += r_inc) < r_inc)
                        break;
                }
            }
            quo = q;
        }

        unsigned sub = 0;
        for (int i = 0; i < rn; i++) {
            unsigned long long prod = (unsigned long long)quo * rh[i];

            unsigned tmp = (unsigned)prod + sub;
            sub = (tmp < sub) + (r_buf[qi + i] < tmp) + (prod >> 32);
            r_buf[qi + i] -= tmp;
        }

        unsigned rem = r_buf[qi + rn] - sub;
        if (rem > r_buf[qi + rn]) {
            unsigned long long carry = 0;
            for (int i = 0; i < rn; i++) {
                unsigned long long tmp;
                carry = (tmp = rh[i] + carry) < carry;
                carry |= (r_buf[qi + i] = r_buf[qi + i] + tmp) < tmp;
            }
            rem += carry;
            quo -= 1;
        }
        r_buf[qi + rn] = rem;
        q_buf[qi] = quo;
    }

    unsigned *res;
    if (is_div) {
        if (l_neg + r_neg == 1)
            rcc_bitint_neg(bits, q_buf);
        res = q_buf;
    } else {
        if (r_shft)
            rcc_bitint_shr(bits + 64, (r_shft << 1) | 1, r_buf, r_buf);
        if (l_neg)
            rcc_bitint_neg(bits, r_buf);
        res = r_buf;
    }
    for (int j = 0; j < cnt; j++)
        rh[j] = res[j];
}
