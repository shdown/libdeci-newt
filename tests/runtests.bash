#!/usr/bin/env bash

N1=103
N2=61
INPUT_FILE=/tmp/input.txt

checker() {
    python3 -c 'x=int(input()); y=int(input()); print(x // y)'
}

xrand_decimal() {
    ./rng "$1"
}

for (( i = 0; i < 1000; ++i )); do
    { xrand_decimal "$N1"; xrand_decimal "$N2"; } > "$INPUT_FILE"
    out=$(./driver < "$INPUT_FILE") || exit $?
    ans=$(checker < "$INPUT_FILE") || exit $?
    if [[ $out != $ans ]]; then
        echo >&2 "FAIL: expected '$ans', found '$out'. See '$INPUT_FILE' for input data."
        exit 1
    fi
    echo >&2 "[OK] $i, answer is '$ans'"
done

echo >&2 "PASSED"
