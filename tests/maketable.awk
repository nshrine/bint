#!/usr/bin/awk -f

BEGIN {
	OFS = "\t"
}

NR == 1 {
	for (i = 2; i <= 1001; i++) {
		$i = rand()
		sub(/0\.0?/, "rs", $i)
	}
	print
}

NR > 1 {
	for (i = 2; i <= 1001; i++) 
		$i = rand() OFS rand()
	print
}
