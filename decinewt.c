/*
 * Copyright (C) 2020  libdeci-newt developers
 *
 * This file is part of libdeci-newt.
 *
 * libdeci-newt is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libdeci-newt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libdeci-newt.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "decinewt.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

static inline bool incr(deci_UWORD *a, size_t n)
{
    if (!n)
        return false;
    deci_UWORD one = 1;
    return !deci_add(a, a + n, &one, &one + 1);
}

static inline bool decr(deci_UWORD *a, size_t n)
{
    if (!n)
        return false;
    deci_UWORD one = 1;
    return !deci_sub(a, a + n, &one, &one + 1);
}

static inline bool greater(deci_UWORD *a, deci_UWORD *b, size_t n)
{
    return deci_compare_n(a, b, n, 0, 0, 1);
}

size_t decinewt_inv_nscratch(size_t nwd, size_t prec)
{
    if (prec > SIZE_MAX / 3)
        return -1;
    size_t r = nwd + prec * 3;
    if (r < nwd)
        return -1;
    return r;
}

// Fix a base B \in \mathbb{N}, B > 1.
//
// Fix h \in \mathbb{R} such that B^3 \le h < B^4.
//
// Define:
//   * r = B^6 / h;
//   * r_e = floor(B^6 / (floor(h) + 1)).
// Then
//   r - 2 < r_e \le r.
// Proof.
//   The fact that (r_e \le r) is trivial.
//
//   Define u = floor(h), r' = B^6 / (u + 1). Note that r_e = floor(r').
//   Then r - r' = B^6 / (u*(u+1)) \le B^6 / (B^6 + B^3) < 1.
//   Now we have r - r_e = (r - r') + frac(r') < 1 + 1 = 2.
static void calc_x0(deci_UWORD *wd_end, deci_UWORD *out)
{
    deci_UWORD a[7] = {0, 0, 0, 0, 0, 0, 1};
    deci_UWORD b[4] = {wd_end[-4], wd_end[-3], wd_end[-2], wd_end[-1]};

    if (!incr(b, 4)) {
        out[0] = 0;
        out[1] = 0;
        out[2] = 1;
        return;
    }

    size_t nr = deci_div(a, a + 7, b, b + 4);
    nr = deci_normalize_n(a, nr);
    assert(nr <= 3);
    for (size_t i = 0; i < nr; ++i)
        out[i] = a[i];
    for (size_t i = nr; i < 3; ++i)
        out[i] = 0;
}

int decinewt_inv(
        deci_UWORD *wd, size_t nwd,
        size_t prec,
        deci_UWORD *s,
        void *userdata,
        decinewt_MUL_CALLBACK mul_callback)
{
    calc_x0(wd + nwd, s);

    // Notes:
    //
    //   1. "(A value) X has precision of N words" means that
    //     answer - 2 * DECI_BASE^(-N) < X <= answer.
    //
    //   2. "(A span) has scale of N" means that we interpret this span so that its N highest words
    //      represent the integer part, and the rest is fractional part. In other words, the value
    //      of such a span is V / DECI_BASE^(L-N), where V is its "normal", integer value, L is its
    //      length in words.

    // We interpret (wd ... wd+nwd) as having scale of 0. This means that 1/DECI_BASE < d < 1.

    size_t p = 3;
    while (p < prec) {
        // Invariants:
        //   1. The current root (x_n) is located at (s ... s+p) with scale of 1.
        //   2. x_n has precision of (p - 2) words.

        deci_UWORD *v = s + p;
        size_t nv = p + nwd;
        // Calculate v = d * x_n. The scale of v is 1.
        if (mul_callback(
            userdata,
            wd, nwd,
            s, p,
            v) < 0)
        {
            return -1;
        }

        // Modify v = 2 - v.
        deci_UWORD v_hi = v[nv - 1];
        assert(v_hi == 0 || v_hi == 1);
        bool borrow = deci_uncomplement(v, v + nv - 1);
        v[nv - 1] = 2 - v_hi - borrow;

        // Modify v *= x_n. The new scale of v is 2.
        if (mul_callback(
            userdata,
            v, nv,
            s, p,
            v) < 0)
        {
            return -1;
        }
        nv += p;

        // Adjust v.
        if (v[nv - 1] != 0) {
            // v >= DECI_BASE
            v[nv - 1] = 0;
            for (size_t i = nv - 2; i != ((size_t) -1); --i)
                v[i] = DECI_BASE - 1;
        }

        // Set x_{n+1} = TRUNCATE(v, next_p).
        size_t next_p = 2 * (p - 1);
        if (next_p > prec)
            next_p = prec;
        memmove(
            /*dst=*/s,
            /*src=*/v + nv - 1 - next_p,
            /*n=*/next_p * sizeof(deci_UWORD));
        p = next_p;
    }

    // Adjust the result if prec < 3.
    if (p > prec) {
        memmove(
            /*dst=*/s,
            /*src=*/s + p - prec,
            /*n=*/prec * sizeof(deci_UWORD));
    }

    return 0;
}

size_t decinewt_div_nscratch(size_t nwx, size_t nwy)
{
    // This calculation can not overflow because nwx >= nwy >= 4.
    size_t p = nwx - nwy + 2;

    size_t n1 = nwx + p;
    if (n1 < p)
        return -1;
    size_t n2 = decinewt_inv_nscratch(nwy, p);
    return n1 > n2 ? n1 : n2;
}

int decinewt_div(
        deci_UWORD *wx, size_t nwx,
        deci_UWORD *wy, size_t nwy,
        deci_UWORD *s,
        void *userdata,
        decinewt_MUL_CALLBACK mul_callback)
{
    if (wy[nwy - 1] == 1 && deci_is_zero_n(wy, nwy - 1)) {
        // Special case: (wy ... wy+nwy) is a power of DECI_BASE.
        size_t nq = nwx + 1 - nwy;
        size_t offset = nwx + 1;
        deci_UWORD *q = wx + nwx - nq;

        memset(s, '\0', offset * sizeof(deci_UWORD));

        memmove(
            /*dst=*/s + nwx - nq,
            /*src=*/q,
            /*n=*/nq * sizeof(deci_UWORD));

        memmove(
            /*dst=*/s + offset,
            /*src=*/q,
            /*n=*/nq * sizeof(deci_UWORD));

        return 0;
    }

    if (decinewt_inv(
        wy, nwy,
        /*prec=*/nwx - nwy + 2,
        s,
        userdata, mul_callback) < 0)
    {
        return -1;
    }

    if (mul_callback(
        userdata,
        wx, nwx,
        s, nwx - nwy + 2,
        s) < 0)
    {
        return -1;
    }

    // The resulting number in (s ... s + 2*nwx-nwy+2) has exactly (nwx+1) fractional words and
    // exactly (nwx+1-nwy) integer words.
    //
    // Its integer part either equals to the result or is smaller by one.

    size_t nq = nwx - nwy + 1;
    size_t offset = nwx + 1;

    if (!incr(s + offset, nq)) {
        decr(s + offset, nq);
    }

    if (mul_callback(
        userdata,
        wy, nwy,
        s + offset, nq,
        s) < 0)
    {
        return -1;
    }

    if (s[nwx] != 0 || greater(s, wx, nwx)) {
        deci_sub_raw(s, s + offset, wy, wy + nwy);
        decr(s + offset, nq);
    }

    return 0;
}
