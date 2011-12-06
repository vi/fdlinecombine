#!/bin/bash

set -e

echo "        (the test runs for 1.5 minutes on my computer)"

echo "Trivial test"
./fdlinecombine 2> /dev/null

echo Basic test
perl -e '
    print "0"x65536, "\n";
    print "1"x65536, "\n";
    print "2"x65536, "\n";
    print "3"x65536, "\n";
    print "4"x65536, "\n";
    print "5"x65536, "\n";
    print "6"x65536, "\n";
    print "7"x65536, "\n";
    print "8"x65536, "\n";
    print "9"x65536, "\n";
    print "10"x65536, "\n";
    print "11"x65536, "\n";
    print "12"x65536, "\n";
    ' > out.sample
for((i=0; i<20; ++i)) {

    ./fdlinecombine  6 5 7 8 9 10 11 12 13 14 15 16 17 \
        5<  <( sleep 0; perl -e 'print "0"x65536, "\n"' ) \
        6<  <( sleep 0; perl -e 'print "1"x65536, "\n"' ) \
        7<  <( sleep 0; perl -e 'print "2"x65536, "\n"' ) \
        8<  <( sleep 0; perl -e 'print "3"x65536, "\n"' ) \
        9<  <( sleep 0; perl -e 'print "4"x65536, "\n"' ) \
        10< <( sleep 0; perl -e 'print "5"x65536, "\n"' ) \
        11< <( sleep 0; perl -e 'print "6"x65536, "\n"' ) \
        12< <( sleep 0; perl -e 'print "7"x65536, "\n"' ) \
        13< <( sleep 0; perl -e 'print "8"x65536, "\n"' ) \
        14< <( sleep 0; perl -e 'print "9"x65536, "\n"' ) \
        15< <( sleep 0; perl -e 'print "10"x65536, "\n"' ) \
        16< <( sleep 0; perl -e 'print "11"x65536, "\n"' ) \
        17< <( sleep 0; perl -e 'print "12"x65536, "\n"' ) \
        > out;

    sort -n out | cmp out.sample -
}

echo Interleaved test
perl -e 'print "0\n"x65536, "1\n"x65536, "2\n"x65536' > out.sample
for((i=0; i<10; ++i)) {

    ./fdlinecombine  6 5 7 \
        5< <( sleep 0; perl -e 'print "0\n"x65536' ) \
        6< <( sleep 0; perl -e 'print "1\n"x65536' ) \
        7< <( sleep 0; perl -e 'print "2\n"x65536' ) \
        > out;

    sort out | cmp out.sample -
}

echo Separator and chopped data test
cat > out.sample <<\EOF
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

    SEPARATOR=10 ./fdlinecombine  6 5 7 \
        5< <( sleep 0; perl -e 'print "qwerty00uioppoi"x65536' ) \
        6< <( sleep 0; perl -e 'print "0asT000YYUdf0ghj0"x65536' ) \
        7< <( sleep 0; perl -e 'print "zx00cvb00000nm"x65536' ) \
        10< <( printf '00' ) \
        > out;

    cat out | tr '0' '\n' | sort | uniq -c | perl -pe 's/^\s+/ /' | diff -u out.sample -
}

echo "Long line test"
echo "    (slow, requires minimum 120M of memory + 120M of disk space)"

perl -e 'print 35*1024*1024, " 1(", 50*1024*1024, ") ",35*1024*1024,"\n"' > out.sample
for((i=1; i<=2; ++i)) {
    echo "    iteration $i of 2";

    (ulimit -v 100000; ./fdlinecombine  6 5 7) \
        5< <( sleep 0; perl -e 'print "0\n" foreach 1..(35*1024*1024)' ) \
        6< <( sleep 0; perl -e 'print "1"x(1024*1024) foreach 1..50'; echo ) \
        7< <( sleep 0; perl -e 'print "2\n" foreach 1..(35*1024*1024)' ) \
        > out;

    LANG=C uniq -c out | perl -ne '/^\s*(\d+)\s+0$/ and $z+=$1 and next; /^\s*(\d+)\s+2$/ and $t+=$1 and next; /\s*(\d+)\s+(1+)/ and $o+=$1 and ($on=length $2) and next; print "fail: $_"; END { print "$z $o($on) $t\n" }' | cmp out.sample -
}

rm out.sample out
