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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

typedef struct {
    FILE *f;
} Rng;
typedef uint32_t RngResult;
#define RNG_RESULT_NDIGITS 9
#define RNG_RESULT_FMT PRIu32

static Rng rng_new(void)
{
    FILE *f = fopen("/dev/urandom", "r");
    if (!f) {
        fprintf(stderr, "Cannot open /dev/urandom.\n");
        abort();
    }
    return (Rng) {f};
}

static RngResult rng_get(Rng *x)
{
    uint32_t y;
    do {
        if (fread(&y, sizeof(y), 1, x->f) != 1) {
            fprintf(stderr, "Read failure or truncated read from /dev/urandom.\n");
            abort();
        }
    } while (y >= 2000000000);
    return y % 1000000000;
}

static void rng_destroy(Rng *x)
{
    if (x->f)
        fclose(x->f);
}

static void print_inline(int ndigits, RngResult x)
{
    printf("%0.*" RNG_RESULT_FMT, ndigits, x);
}

static void print_endline(void)
{
    printf("\n");
}

static void gen_n(Rng *x, size_t n)
{
    if (!n)
        return;

    RngResult first;
    do {
        first = rng_get(x) % 10;
    } while (!first);
    print_inline(1, first);

    --n;
    for (; n >= RNG_RESULT_NDIGITS; n -= RNG_RESULT_NDIGITS) {
        print_inline(RNG_RESULT_NDIGITS, rng_get(x));
    }

    RngResult m = 1;
    for (size_t i = 0; i < n; ++i)
        m *= 10;
    print_inline(n, rng_get(x) % m);
    print_endline();
}

int main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "USAGE: rng NDIGITS\n");
        abort();
    }
    size_t n;
    if (sscanf(argv[1], "%zu", &n) != 1) {
        fprintf(stderr, "Cannot parse NDIGITS.\n");
        abort();
    }
    Rng x = rng_new();
    gen_n(&x, n);
    rng_destroy(&x);
}
