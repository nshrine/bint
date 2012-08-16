#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <getopt.h>
#include <unistd.h>
#include <error.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s BINFILE IDSFILE\n", argv[0]);
		exit(1);
	}

	char *binfile = argv[1];
	char *idxfile = argv[2];
	FILE *binfp = fopen(binfile, "r");
	if (!binfp)
		error(1, errno, "%s", binfile);
	FILE *idxfp;
	if (strcmp(idxfile, "-") == 0)
		idxfp = stdin;
	else
		idxfp = fopen(idxfile, "r");
	if (!idxfp)
		error(1, errno, "%s", idxfile);
	
	struct stat buf;
	if (stat(binfile, &buf) == -1)
		error(1, errno, "%s", binfile);
	off_t size = buf.st_size;
	float *map = mmap(0, size, PROT_READ, MAP_SHARED, fileno(binfp), 0);
	if (map == MAP_FAILED) {
		error(1, errno, "Unable to memory map %s", binfile);
    }

	char *line = NULL, *word, *p;
	size_t n, nvals, i;

	getline(&line, &n, idxfp);
	strtok(line, " ");
	nvals = 1;
	while (strtok(NULL, " ") != NULL)
		nvals++;

	unsigned long idx[nvals];
	rewind(idxfp);
	while (getline(&line, &n, idxfp) != -1) {
		p = strchr(line, '\n');
		if (p)
			*p = '\0';
		i = 0;
		word = strtok(line, " ");
		idx[i++] = atol(word);
		while ((word = strtok(NULL, " ")) != NULL)
			idx[i++] = atol(word);
		for (i = 0; i < nvals; i++)
			printf("%f\n", map[idx[i]]);
	}

	free(line);
	if (munmap(map, size) == -1)
		error(1, errno, "%s", "unmapping file");
	fclose(idxfp);
	fclose(binfp);

	exit(EXIT_SUCCESS);
}
