#!/bin/bash

output="np2_rhythm.h"

out()
{
    printf "${1}" >> $output
}

truncate -s 0 $output

for q in bd hh rim sd tom top; do
    out "static unsigned char np2_2608_${q}[] = \n"
    out "{\n"
    hexdump -ve '12/1 "0x%02x, " "\n"' 2608_${q}.wav >> $output
    out "0x00\n"
    out "};\n"
    out "\n"
done

sed -i "s/0x  /0x00/g" $output

