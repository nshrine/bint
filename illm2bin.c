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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include "bint.h"
#include "illm.h"

#define BUF 255
#define NL 50
#define MAX_COLS 4
#define DEFAULT_OUT "final_report"

const char *lrr_col[1] = { LRR_COL };
const char *baf_col[1] = { BAF_COL };
const char *cnv_col[2] = { LRR_COL, BAF_COL };
const char *xy_col[2] = { X_COL, Y_COL };
const char *program_name;

static const struct option longopts[] =
{
  { "out", required_argument, NULL, 'o' },
  { "help", no_argument, NULL, 'h' },
  { "cnv", no_argument, NULL, 'c' },
  { "xy", no_argument, NULL, 'x' },
  { "lrr", no_argument, NULL, 'l' },
  { "baf", no_argument, NULL, 'b' },
  { "version", no_argument, NULL, 'v' },
  { "nomagic", no_argument, NULL, 'n' },
  { NULL, 0, NULL, 0 }
};

int nsnps = 0, nsamp = 0, ncol = 0, lastcol;
int colsnps[MAX_COLS];
int snpmajor;

typedef struct {
	char snp[NL];
	char id[NL];
	float val[MAX_COLS];
} data_line;

static int read_header(FILE *);
static int get_colsnps(FILE *, const char *[]);
static char * readline(char *, int, FILE *);
static int read_data(data_line *, FILE *);
static void print_help();
static void print_version();

/*
static void copyline(data_line *, data_line *);
*/

static void usage()
{
	printf("Usage: %s [OPTIONS] FILE\n", program_name);
	printf("Try %s --help for more information\n", program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	program_name = argv[0];
	
	/* Set default columns and ouput file */
	const char **req = cnv_col;
	ncol = 2;
	setbase(DEFAULT_OUT);
	int i;
	for (i = 0; i < MAX_COLS; i++)
		colsnps[i] = -1;

	/* Process command arguments */
	int usemagic = 1;
	int optc;
	while ((optc = getopt_long(argc, argv, "o:hcxlbvn", longopts, NULL)) != -1) {
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
	  		case 'c':
				req = cnv_col;
				ncol = 2;
				break;
	  		case 'x':
				req = xy_col;
				ncol = 2;
				break;
			case 'l':
				req = lrr_col;
				ncol = 1;
				break;
			case 'b':
				req = baf_col;
				ncol = 1;
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

	/* Open the input file or stdin */
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

	/* Get the snp and sample numbers from the header */
	if (!read_header(fp))
		error(2, 0, "Unable to get snp/sample number from %s\n", infile);
	printf("SNPs: %d\nSamples: %d\n", nsnps, nsamp);

	/* Find the required columns in the header line */
	if (!get_colsnps(fp, req))
		error(2, 0, "Cannot find required columns in %s\n", infile);
	printf("Required columns:");
	for (i = 0; i < ncol; i++)
		printf(" %d", colsnps[i] + 1); 
	printf("\n");

	/* Read the first two lines and determine whether file is by SNP or by Sample */
	data_line line1;
	if (read_data(&line1, fp) == -1)
		error(2, errno, "Can't read first data line in %s\n", infile);
	data_line line2;
	if (read_data(&line2, fp) == -1)
		error(2, errno, "Can't read second data line in %s\n", infile);
	if (strcmp(line1.snp, line2.snp) == 0)
		snpmajor = 1;
	else if (strcmp(line2.id, line2.id) == 0)
		snpmajor = 0;
	else
		error(2, 0, "Can't determine major mode from %s\n", infile);
	printf("%s grouped by %s\n", infile, snpmajor ? "SNP" : "sample");
	int n = snpmajor ? nsnps : nsamp;
	int m = snpmajor ? nsamp : nsnps;

	/* Allocate enough data_line structures for a complete SNP or Sample */
	data_line *lines = malloc(m * sizeof(data_line));
	if (!lines)
		error(1, errno, "%s", "lines malloc");

	/* Copy the first two lines already read from file into the array */
	memcpy(&lines[0], &line1, sizeof(data_line));
	memcpy(&lines[1], &line2, sizeof(data_line));

	/* Allocate a float buffer to be filled then written to binary file */
	float *vals = malloc(ncol * m * sizeof(float));
	if (!vals)
		error(1, errno, "%s", "vals malloc");

	/* Open the output files */
	const char *xfile = snpmajor ? idsfile : snpsfile;
	FILE *fpx = fopen(xfile, "w");
	if (!fpx)
		error(1, errno, "%s", xfile);

	const char *yfile = snpmajor ? snpsfile : idsfile;
	FILE *fpy = fopen(yfile, "w");	
	if (!fpy)
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

	/* Loop through lines filling float buffer then writing to files. */
	int j, k;
	for (i = 0; i < n; i++) {
		for (j = 0; j < m; j++) {

			/* Start reading after the 2 lines already read */
			if (i > 0 || j > 1)
				if (read_data(&lines[j], fp) == -1) 
					error(2, errno, "Fewer lines than expected in %s\n", infile);
			for (k = 0; k < ncol; k++)
				vals[j * ncol + k] = lines[j].val[k];
			if (i == 0)
				fprintf(fpx, "%s\n", snpmajor ? lines[j].id : lines[j].snp);
			if (j == 0)
				fprintf(fpy, "%s\n", snpmajor ? lines[j].snp : lines[j].id);
		}
		fwrite(vals, sizeof(float), ncol * m, fout);
		printf("\rConverting to binary...%2.1f%%",
				((float) (i + 1) / (float) n) * 100.0);
		fflush(stdout);
	}
	printf("\n");
				
	fclose(fpx);
	fclose(fpy);
	fclose(fout);
	if (strcmp(infile, "stdin") != 0)
		fclose(fp);

	free(vals);
	free(lines);

	printf("Binary data [ %s ]\n", binfile);
	printf("SNP names [ %s ]\n",  snpsfile);
	printf("Sample names [ %s ]\n", idsfile);

	exit(EXIT_SUCCESS);
}

static int read_header(FILE *fp)
{
	int result = 0, match;
	char headline[BUF];
	char *name;

	do {
		readline(headline, BUF, fp);
		int n = strlen(headline);
		char tmp[n + 1];
		strcpy(tmp, headline);
		name = strtok(tmp, "\t");
		char *value = strtok(NULL, "\t");
		if (!nsnps && strcmp(name, SNPLINE) == 0)
			nsnps = atoi(value);
		else if (nsnps && strcmp(name, SAMPLINE) == 0)
			nsamp = atoi(value);
		match = strcmp(name, DATA);
	} while (headline != NULL && match != 0);

	if (nsnps > -1 && nsamp > -1)
		result = 1;
	return result;
}

static int get_colsnps(FILE *fp, const char *reqcols[])
{
	int result = 0;
	char colnames[BUF];
	readline(colnames, BUF, fp);
	char *colname = strtok(colnames, "\t");

	int i = 0, k;
	do {
		for (k = 0; k < ncol; k++) {
			if (strcmp(colname, reqcols[k]) == 0) {
				colsnps[k] = i;
				if (i > lastcol)
					lastcol = i;
			}
		}
		i++;
	} while ((colname = strtok(NULL, "\t")) != NULL);

	result = 1;
	for (k = 0; k < ncol; k++) {
		if (colsnps[k] < 0)
			result = 0;
	}
	return result;
}

static int read_data(data_line *data, FILE *fp) {
	char line[BUF];
	char *tmp;
	int k;

	if (readline(line, BUF, fp)) {
		tmp = strtok(line, "\t");
		strcpy(data->snp, tmp);
		tmp = strtok(NULL, "\t");
		strcpy(data->id, tmp);

		int i = 2;
		for (k = 0; k < ncol; k++) 
			data->val[k] = NAN;

		char *end;
		while ((tmp = strtok(NULL, "\t")) != NULL && i <= lastcol) {
			for (k = 0; k < ncol; k++) {
				if (i == colsnps[k]) {
					data->val[k] = strtof(tmp, &end);
					if (tmp == end)
						data->val[k] = NAN;
				}
		
			}
			i++;
		}
	} else {
		k = -1;
	}

	return k;
}
	
static char * readline(char *line, int n, FILE *fp)
{
	char *result = fgets(line, n, fp);

	if (result) {
		char *p = strstr(line, "\n");
		if (p)
			*p = '\0';
		p = strstr(line, "\r");
		if (p)
			*p = '\0';
	}

	return result;
}

/*
static void copyline(data_line *to, data_line *from) {
	strcpy(to->snp, from->snp);
	strcpy(to->id, from->id);
	to->x = from->x;
	to->y = from->y;
}
*/

static void print_help()
{
	printf("\
Usage: %s [OPTIONS] FILE\n", program_name);

	fputs("\
Converts required columns of illumina final report file to binary.\n", stdout);

	puts("");
	printf("\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n\
  -o, --out NAME      specify output filename (default %s)\n\
  -c, --cnv           get cnv analysis columns LRR & BAF (default)\n\
  -x, --xy            get X, Y intensity columns\n\
  -l, --lrr           get LRR column only\n\
  -b, --baf           get BAF column only\n", DEFAULT_OUT);

	printf("\n");
	fputs("\
FILE is illumina final report file output by Bead/GenomeStudio\n\
When FILE is -, read from standard input\n", stdout);
	puts("");

	puts("\
Examples:\n\
  illm2bin --cnv --out mydata final_report.txt\n\n\
  zcat final_report.txt.gz | illm2bin --lrr --out lrronly -\n");
	puts("");

	printf("\
Report bugs to: %s\n", PACKAGE_BUGREPORT);
#ifdef PACKAGE_URL
	printf("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

static void print_version()
{
	printf("illm2bin (%s) %s\n", PACKAGE, VERSION);
	puts("");
	printf ("\
Copyright (C) %s Nick Shrine.\n\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n",
              "2012");
}
