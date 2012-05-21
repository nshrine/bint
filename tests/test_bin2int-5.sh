#!/bin/sh

[ "$OSTYPE" = "msys" ] && DIFFARG=--strip-trailing-cr

bin2int -o id50_col1.int table_col1 id50 > /dev/null &&
bin2int -o id50_col2.int table_col2 id50 > /dev/null &&
bin2int -o id50_col12.int table_col12 id50 > /dev/null &&
cut -f 1,2 id50_col12.int | diff $DIFFARG id50_col1.int -  &&
cut -f 1,3 id50_col12.int | diff $DIFFARG id50_col2.int -
