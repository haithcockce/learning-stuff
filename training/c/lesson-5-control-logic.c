#include <stdio.h>

void if_examples(void);
void loop_examples(void);

void if_examples(void) {
    int i = 0;

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
        print("This, however, will be executed!\n");
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

void main() {
}
