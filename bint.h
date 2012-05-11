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
 * File:   bint.h
 * Author: Nick
 *
 * Created on December 26, 2011, 4:39 PM
 */

#ifndef _BINT_H
#define	_BINT_H

#ifdef	__cplusplus
extern "C" {
#endif

#define IDLEN 50
#define OFFSET 2

/* Random magic number to identify bint file */
extern const float magic_no;

char *base, *infile;
char binfile[IDLEN], snpsfile[IDLEN], idsfile[IDLEN];
int snpmajor;

typedef struct {
	char *id;
	float *vals;
} indv_dat;

void setbase(const char *);
int writeheader(FILE *);
int readheader(FILE *);

#ifdef	__cplusplus
}
#endif

#endif	/* _BINT_H */
