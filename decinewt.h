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

#pragma once

#include <stddef.h>
#include "deci.h"

#define DECINEWT_MIN 4

// Must multiply (wa ... wa+nwa) by (wb ... wb+nwb), writing the result into (out ... out+nwa+nwb).
//
// <IMPORTANT NOTE>
//   The caller may call this callback with 'out' equal to either 'wa' or 'wb'. The spans
//     * (wa ... wa+nwa);
//     * (wb ... wb+nwb);
//     * (out ... out+nwa+nwb)
//   otherwise have no intersection. In particular, this means that 'wa' != 'wb'.
// </IMPORTANT NOTE>
//
// Must return -1 on error (e.g. on out-of-memory condition), 0 on success.
typedef int (*decinewt_MUL_CALLBACK)(
        void *userdata,
        deci_UWORD *wa, size_t nwa,
        deci_UWORD *wb, size_t nwb,
        deci_UWORD *out);

// Assumes nwd >= DECINEWT_MIN.
// Returns ((size_t) -1) on overflow.
// Otherwise guarantees that returned value >= prec.
size_t decinewt_inv_nscratch(
        size_t nwd, size_t prec);

// Assumes:
//   * nwd >= DECINEWT_MIN;
//   * (wd ... wd+nwd) is normalized;
//   * (wd ... wd+nwd) does not represent a power of DECI_BASE;
//   * 'scratch' has capacity of 'decinewt_inv_nscratch(nwd, prec)'.
//
// The result is written into (scratch ... scratch+prec).
//
// Returns -1 if 'mul_callback' returned -1; otherwise, returns 0.
int decinewt_inv(
        deci_UWORD *wd, size_t nwd,
        size_t prec,
        deci_UWORD *scratch,
        void *userdata,
        decinewt_MUL_CALLBACK mul_callback);

// Assumes nwx >= nwy >= DECINEWT_MIN.
// Returns ((size_t) -1) on overflow.
// Otherwise guarantees that returned value >= (2 * nwx - nwy + 2).
size_t decinewt_div_nscratch(
        size_t nwx, size_t nwy);

// Divides (wx ... wx+nwx) by (wy ... wy+nwy).
//
// Assumes:
//   * nwx >= nwy >= DECINEWT_MIN;
//   * (wy ... wy+nwy) is normalized;
//   * 'scratch' has capacity of 'decinewt_div_nscratch(nwx, nwy)'.
//
// The quotient 'q' is written into (scratch+nwx+1 ... scratch+2*nwx-nwy+2).
//
// The value of ('q' times (wy ... wy+nwy)) is written into (scratch ... scratch+nwx+1).
// Subtract it from (wx ... wx+nwx) to obtain the remainder.
//
// Note that the above implies (scratch ... scratch+nwx+1) <= (wx ... wx+nwx); and, in particular,
// scratch[nwx] == 0.
//
// Returns -1 if 'mul_callback' returned -1; otherwise, returns 0.
int decinewt_div(
        deci_UWORD *wx, size_t nwx,
        deci_UWORD *wy, size_t nwy,
        deci_UWORD *scratch,
        void *userdata,
        decinewt_MUL_CALLBACK mul_callback);
