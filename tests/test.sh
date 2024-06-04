#!/usr/bin/bash
set -o nounset
set -o errtrace
function catch_error {
    local LEC=$? name i line file
    echo "Traceback (most recent call last):" >&2
    for ((i = ${#FUNCNAME[@]} - 1; i >= 0; --i)); do
        name="${FUNCNAME[$i]}"
        line="${BASH_LINENO[$i]}"
        file="${BASH_SOURCE[$i]}"
        echo "  File ${file@Q}, line ${line}, in ${name@Q}" >&2
    done
    echo "Error: [ExitCode: ${LEC}]" >&2
    exit "${LEC}"
}
trap catch_error ERR

dirname=$(dirname -- "$0") || exit
cd -- "$dirname" || exit

bin=$(mktemp) || exit
trap "rm -- ${bin@Q}" EXIT

gcc ./test.c -o "$bin" || exit
echo compilation completed!

for file in ./tests/*.sipa; do
    printf '\e[92mtest %q\e[39m\n' "$file"
    mapfile -d '' out < <(
        "$bin" < "$file" || lec=$?
        printf '%d\0' ${lec-0}
    )
    if [ "${out[-1]}" != 0 ]; then
        echo
        echo "test failed! (code: ${out[-1]})" >&2
        exit 3
    fi
    short=${out[0]}
    long=${out[1]}
    printf -- '-- short --\n%s\n-- long --\n%s' "$short" "$long"

    printf %s "$short" | "$bin" >/dev/null
    printf %s "$long" | "$bin" >/dev/null
done
