#!/bin/sh

bin2int -o id50_col1.int table_col1 id50 > /dev/null &&
bin2int -o id50_col2.int table_col2 id50 > /dev/null &&
bin2int -o id50_col12.int table_col12 id50 > /dev/null &&
join -t "	" id50_col1.int id50_col2.int | diff id50_col12.int -
