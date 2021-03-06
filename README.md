This is the README file for the bint distribution.
bint converts intensity data text files to binary for fast subsetting.

  Copyright (C) 2012 Nick Shrine

  Copying and distribution of this file, with or without modification,
  are permitted in any medium without royalty provided the copyright
  notice and this notice are preserved.

See the file INSTALL for building and installation instructions.

# Overview

bint was written for the purpose of obtaining subsets of intensity
data from genotyping assays. Either the X & Y intensities for 
creating cluster plots or Log R Ratio and B Allele Frequencies for
CNV detection. Extracting the data for individual SNP/CNV markers
or individual samples was slow grep/awk'ing the text files
exported from the genotyping run (e.g. Illumina final report files).
bint converts the text representation of the intensity float data
to into a IEEE754 indexed binary file for rapid extraction of 
subsets of the data. In theory bint could be used for any large
tables of float data.

# Input files

The input files are either tab-separated tabular as described
below or Illumina final report files. When you do "make check"
examples of these input files are created in the tests
directory, table.txt and final_report.txt, respectively.

# Programs

int2bin
: takes a tab-separated text file like below and converts it
to binary. There may be more than 1 tab-separated value per cell.

```
 Barcode snp1    snp2    ...    snpM
 id1     0.16    0.28    ...    -0.02  
 id2     0.82    NA      ...    1.1
  :      ...     ...     ...   	: 
  :      ...     ...     ...   	:
 idN     -1.3e-2 0.01    ...    0.34
```

illm2bin
: takes a final report file exported from Illumina's
Bead/GenomeStudio software and converts it to binary.

bin2int
: extracts the intensity data for a single SNP/CNV marker
or single sample from a binary file.

The grouping of the data, either by SNP or by sample is recorded
in the binary file header e.g. the input file for int2bin above
could have rows of SNPs and columns of samples instead.

See the man pages or --help output for info on options.

# Output

int2bin and illm2bin produce 3 files:

data.bin
: the binary data

data.snps
: the list of SNP/CNV markers in the binary file

data.ids
: the list of samples in the binary file

The .snps and .ids files are single column readable text.

bin2int produces a text file in columns with a .int extension.
The first column will contain the SNP/CNV marker names or sample
IDS depending on whether you specified a sample or a SNP/CNV to
extract the data for respectively. Subsequent columns contain the
intensity values. The number of columns depends on how many values
per id there are, usually 2 if it is X/Y or LRR/BAF data.

# Dependencies

The files in lib and m4 are imported from gnulib for portability
to non-glibc systems like BSD and MINGW.
	(http://www.gnu.org/software/gnulib/)

Tested on Linux, FreeBSD and Windows.
Building on Windows requires MINGW (http://www.mingw.org/)

bint is free software.  See the file COPYING for copying conditions.

Report bugs to: nrgs@users.sourceforge.net
bint home page: <http://sourceforge.net/projects/bint/>
