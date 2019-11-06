#include <stdio.h>

void boolean_examples(void);
void if_examples(void);
void loop_examples(void);


void boolean_examples() {
    int x = 0;
    int y = 25;
    printf("Boolean examples\n");
    printf("x = %d, y = %d\n", x, y);
    printf("x || y => %d, x && y => %d, !y => %d\n", (x||y), (x&&y), (!y));
}

void if_examples(void) {
    int i = 0;

    printf("if statement\n");

    /* if statement
     *
     * wrapped code is executed only when the tested expression is true or has
     * a value that is not 1.
     */
    if (i > 0) {
        printf("This will not execute because 'i' is 0!\n");
    }

    /* if-else statement
     *
     * like the if statement above, except if the expression is false, then the
     * else statement is ran. 
     */
    if (i > 0) {
        printf("This still will never be executed!\n");
    }
    else {
        printf("This, however, will be executed!\n");
    }

    /* if-else if-else statement
     * 
     * Like above, but we can group additional evaluations in the event we want
     * to do one of many things based on some condition. 
     */
    if (i > 0) {
        printf("Another useless print because it will never be executed!\n");
    }
    else if (i == 0) {
        printf("This should print because i == 0!\n");
    }
    else {
        printf("And this should not print either\n");
    }
}

void loop_examples() {
    int i = 0;

    printf("Loop examples\n");
    while (i++ < 3) {
        printf("I should print 3 times, and i is currently %d\n", i);
    }

    for (i = 0; i < 3; i++) {
        printf("I'm just a fancy while loop\n");
    }

    /* you can make infinite loops! */
    i = 0;
    do {
        printf("Wait is this infinite?\n");
        if (++i == 3) {
            break;
        }
    } while (1);

    /* All areas of a for loop are optional! */
    for (i = 0;;) {
        printf("Why would you make a loop like this?\n");
        if (++i == 3) {
            break;
        }
    }

}


void main() {
    boolean_examples();
    if_examples();
    loop_examples();
}
