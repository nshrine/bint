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
  { "cols", required_argument, NULL, 'c' },
  { "nomagic", no_argument, NULL, 'n' },
  { NULL, 0, NULL, 0 }
};

static int set_req_cols(const char *, req_cols *);
static int write_names(char *, const char *);
static int line_vals(indv_dat *, char *, req_cols *);
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
	req_cols rcols;
	char rcolstring[10] = "";

	/* Process command arguments */
	int usemagic = 1;
	int optc;
	while ((optc = getopt_long(argc, argv, "o:hvbc:n", longopts, NULL)) != -1) {
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
			case 'c':
				strncpy(rcolstring, optarg, 10);
				break;
			case 'n':
				usemagic = 0;
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

	FILE *fout = fopen(binfile, "wb");
	if (!fout)
		error(1, errno, "%s", binfile);
		
	/* Write the header */
	if (usemagic) {
		if (!writeheader(fout))
			exit(EXIT_FAILURE);
	} else {
		fprintf(stderr, "Writing binary file with no magic number\n");
	}

	printf("Converting intensity values to binary\n");
	fflush(stdout);
	int ny = 0;
	indv_dat dat = { NULL, NULL };
	while (getlin(&line, &n, fp) != -1) {

		/* Count number of vals in first line to alloc indv_dat */
		if (ny == 0) {
			read = line_vals(NULL, line, NULL);
			if (read < 1)
				error(2, 0, "%s", "No values found");
			if (read % nx != 0)
				error(2, 0, "%s", "Not same number of values for each marker");
			rcols.perkey = read / nx;
			printf("%d values per marker\n", rcols.perkey);

			/* Get the record length depending on how many cols required */
			int reclen = set_req_cols(rcolstring, &rcols);
			dat.vals = malloc(sizeof(float) * nx * reclen);
			if (dat.vals == NULL)
				error(1, errno, "%s", "dat.vals");
		}

		/* Read vals from each line */
		read = line_vals(&dat, line, &rcols);
		fputs(dat.id, fp2);
		fputc('\n', fp2);
		fwrite(dat.vals, sizeof(float), read, fout);
		ny++;
		printf("Read %d\r", ny);
		fflush(stdout);
	}
			
	if (strcmp(infile, "stdin") == 0)
		fclose(fp);
	fclose(fp2);
	fclose(fout);

	free(line);
	free(dat.vals);

	printf("\n[ %s ]\n", binfile);
	printf("Wrote %d ids to [ %s ]\n", ny, yfile);
	
	exit(EXIT_SUCCESS);
}

static int set_req_cols(const char *arg, req_cols *rcols)
{
	int i;
	rcols->nreq = strlen(arg);
	
	if (rcols->nreq == 0) {
		rcols->nreq = rcols->perkey;
		for (i = 0; i < rcols->perkey; i++)
			rcols->cols[i] = i + 1;
	} else if (rcols->nreq > rcols->perkey) {
		error(2, 0, "%d columns specified, only %d available",
				rcols->nreq, rcols->perkey);
	} else {
		for (i = 0; i < rcols->nreq; i++) {
			if (!isdigit(arg[i]) || arg[i] == '0')
				error(2, 0, "Invalid column specifier: %c", arg[i]);
			const char coli[2] = { arg[i], '\0' };
			rcols->cols[i] = atoi(coli);
			if (rcols->cols[i] > rcols->perkey)
				error(2, 0, "Column %d specified, only %d available",
						rcols->cols[i], rcols->perkey);
		}
	}

	return rcols->nreq;
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
		char *name = strtok(line, " \t");

		/* Write each space/tab delimited name to file */
		while ((name = strtok(NULL, " \t")) != NULL) {
			fputs(name, fp);
			fputc('\n', fp);
			n++;
		}
		fclose(fp);
	}

	return n;
}

static int line_vals(indv_dat *dat, char *line, req_cols *rcols)
{
	char *snp, *end, *tmp;
	if (dat == NULL) {
		tmp = malloc((strlen(line) + 1) * sizeof(char));
		strcpy(tmp, line);
	} else {
		tmp = line;
	}

	int i = 0, j = 0, valno, k;
	char *id = strtok(tmp, " \t");
	if (dat != NULL)
		dat->id = id;
	while ((snp = strtok(NULL, " \t")) != NULL) {
		if (dat != NULL) {
			valno = i % rcols->perkey + 1;
			for (k = 0; k < rcols->nreq; k++) {
				if (rcols->cols[k] == valno) {
					dat->vals[j] = strtof(snp, &end);
					if (snp == end)
						dat->vals[j] = NAN;
					j++;
				}
			}
		}
		i++;
	}

	if (dat == NULL) {
		free(tmp);
		j = i;
	}

	return j;
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
  -b, --bysnp         file is snp-per-row instead of sample-per-row\n\
  -c, --cols N        specify columns to include when multiple values per id/snp\n\
  -n, --nomagic       do not write a magic number to header of binary file\n", DEFAULT_OUT);

	printf("\n");
	fputs("\
FILE is table of X,Y or LRR,BAF values, assumed to be\n\
SNP columns and sample rows or vice-versa if --bysnp given.\n\
When FILE is blank or -, read from standard input\n", stdout);
	puts("");

	puts("\
Examples:\n\
  int2bin --cols 34 mydata lrrbaf.txt\n\n\
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
