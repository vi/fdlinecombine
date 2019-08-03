#!/bin/bash

# Long line test
#
#    (slow, requires minimum 120M of memory + 120M of disk space)

set -e

perl -e 'print 35*1024*1024, " 1(", 50*1024*1024, ") ",35*1024*1024,"\n"' > long-line.sample
for((i=1; i<=2; ++i)) {
    echo "    iteration $i of 2";

    (ulimit -v 100000; $1  6 5 7) \
        5< <( sleep 0; perl -e 'print "0\n" foreach 1..(35*1024*1024)' ) \
        6< <( sleep 0; perl -e 'print "1"x(1024*1024) foreach 1..50'; echo ) \
        7< <( sleep 0; perl -e 'print "2\n" foreach 1..(35*1024*1024)' ) \
        > long-line;

    LANG=C uniq -c long-line | perl -ne '/^\s*(\d+)\s+0$/ and $z+=$1 and next; /^\s*(\d+)\s+2$/ and $t+=$1 and next; /\s*(\d+)\s+(1+)/ and $o+=$1 and ($on=length $2) and next; print "fail: $_"; END { print "$z $o($on) $t\n" }' | cmp long-line.sample -
}

rm long-line.sample long-line
