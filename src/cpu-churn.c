#include <stdio.h> 
#include <limits.h>

void main() {
    unsigned long i;
    unsigned long temp;

    printf("Starting churn...");
    unsigned long BASE = INT_MAX;
    unsigned long MAX = BASE * 50ul;
    for (i = 0; i < MAX; i++) {
        temp += i - 1;
    }
}
	
