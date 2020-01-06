#include <stdio.h>
#include <stdlib.h>

void pass_by_value(int);
void pass_by_reference(int*);

void main() {
    int i = 50;
    pass_by_value(i);
    printf("After pass_by_value: %d\n", i);
    pass_by_reference(&i);
    printf("After pass_by_reference: %d\n", i);
    char* a_string = "Hello world!\n";

    printf("0x%x, %s", a_string, a_string);

    char* iterator;
    for (iterator = a_string; iterator < (a_string + 5); iterator++) {
        printf("%c\n", *iterator);
    }

}


void pass_by_value(int i) {
    printf("Current value: %d\n", i);
    i = i * 2;
    printf("NOW: %d\n", i);
}

void pass_by_reference(int *i) {
    printf("Current value: %d\n", *i);
    *i = *i * 2;
}
