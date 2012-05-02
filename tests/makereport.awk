#!/usr/bin/awk -f

BEGIN {
	OFS = "\t"
}

NR == 1 {
	tmp = $0
	cmd = "wc -l "FILENAME
	cmd | getline 
	nids = $1 - 1
	$0 = tmp
	nsnps = NF - 1
	print "[Header]"
	print "BSGT Version	3.3.4"
	print "Processing Date	30/4/2012 11:16 AM"
	print "Content		Human1-2M-DuoCustom_v1_A.bpm"
	print "Num SNPs	" nsnps
	print "Total SNPs	" nsnps
	print "Num Samples	" nids
	print "Total Samples	" nids
	print "[Data]"
	print "SNP Name	Sample ID	X	Y"
	for (i = 1; i <= nsnps; i++)
		snp[i] = $(i+1)
}

NR == 2 {
	perkey = (NF - 1) / nsnps
}

NR > 1 {
	for (i = 1; i <= nsnps; i++) 
		print snp[i], $1, $(i*2), $(i*2+1)
}
