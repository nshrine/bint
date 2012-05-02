/* This file is part of bint

   Copyright 2012, Nick Shrine

   bint is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   bint is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include "bint.h"
#include "getstrings.h"

#define DEFAULT_OUT "table_out"

const char *program_name;

static const struct option longopts[] =
{
  { "out", required_argument, NULL, 'o' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { "bysnp", no_argument, NULL, 'b' },
  { NULL, 0, NULL, 0 }
};

static int write_names(char *, const char *);
static int line_vals(indv_dat *, char *);
static void print_help();
static void print_version();

static void usage()
{
	printf("Usage: %s [OPTIONS] FILE\n", program_name);
	printf("Try %s --help for more information\n", program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	program_name = argv[0];
	setbase(DEFAULT_OUT);

	/* Assume row per id */
	snpmajor = 0;

	/* Process command arguments */
	int optc;
	while ((optc = getopt_long(argc, argv, "o:hvb", longopts, NULL)) != -1) {
		switch (optc) {
			case 'o':
				setbase(optarg);
				break;
      		case 'v':
        		print_version();
        		exit(EXIT_SUCCESS);
        		break;
      		case 'h':
        		print_help();
        		exit(EXIT_SUCCESS);
        		break;
			case 'b':
				snpmajor = 1;
				break;
      		default:
        		break;
      	}
	}
	if (optind == argc)
		usage();

	FILE *fp;
	infile = argv[optind];
	if (strcmp(infile, "-") == 0) {
		infile = "stdin";
		fp = stdin;
	} else {
		fp = fopen(infile, "r");
		if (!fp) 
			error(1, errno, "%s", infile);
	}

	/* Get the first line with the names */
	char *line = NULL;
	size_t n = 0;
	size_t read = getlin(&line, &n, fp);
	if (read == -1)
		error(1, errno, "%s", infile);

	const char *xfile = snpmajor ? idsfile : snpsfile;
	int nx = write_names(line, xfile);
	if (nx == -1)
		exit(EXIT_FAILURE);
	else
		printf("Wrote %d names to [ %s ]\n", nx, xfile);

	const char *yfile = snpmajor ? snpsfile : idsfile;
	FILE *fp2 = fopen(yfile, "w");
	if (!fp2)
		error(1, errno, "%s", yfile);

	FILE *fout = fopen(binfile, "w");
	if (!fout)
		error(1, errno, "%s", binfile);
		
	/* Write the header */
	if (!writeheader(fout))
		exit(EXIT_FAILURE);

	printf("Converting intensity values to binary\n");
	fflush(stdout);
	unsigned int ny = 0;
	indv_dat dat = { NULL, NULL };
	while (getlin(&line, &n, fp) != -1) {

		/* Count number of vals in first line to alloc indv_dat */
		if (ny == 0) {
			read = line_vals(NULL, line);
			if (read < 1)
				error(2, 0, "%s", "No values found");
			if (read % nx != 0)
				error(2, 0, "%s", "Not same number of values for each marker");
			printf("%zu values per marker\n", read / nx);
			dat.vals = malloc(sizeof(float) * read);
			if (dat.vals == NULL)
				error(1, errno, "%s", "dat.vals");
		}

		/* Read vals from each line */
		read = line_vals(&dat, line);
		fputs(dat.id, fp2);
		fputc('\n', fp2);
		fwrite(dat.vals, sizeof(float), read, fout);
		ny++;
		printf("Read %u\r", ny);
		fflush(stdout);
	}
			
	fclose(fp);
	fclose(fp2);
	fclose(fout);

	free(line);
	free(dat.vals);

	printf("\n[ %s ]\n", binfile);
	printf("Wrote %u ids to [ %s ]\n", ny, yfile);
	
	exit(EXIT_SUCCESS);
}

static int write_names(char *line, const char *file)
{
	int n = 0;

	FILE *fp = fopen(file, "w");
	if (!fp) {
		error(0, errno, "%s", file);
		n = -1;
	} else {

		/* Skip the first word which will be "Sample" or "Barcode" */
		char *name = strtok(line, "\t");

		/* Write each tab delimited name to file */
		while ((name = strtok(NULL, "\t")) != NULL) {
			fputs(name, fp);
			fputc('\n', fp);
			n++;
		}
		fclose(fp);
	}

	return n;
}

static int line_vals(indv_dat *dat, char *line)
{
	char *snp, *end, *tmp;
	if (dat == NULL) {
		tmp = malloc((strlen(line) + 1) * sizeof(char));
		strcpy(tmp, line);
	} else
		tmp = line;
	int i = 0;
	char *id = strtok(tmp, "\t");
	while ((snp = strtok(NULL, "\t")) != NULL) {
		if (dat != NULL) {
			dat->id = id;
			dat->vals[i] = strtof(snp, &end);
			if (snp == end)
				dat->vals[i] = NAN;
		}
		i++;
	}
	if (dat == NULL)
		free(tmp);
	return i;
}

static void print_help()
{
	printf("\
Usage: %s BASENAME [FILE]\n", program_name);

	fputs("\
Converts table of intensity values to binary.\n", stdout);

	puts("");
	printf("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n\
  -o, --out NAME      specify output filename (default %s)\n\
  -b, --bysnp         file is snp-per-row instead of sample-per-row\n", DEFAULT_OUT);

	printf("\n");
	fputs("\
FILE is table of X,Y or LRR,BAF values, assumed to be\n\
SNP columns and sample rows or vice-versa if --bysnp given.\n\
When FILE is blank or -, read from standard input\n", stdout);
	puts("");

	puts("\
Examples:\n\
  int2bin mydata lrrbaf.txt\n\n\
  zcat lrrbaf.txt.gz | int2bin mydata -\n");
	puts("");

	printf("\
Report bugs to: %s\n", PACKAGE_BUGREPORT);
#ifdef PACKAGE_URL
	printf("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

static void print_version()
{
	printf("int2bin (%s) %s\n", PACKAGE, VERSION);
	puts("");
	printf ("\
Copyright (C) %s Nick Shrine\n\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n",
              "2012");
}
