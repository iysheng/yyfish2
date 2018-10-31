#!/bin/sh
PWD=`pwd`
echo $PWD
find  $PWD                                                                \
-path "$PWD/arch/*" ! -path "$PWD/arch/arm*" -prune -o               \
-path "$PWD/Documentation*" -prune -o                                 \
-path "$PWD/scripts*" -prune -o                                       \
 \( -name "*.[chsS]" -o -name "Makefile" \) -type f -print > $PWD/cscope.files

cscope -b -q
