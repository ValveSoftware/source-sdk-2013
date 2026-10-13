#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Usage: $0 <filename> <name>"
    exit 1
fi

sFileName="$1"
sObjName="$2"

exec 3< "$sFileName"

printf "static unsigned char %s[] = {\n    " "$sObjName"

i=0
while :; do
    Byte=$(dd bs=1 count=1 <&3 2>/dev/null | od -An -t u1)
    [ -z "$Byte" ] && break

    Byte=$(printf "%s" "$Byte" | tr -d '[:space:]')
    printf "0x%02x," "$Byte"

    i=$(( i + 1 ))
    if [ $(( i % 20 )) -eq 0 ]; then
        printf "\n    "
    fi
done

printf "0x00\n};\n"
