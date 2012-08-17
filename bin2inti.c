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

const int base = 10;

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s BINFILE IDXFILE\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char *binfile = argv[1];
	char *idxfile = argv[2];
	struct stat buf;

	int binfd = open(binfile, O_RDONLY);
	if (binfd == -1)
		error(1, errno, "%s", binfile);
	if (fstat(binfd, &buf) == -1)
		error(1, errno, "%s", binfile);
	off_t size = buf.st_size;
	
	FILE *idxfp = fopen(idxfile, "rb");
	if (!idxfp)
		error(1, errno, "%s", idxfile);
	if (stat(idxfile, &buf) == -1)
		error(1, errno, "%s", idxfile);
	size_t nvals = buf.st_size / sizeof(double);
	
	float *map = mmap(0, size, PROT_READ, MAP_SHARED, binfd, 0);
	if (map == MAP_FAILED) {
		error(1, errno, "Unable to memory map %s", binfile);
    }

	double didx[nvals];
	if (fread(&didx, sizeof(double), nvals, idxfp) != nvals)
		error(1, errno, "%s", idxfile);
	
	size_t i;
	long idx;
	for (i = 0; i < nvals; i++) {
		idx = (long) didx[i];
		if (idx < 0 || idx >= size / sizeof(float)) 
			error(2, 0, "Index %zu out of range: %ld", i, idx);
		printf("%f\n", map[idx]);
	}

	if (munmap(map, size) == -1)
		error(1, errno, "%s", "unmapping file");
	close(binfd);
	fclose(idxfp);

	exit(EXIT_SUCCESS);
}
