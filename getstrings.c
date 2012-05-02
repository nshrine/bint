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
#include <string.h>
#include <stdio.h>

size_t getlin(char **line, size_t *n, FILE *fp)
{
	int read = -1;
	
	read = getline(line, n, fp);
	if (read > -1) {
		char *p = strchr(*line, '\n');
		if (p)
			*p = '\0';
	}
	return read;
}

int getstrings(char *input, char ***output, const char *delim)
{
	int n, i, l;
	char **result, *tmp, *word;

	if (output == NULL) {
		l = strlen(input);
		tmp = malloc((l + 1) * sizeof(char));
		strcpy(tmp, input);
		input = tmp;
	} else {
		n = getstrings(input, NULL, delim);
		result = malloc(n * sizeof(char *));
	} 
	
	i = 0;
	word = strtok(input, delim);
	do {
		if (output != NULL) {
			l = strlen(word);
			*(result + i) = malloc((l + 1) * sizeof(char));
			strcpy(*(result + i), word);
		}
		i++;
	} while((word = strtok(NULL, delim)) != NULL);

	if (output == NULL)
		free(tmp);
	else
		*output = result;

	return i;
}

void freestrings(char **strings, int n)
{
	int i;
	for (i = 0; i < n; i++)
		free(strings[i]);
	free(strings);
}

/*
int strcount(char *input, char delim) {
	int i = 0, n = 0;

	do {
		if (i > 0 && input[i] == delim)
			if (input[i-1] != delim)
				n++;
	} while (input[++i] != '\0');

	if (input[i] == delim)
		n--;

	return ++n;
}
*/
