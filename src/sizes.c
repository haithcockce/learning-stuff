#include <stdio.h>
#include <limits.h>

void main() {

    printf("\n    SIZE OF:\n --------------\n");
    printf(" char:        %d B\n", sizeof(char));
    printf(" short:       %d B\n", sizeof(short));
    printf(" int:         %d B\n", sizeof(int));
    printf(" long:        %d B\n", sizeof(long));
    printf(" long long:   %d B\n", sizeof(long long));
    printf(" float:       %d B\n", sizeof(float));
    printf(" double:      %d B\n", sizeof(double));
    printf(" long double: %d B\n", sizeof(long double));
	printf(" void *     : %d B\n", sizeof(void*));


    printf("\n MIN AND MAX VALUES \n");
    printf(" char:                %d to %d\n", CHAR_MIN, CHAR_MAX);
    printf(" unsigned char:       %d to %d\n", 0, UCHAR_MAX);
    printf(" short:               %d to %d\n", SHRT_MIN, SHRT_MAX);
    printf(" unsigned short:      %d to %d\n", 0, USHRT_MAX);
    printf(" int:                 %d to %d\n", INT_MIN, INT_MAX);
    printf(" unsigned int:        %d to %lld\n", 0, UINT_MAX);
    printf(" long:                %lld to %lld\n", LONG_MIN, LONG_MAX);
    printf(" unsigned long:       %llu to %llu\n", 0, ULONG_MAX);

}
