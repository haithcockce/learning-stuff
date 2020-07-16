#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define K 1024
#define LIMIT (K * K * 1000)

void main() {
	int* throw_away;
	int i;
	int read_rand;
	int rand_gen = open("/dev/urandom", O_RDONLY);

	for (i = 0; i < LIMIT; i += K) {
       		throw_away = (int*) malloc(K * 4); 
		read_rand = read(rand_gen, throw_away, K);
		if (i % (64 * K) == 0) {
			sleep(1);
		}
	}
	sleep(1000);
}
