#!/usr/bin/env bash

set -o pipefail

export PRE_DEFINED="not override"

mkdir -p output

tests=0
errors=0
for env in *.env; do
    name=$(basename "$env" .env)

    DOTENV_CONFIG_PATH="$env" node printDotenv.js > "output/$name-original.txt"
    ../build/debug/dotenv --file="$env" -- node -e 'require("./printEnv.js").printEnv();' > "output/$name-cpp.txt"

    if cmp "output/$name-original.txt" "output/$name-cpp.txt"; then
        echo "[  OK  ] $name"
    else
        echo "[ FAIL ] $name: Output is different!">&2
        echo >&2
        diff "output/$name-original.txt" "output/$name-cpp.txt"
        errors=$((errors+1))
    fi
    tests=$((tests+1))
done

echo
echo "$tests tests, $((tests-errors)) successful, $errors failed"

if [ "$errors" -ne 0 ]; then
    exit 1
fi
