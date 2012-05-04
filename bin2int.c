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

/* 
 * File:   bin2int.c
 * Author: Nick
 *
 * Created on December 26, 2011, 4:34 PM
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>
#include "bint.h"
#include "getstrings.h"

const char *program_name;

static const struct option longopts[] =
{
  { "out", required_argument, NULL, 'o' },
  { "help", no_argument, NULL, 'h' },
  { "version", no_argument, NULL, 'v' },
  { NULL, 0, NULL, 0 }
};

static float get_value(FILE *, long, long);
static int keypos(FILE *, const char *, char ***, int *);
static int valsperkey(const char *, int);
static int linecount(FILE *);
static void print_help();
static void print_version();

void usage()
{
	printf("Usage: %s [OPTIONS] BASENAME KEY\n", program_name);
	printf("Try %s --help for more information\n", program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	program_name = argv[0];
	char outfile[IDLEN] = "";

	/* Process command arguments */
	int optc;
	while ((optc = getopt_long(argc, argv, "o:hv", longopts, NULL)) != -1) {
		switch (optc) {
			case 'o':
				strcpy(outfile, optarg);
				break;
      		case 'v':
        		print_version();
        		exit(EXIT_SUCCESS);
        		break;
      		case 'h':
        		print_help();
        		exit(EXIT_SUCCESS);
        		break;
      		default:
        		break;
      	}
	}
	if (optind > argc - 2)
		usage();

	char *key;
	char **snps, **ids;
	FILE *fp;
	enum type { unknown, snp, sample } key_type = unknown;
	int idx, nids, nsnps;

	setbase(argv[optind]);
	key = argv[optind + 1];
	if (strlen(outfile) < 1) {
		strcpy(outfile, key);
		strcat(outfile, ".int");
	}

	fp = fopen(idsfile, "r");
	if (!fp)
		error(1, errno, "%s", idsfile);

	nids = keypos(fp, key, &ids, &idx);
	fclose(fp);
	if (nids < 1)
		error(2, 0, "No ids in %s", idsfile);
	if (idx > -1)
		key_type = sample;

	fp = fopen(snpsfile, "r");
	if (!fp)
		error(1, errno, "%s", snpsfile);

	if (idx > -1) {
		nsnps = keypos(fp, key, &snps, NULL);
	} else {
		nsnps = keypos(fp, key, &snps, &idx);
		if (idx > -1)
			key_type = snp;
	}
	fclose(fp);
	if (nsnps < 1)
		error(2, 0, "No snps in %s", snpsfile);

	printf("SNPs: %d\nSamples: %d\n", nsnps, nids);

	if (key_type == unknown)
		error(2, 0, "%s not found", key);

	fp = fopen(binfile, "rb");
	if (!fp)
		error(1, errno, "%s", binfile);

	/* Read binary file header */
	if (!readheader(fp))
		exit(EXIT_FAILURE);
	printf("%s grouped by %s\n", binfile, snpmajor ? "SNP" : "sample");

	int perkey = valsperkey(binfile, nids * nsnps);
	if (perkey == -1) 
		exit(EXIT_FAILURE);
	else if (perkey == 0)
		error(2, 0, "%s", "No values read");
	printf("values per id: %d\n", perkey);

	long i, j, begin, end, incr, reclen, nx, ny;

	nx = snpmajor ? nids : nsnps;
	ny = snpmajor ? nsnps : nids;
	reclen = nx * perkey;

	if ((snpmajor && key_type == sample)
			|| (!snpmajor && key_type == snp)) {
		begin = (long) idx * perkey;
		end = begin + (ny * reclen);
		incr = reclen;
	} else if ((snpmajor && key_type == snp)
			|| (!snpmajor && key_type == sample)) {
		begin = (long) idx * reclen;
		end = begin + reclen;
		incr = perkey;
	}
		
	if (key_type == sample) {
		free(*ids);
		free(ids);
		ids = snps;
	} else {
		free(*snps);
		free(snps);
	}

#ifdef HAVE_MMAP
	int fd = fileno(fp);
	size_t size = nsnps * nids * perkey * sizeof(float)
			+ OFFSET * sizeof(float);
	float *map = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		/* close(fd); */
		error(0, errno, "Unable to memory map %s", binfile);
	}
#endif

	printf("Extracting to [ %s ]\n", outfile);
	FILE *fout = fopen(outfile, "w");
	if (!fout)
		error(1, errno, "%s", outfile);

	int k = 0;
	float value;
	for (i = begin; i < end; i += incr) {
		fprintf(fout, "%s\t", ids[k++]);
		for (j = 0; j < perkey; j++) {
#ifdef HAVE_MMAP
			if (map != MAP_FAILED)
				value = map[i + j + OFFSET];
			else
#endif
			value = get_value(fp, i, j);
			fprintf(fout, "%f", value);
			if (j < perkey - 1)
				fputc('\t', fout);
		}
		fputc('\n', fout);
		printf("\r%2.0f%%", ((float) i / (float) end) * 100.0);
		fflush(stdout);
	}
	printf("\rdone\n");
		
	fclose(fp);
	fclose(fout);
#ifdef HAVE_MMAP
	if (munmap(map, size) == -1)
		error(1, errno, "%s", "unmapping file");
	close(fd);
#endif

	free(*ids);
	free(ids);

	return (EXIT_SUCCESS);
}

static float get_value(FILE *fp, long i, long j)
{
	float value;
	long pos = (OFFSET + i + j) * sizeof(float);
	int ret = fseek(fp, pos, SEEK_SET);
	if (ret == -1)
		error(1, errno, "Error seeking %s", binfile);
	ret = fread(&value, sizeof(float), 1, fp);
	if (ret != 1)
		error(1, errno, "Error reading from %s", binfile);
	return value;
}

static int keypos(FILE *fp, const char *key, char ***names, int *pos)
{
	int i = 0, len = 0;
	char *line = NULL;
	char *rows;
	size_t n;

	if (pos != NULL)
		*pos = -1;

	len = linecount(fp);
	rows = malloc(len * IDLEN * sizeof(char));
	if (!rows)
		error(1, errno, "%s", "keypos rows");
	*names = malloc(len * sizeof(char *));
	if (!*names)
		error(1, errno, "%s", "keypos names");

	rewind(fp);
	while (getlin(&line, &n, fp) != -1) {
		if ((pos != NULL) && (strcmp(key, line) == 0))
			*pos = i;
		*(*names + i) = rows + (i * IDLEN);
		strcpy(*(*names + i), line);
		i++;
	}
	free(line);
	return i;
}

static int valsperkey(const char *path, int nkey)
{
	struct stat filestat;
	off_t valsize = nkey * sizeof(float);
	off_t datsize, rem;

	int perkey = stat(path, &filestat);
	if (perkey == -1) {
		error(0, errno, "Unable to fstat %s", binfile);
	} else {
		/* Data size is file size minus header */
		datsize = filestat.st_size - (OFFSET * sizeof(float));
		rem = datsize % valsize;
		if (rem > 0) {
			error(0, 0, "wrong values per key in %s", binfile);
			perkey = -1;
		} else {
			perkey = filestat.st_size / valsize;
		}
	}

	return perkey;
}

static int linecount(FILE *fp)
{
	int c, lines = 0;

	while ((c = getc(fp)) != EOF)
		if (c == '\n')
			lines++;

	return lines;
}

static void print_help()
{
	printf("\
Usage: %s [OPTIONS] BASENAME KEY\n", program_name);

	fputs("\
Extracts intensity values from binary file.\n", stdout);

	puts("");
	fputs("\
  -o, --out           output filename (default KEY.int)\n\
  -h, --help          display this help and exit\n\
  -v, --version       display version information and exit\n", stdout);

	printf("\n");
	fputs("\
BASENAME is base name of .bin, .snps, .ids files\n\
KEY is the snp or sample name to extract\n", stdout);
	puts("");

	puts("\
Examples:\n\
  bin2int mydata rs12345\n");
	puts("");

	printf("\
Report bugs to: %s\n", PACKAGE_BUGREPORT);
#ifdef PACKAGE_URL
	printf("%s home page: <%s>\n", PACKAGE_NAME, PACKAGE_URL);
#endif
}

/* Fixed while camping in the rain in South African jungle */
static void print_version()
{
	printf("bin2int (%s) %s\n", PACKAGE, VERSION);
	puts("");
	printf ("\
Copyright (C) %s Nick Shrine\n\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n",
              "2012");
}
