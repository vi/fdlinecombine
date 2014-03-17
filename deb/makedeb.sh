#!/bin/bash
set -e

P=fdlinecombine
V=0.4
FILES="fdlinecombine.c Makefile"

rm -Rf "$P"-$V
trap "rm -fR \"$P\"-$V" EXIT 
mkdir "$P"-$V
for i in $FILES; do cp -v ../"$i" "$P"-$V/; done
tar -czf ${P}_$V.orig.tar.gz "$P"-$V

cp -R debian "$P"-$V
(cd "$P"-$V && debuild)

