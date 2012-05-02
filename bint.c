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
#include <error.h>
#include <errno.h>
#include "bint.h"

const float magic_no = 157129.0;

void setbase(const char *name)
{
	strcpy(binfile, name);
	strcat(binfile, ".bin");
	strcpy(snpsfile, name);
	strcat(snpsfile, ".snps");
	strcpy(idsfile, name);
	strcat(idsfile, ".ids");
}

int writeheader(FILE *fp)
{
	size_t count;
	float mode = (float) snpmajor;
	count = fwrite(&magic_no, sizeof(float), 1, fp);
	count += fwrite(&mode, sizeof(float), 1, fp);

	if (count != OFFSET) {
		error(0, errno, "Unable to write header to %s", binfile);
		count = 0;
	}

	return count;
}
	
int readheader(FILE *fp)
{
	size_t count;
	float m, mode;
	count = fread(&m, sizeof(float), 1, fp);
	count += fread(&mode, sizeof(float), 1, fp);
	snpmajor = (int) mode;

	if (count != OFFSET) {
		error(0, errno, "Unable to read header from %s", binfile);
		count = 0;
	} else if (m != magic_no) {
		error(0, 0, "Wrong magic number, %s not a bint file?", binfile);
		count = 0;
	} else if (snpmajor != 0 && snpmajor != 1) {
		error(0, 0, "Major mode field corrupt in %s, mode: %d", binfile, snpmajor);
		count = 0;
	}

	return count;
}

/*
void error(int status, int errnum, const char *format, ...)
{
	va_list args;
	
	fflush(stdout);
	fprintf(stderr, "%s: ", program_name);
	
	va_start(args, format);
	vfprintf (stderr, format, args);
	va_end(args);
	
	if (errnum != 0)
		fprintf(stderr, ": %s", strerror(errnum));
	fprintf(stderr, "\n");
	
	if (status != 0)
		exit(status);
}
*/

