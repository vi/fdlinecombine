#!/bin/bash

set -e

perl -e 'print "0\n"x65536, "1\n"x65536, "2\n"x65536' > interleaved.sample
for((i=0; i<10; ++i)) {

    $1  6 5 7 \
        5< <( sleep 0; perl -e 'print "0\n"x65536' ) \
        6< <( sleep 0; perl -e 'print "1\n"x65536' ) \
        7< <( sleep 0; perl -e 'print "2\n"x65536' ) \
        > interleaved;

    sort interleaved | cmp interleaved.sample -
}

rm interleaved.sample interleaved
