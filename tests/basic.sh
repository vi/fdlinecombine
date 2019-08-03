#!/bin/bash

# Basic

set -e

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
    ' > basic.sample
for((i=0; i<20; ++i)) {

    $1  6 5 7 8 9 10 11 12 13 14 15 16 17 \
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
        > basic;

    sort -n basic | cmp basic.sample -
}

rm basic.sample basic
