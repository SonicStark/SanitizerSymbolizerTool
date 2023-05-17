#!/bin/bash

set -euo pipefail

echo "All parameters of this script will be"
echo "passed to gdb in intact. ELF files without"
echo "executable permissions and incorrect parameter"
echo "settings may lead to incorrect calculation results,"
echo "in which situation the offset value is often 0."
echo ""
echo "What's more, modern version of GCC is usually configured"
echo "with --enable-default-pie, so it sets -pie and -fPIE by default."
echo "When you disable this and generate a normal executable"
echo "(ET_EXEC rather than ET_DYN) by linking with -no-pie, the offset"
echo "calculated by the following process should always be 0."
echo ""

gdb_command=$(echo -e "set pagination off\ninfo files\nstart\ninfo files")

output_text=$(echo "$gdb_command" | gdb -q --args  $@  2>/dev/null | grep "is .text" | grep -v " in ")

DeltaLst=()

while read -r range_line
do
    words_lst=($range_line)
    DeltaLst[${#DeltaLst[*]}]=${words_lst[0]}
done < <(printf '%s\n' "$output_text")

declare -i addr_0=${DeltaLst[0]}
declare -i addr_1=${DeltaLst[1]}
declare -i offset=${DeltaLst[1]}-${DeltaLst[0]}

echo ">>> `printf '0x%016x\n' $addr_0` is where .text starts in VMA indicates by the ELF file."
echo ">>> `printf '0x%016x\n' $addr_1` is where .text starts in VMA after the ELF file loaded."
echo ">>> `printf '0x%016x\n' $offset` is the offset value."