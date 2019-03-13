#include <stdio.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#define MALLOC_SZ 1024 * 1024

void main() {
	int rand_gen = open("/dev/urandom", O_RDONLY);

	printf("Imma eat yo memz\n");

	for(;;) {
		char* buffer = malloc(MALLOC_SZ);
		read(rand_gen, buffer, MALLOC_SZ);
	}
}
