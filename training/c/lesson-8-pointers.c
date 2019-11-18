#include <stdio.h>

void main() {
    int i = 25; 
    int *pointer_i = &i; 

    char* a_string = "Hello world!\n";

    printf("0x%x, %s", a_string, a_string);

    char* iterator;
    for (iterator = a_string; iterator < (a_string + 5); iterator++) {
        printf("%c\n", *iterator);
    }
}
