#include <stdio.h>


void print_interpretations(void);
void print_sizes(void);
void print_bitwise_arithmetic(void);

void print_interpretations() {
    float f = 65.5;
    printf("f = 65.5 Various representations:\n");
    char c = (char) f;
    int i = (int) f;
    printf("char: %c, integer: %d, hexadecimal: 0x%x, floating point: %f\n", c, i, f, f);
}

void print_bitwise_arithmetic() {
    int i = 0; 
    int j = 15;
    printf("int i = 0x%x, int j = 0x%x\n", i, j);
    printf("~i = 0x%x\n", ~i);
    printf("i & j = 0x%x\n", (i&j));
    printf("i | j = 0x%x\n", (i|j));
    printf("i ^ j = 0x%x\n", (i^j));
    printf("j << 2 =0x%x\n", (j<<2));
    printf("j >> 2 = 0x%x\n", (j>>2));
    printf("(~i) << 2 = 0x%x\n", ((~i)<<2));
    printf("signed   (~i) >> 2 = 0x%x\n", ((~i)>>2));
    printf("unsigned (~i) >> 2 = 0x%x\n", ((unsigned) (~i))>>2);
    printf("i - 1 = 0x%x\n", ((unsigned int) i) - 1);
    printf("(~i) + 1 = 0x%x\n", ((~i)+1));
}

void print_sizes() {
    printf("Size of char: %d bytes\n", sizeof(char));
    printf("Size of int: %d bytes\n", sizeof(int));
    printf("Size of long: %d bytes\n", sizeof(long));
    printf("Size of double: %d bytes\n", sizeof(double));
    printf("Size of float: %d bytes\n", sizeof(float));
}
 


void main() {
    print_interpretations();
    print_sizes();
    print_bitwise_arithmetic();
}
