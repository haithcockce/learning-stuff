#include <stdio.h>

/* Function Prototypes 
 *
 * Defines how you interact with the function, IE what parameters, if any, can
 * you give it, and what is expected to be returned from it if anything. This
 * can be thought of as an interface. 
 *
 * Breakdown of the following example function prototypes: 
 *
 * (1) void: means the function returns nothing when it finishes
 * (2) my_print_function: the name of the function 
 * (3) (void): means the function does not take any parameters
 * */ 
void my_print_function(void);

/* (1) int: this function returns a number
 * (2) what_number: the name of the function
 * (3) (int): this function takes a number as a parameter
 */
int what_number(int);



void my_print_function() {
	printf("I want to print something specific!\n");
}

/* Like the prototype indicated, we take a number as input and that number is
 * 'i' for the function.
 *
 * Like an expresson, what_number returns a value, specifically the parameter,
 * 'i', multiplied by 4. 
 */
int what_number(int i) {
	return (i * 4);
}

void main() {
	my_print_function();
	printf("Return values can be passed as parameters!\n");
	printf("%d\n", what_number(10));
}
