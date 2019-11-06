#include <stdio.h>

void main() {
    printf("5/2 = %d, 5%2 = %d\n", (5/2), (5%2));
    int x = 0;
    printf("x = 0\n");
    printf("!x => %d, x == 0 => %d, x != 0 => %d, x > 0 => %d, x <= 0 => %d\n", 
            (!x), (x==0), (x!=0), (x>0), (x<=0));
    printf("1 < 2 < 3 => %d\n", (1<2<3));
    printf("x++ => %d, ", x++);
    x = 0;
    printf("++x => %d\n", ++x);
}
