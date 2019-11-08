#include <stdio.h>

void main() {
    char* a_string = "Hello world!\n";

    printf("0x%x, %s", a_string, a_string);

    char* iterator;
    for (iterator = a_string; iterator < (a_string + 5); iterator++) {
        printf("%s", iterator);
    }
}
