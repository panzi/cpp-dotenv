#!/usr/bin/env bash

set -eo pipefail

export PRE_DEFINED="not override"

node printDotenv.js > original.txt
../build/debug/dotenv --file=edge-cases.env -- node -e 'require("./printEnv.js").printEnv();' > cpp.txt

echo
if cmp original.txt cpp.txt; then
    echo 'Output matches!'
else
    echo 'Error: Output is different!'>&2
    echo >&2
    diff original.txt cpp.txt
fi
