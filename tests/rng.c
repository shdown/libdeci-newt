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

typedef uint32_t Rng;
typedef uint32_t RngResult;
#define RNG_RESULT_NDIGITS 9
#define RNG_RESULT_FMT PRIu32

static Rng rng_new(void)
{
    Rng x;
    FILE *f = fopen("/dev/urandom", "r");
    if (!f) {
        fprintf(stderr, "Cannot open /dev/urandom.\n");
        abort();
    }
    if (fread(&x, sizeof(x), 1, f) != 1) {
        fprintf(stderr, "Cannot read from /dev/urandom.\n");
        abort();
    }
    fclose(f);
    return x;
}

static RngResult rng_get(Rng *x)
{
    do {
        *x = (*x * 1103515245 + 12345) & 2147483647;
    } while (*x >= 2000000000);
    return *x % 1000000000;
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
}
