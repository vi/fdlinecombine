#!/bin/bash

# Separator and chopped data test

set -e

cat > sep-chop.sample <<\EOF
 589828 
 65536 asT
 65536 cvb
 65536 ghj
 1 nm
 65535 nmzx
 1 qwerty
 1 uioppoi
 65535 uioppoiqwerty
 65536 YYUdf
 1 zx
EOF
for((i=0; i<5; ++i)) {

    SEPARATOR=<(printf '00') $1  6 5 7 \
        5< <( sleep 0; perl -e 'print "qwerty00uioppoi"x65536' ) \
        6< <( sleep 0; perl -e 'print "0asT000YYUdf0ghj0"x65536' ) \
        7< <( sleep 0; perl -e 'print "zx00cvb00000nm"x65536' ) \
        > sep-chop;

    cat sep-chop | tr '0' '\n' | sort | uniq -c | perl -pe 's/^\s+/ /' | diff -u sep-chop.sample -
}

rm sep-chop.sample sep-chop
