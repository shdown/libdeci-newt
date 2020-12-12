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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdint.h>

static void die_oom(void)
{
    fprintf(stderr, "Out of memory.\n");
    abort();
}

static void *xmalloc(size_t n, size_t m)
{
    if (m && n > SIZE_MAX / m)
        die_oom();
    size_t sz = n * m;
    void *r = malloc(sz);
    if (!r && sz)
        die_oom();
    return r;
}

static inline size_t div_ceil_zu(size_t a, size_t b)
{
    return a / b + !!(a % b);
}

static char *read_number_str(void)
{
    char *buf = NULL;
    size_t nbuf = 0;
    ssize_t r = getline(&buf, &nbuf, stdin);
    if (r < 0) {
        if (ferror(stdin)) {
            perror("getline");
        } else {
            fprintf(stderr, "Got EOF.\n");
        }
        abort();
    }

    if (r && buf[r - 1] == '\n') {
        --r;
        buf[r] = '\0';
    }

    if (!r) {
        fprintf(stderr, "Got empty line.\n");
        abort();
    }
    for (size_t i = 0; i < (size_t) r; ++i) {
        unsigned char c = buf[i];
        if (c < '0' || c > '9') {
            fprintf(stderr, "Got non-numeric data.\n");
            abort();
        }
    }

    return buf;
}

static deci_UWORD parse_word(const char *s, size_t ns)
{
    deci_UWORD r = 0;
    for (size_t i = 0; i < ns; ++i) {
        r *= 10;
        r += s[i] - '0';
    }
    return r;
}

static void parse_str(deci_UWORD *out, const char *s, size_t ns)
{
    size_t i = ns;
    while (i >= DECI_BASE_LOG) {
        i -= DECI_BASE_LOG;
        *out++ = parse_word(s + i, DECI_BASE_LOG);
    }
    if (i) {
        *out++ = parse_word(s, i);
    }
}

#if DECI_WORD_BITS == 32
#   define DECI_UWORD_FMT PRIu32
#elif DECI_WORD_BITS == 64
#   define DECI_UWORD_FMT PRIu64
#else
#   error "Unsupported value of DECI_WORD_BITS."
#endif

static void pretty_print(const deci_UWORD *w, size_t nw)
{
    // normalize
    while (nw && w[nw - 1] == 0)
        --nw;

    if (!nw) {
        printf("0\n");
        return;
    }

    size_t i = nw - 1;
    printf("%" DECI_UWORD_FMT, w[i]);
    while (i) {
        --i;
        printf("%0*" DECI_UWORD_FMT, DECI_BASE_LOG, w[i]);
    }
    printf("\n");
}

int mul_cb(void *userdata, deci_UWORD *wa, size_t nwa, deci_UWORD *wb, size_t nwb, deci_UWORD *wr)
{
    (void) userdata;

    size_t nwr = nwa + nwb;
    if (wr == wa || wr == wb) {
        deci_UWORD *ptr = xmalloc(nwr, sizeof(deci_UWORD));
        deci_zero_out_n(ptr, nwr);
        deci_mul(wa, wa + nwa, wb, wb + nwb, ptr);
        deci_memcpy(wr, ptr, nwr);
        free(ptr);

    } else {
        deci_zero_out_n(wr, nwr);
        deci_mul(wa, wa + nwa, wb, wb + nwb, wr);
    }
    return 0;
}

int main()
{
    char *a = read_number_str();
    size_t na = strlen(a);

    char *b = read_number_str();
    size_t nb = strlen(b);

    size_t nwa = div_ceil_zu(na, DECI_BASE_LOG);
    deci_UWORD *wa = xmalloc(nwa, sizeof(deci_UWORD));
    parse_str(wa, a, na);

    size_t nwb = div_ceil_zu(nb, DECI_BASE_LOG);
    deci_UWORD *wb = xmalloc(nwb, sizeof(deci_UWORD));
    parse_str(wb, b, nb);

    if (nwa < nwb) {
        fprintf(stderr, "length(A) < length(B).\n");
        abort();
    }
    if (wb[nwb - 1] == 0) {
        fprintf(stderr, "B is not normalized (leading zeros?).\n");
        abort();
    }
    if (nwb < DECINEWT_MIN) {
        fprintf(stderr, "length(B) < DECINEWT_MIN = %zu.\n", (size_t) DECINEWT_MIN);
        abort();
    }

    size_t ns = decinewt_div_nscratch(nwa, nwb);
    deci_UWORD *s = xmalloc(ns, sizeof(deci_UWORD));

    int rc = decinewt_div(wa, nwa, wb, nwb, s, NULL, mul_cb);
    if (rc < 0) {
        fprintf(stderr, "decinewt_div() failed with code %d.\n", rc);
        abort();
    }
    pretty_print(s + nwa + 1, nwa - nwb + 1);
}
