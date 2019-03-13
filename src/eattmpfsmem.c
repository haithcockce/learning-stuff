#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void main() {
    printf("Sleeping 10 seconds before writing to a file in /mnt.\n");
    sleep(10);
    printf("Opening /mnt/test to write to, then sleeping for 10 more seconds.\n");
    FILE *fp;
    fp = fopen("/mnt/test", "w");
    sleep(10);
    printf("Writing to the file 512 MiB worth of 0s.\n");
    char* mem = (char*) malloc(512 * 1024 * 1024);
    memset(mem, 0, 512 * 1024 * 1024);
    fwrite(mem, sizeof(char), 512 * 1024 * 1024, fp);
    free(mem);
    printf("Wrote 512 MiB of 0s. Sleeping for 10 seconds.\n");
    sleep(10);
    printf("Removing file.\n");
    remove("/mnt/test");
    
}
