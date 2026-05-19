#!/bin/sh

set -eu

if [ $# -ne 3 ]; then
    echo "usage: $0 <name> <infile> <outfile>" >&2
    exit 1
fi

name=$1
infile=$2
outfile=$3

if [ ! -f "$infile" ]; then
    echo "error: input file '$infile' does not exist" >&2
    exit 1
fi

len=$(wc -c < "$infile" | tr -d ' ')

escaped=$(sed \
    -e 's/\\/\\\\/g' \
    -e 's/"/\\"/g' \
    -e 's/^/"/' \
    -e 's/$/\\n"/' \
    "$infile")

{
    echo "/* Auto-generated from $infile on $(date). DO NOT EDIT! */"
    echo ""
    echo "#ifndef ${name}_H"
    echo "#define ${name}_H"
    echo ""
    echo "#define ${name}_DATA \\"
    printf '%s\n' "$escaped" | awk '
        { lines[NR] = $0 }
        END {
            for (i = 1; i <= NR; i++) {
                if (i < NR) printf "    %s \\\n", lines[i]
                else        printf "    %s\n",   lines[i]
            }
        }'
    echo ""
    echo "#define ${name}_LEN $len"
    echo ""
    echo "#endif // ${name}_H"
} > "$outfile"
