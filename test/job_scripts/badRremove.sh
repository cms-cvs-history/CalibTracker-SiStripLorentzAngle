#!/bin/bash

LIST_R=list_CRAFT_V3P.txt
LIST_BR=BadRuns_V3P.txt
OUTPUT=list_CRAFT_V3P_woBr.txt

for i in `gawk '{print $1}' $LIST_R`; do
BAD=0
grep -Fq "$i" $LIST_BR && BAD=1
if [ $BAD -eq 0 ] ; then
echo $i >> $OUTPUT
fi
done;
