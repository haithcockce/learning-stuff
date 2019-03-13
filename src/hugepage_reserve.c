#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
 
void check_args(int argc, char **argv);
#define HP_SZ 2097152
 
#define USAGE "USAGE:\n"\
              "# hugepage_reserve <HP_CNT> [-t|--touch <TOUCH_CNT>]\n"\
              "      HP_CNT: count of hugepages to allocate\n"\
              "      TOUCH_CNT: count of hugepages to actually write to\n"\

int main(int argc, char **argv) {
    
    //check_args(argc, argv);
    char *addr;
    int hp_cnt = atoi(argv[1]);
 
    addr = mmap(NULL, hp_cnt * HP_SZ, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS|MAP_HUGETLB, -1, 0);
    if(addr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    sleep(10);
 
    memset(addr, 0, hp_cnt * HP_SZ);
 
    sleep(10);
    return 0;
}

void check_args(int argc, char **argv) {
    if (argc != 2 || argc != 4) {
        printf("ERROR: Incorrect amount of arguments\n%s", USAGE);
        exit(EINVAL);
    }
}
