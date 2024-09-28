#!/bin/bash

pushd "$(dirname "$0")" > /dev/null

if [ ! -f "devtools/bin/vpc" ]; then
    echo "Error: vpc tool not found in devtools/bin/"
    popd > /dev/null
    exit 1
fi

echo "Running build solution"
devtools/bin/vpc /hl2mp +everything /mksln everything

popd > /dev/null
