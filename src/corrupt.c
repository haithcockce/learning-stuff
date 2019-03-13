#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define USAGE "USAGE: corrupt <file>\n"

void main(int argc, char** argv) {

    // make sure the args are sane
    if(argc != 2) {
        printf("ERROR: Requires an argument.\n");
        printf(USAGE);
        exit(ENOENT);
    }
    FILE* fp = fopen(argv[1], "a+");
    if(!fp) {
        printf("ERROR: could not find or open file %s.\n", argv[1]);
        printf(USAGE);
        exit(ENOENT);
    }

    // Get the length of the file and then move to half-way into the file
    fseek(fp, 0, SEEK_END);
    unsigned long long file_size = ftell(fp);
    fseek(fp, (file_size / 2), SEEK_SET);

    // Write 0s to the file. Amount of 0s is approximately 1% of file size.
    unsigned long long corruption_amount = file_size / 100;
    int buffer[corruption_amount];
    memset(buffer, 'A', corruption_amount);
    fwrite(buffer, sizeof(char), corruption_amount, fp);

    // Clean up
    fsync(fp);
    fclose(fp);
}
