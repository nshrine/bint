#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <R.h>

static const int base = 10;

void getbin(char **binfile, double *didx, int *nvals, double *vals)
{
	int binfd = open(*binfile, O_RDONLY);
	if (binfd == -1)
		error("%s", *binfile);

	struct stat buf;
	if (fstat(binfd, &buf) == -1)
		error("%s", *binfile);
	off_t size = buf.st_size;
	
	float *map = mmap(0, size, PROT_READ, MAP_SHARED, binfd, 0);
	if (map == MAP_FAILED)
		error("Unable to memory map %s", *binfile);

	int i;
	long idx;
	for (i = 0; i < *nvals; i++) {
		idx = (long) didx[i];
		if (idx < 0 || idx >= size / sizeof(float)) 
			error("Index %zu out of range: %ld", i, idx);
		vals[i] = map[idx];
	}

	if (munmap(map, size) == -1)
		error("%s", "unmapping file");
	close(binfd);
}
