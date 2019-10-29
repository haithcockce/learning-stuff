#include <stdio.h>

void main() {
	char a_letter = 'a';
	int a_number = 10;
	char* a_string = "Here is some text";
	long a_big_number = 5000000000;

	/* Array
	 *
	 * For many languages, you can have a list of items where the name of the
	 * list is something_like_this[] and you can access specific items in the
	 * list by passing in the index of that item. 
	 *
	 * HOWEVER, lists like this are 0-based, this just means the first item in
	 * the list is at index 0. For example, to get the first element of the list
	 * 'an_array', you do an_array[0]
	 */
    int an_array[5] = {1, 2, 3, 4, 5};

	printf("The last item in an_array is %d\n", an_array[4]);
	printf("Wait, we can do the same with that string? The first item is %c\n", 
			a_string[0]);  // more on this later...
}
