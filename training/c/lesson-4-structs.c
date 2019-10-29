#include <stdio.h>


struct address {
	unsigned int house_number;
	char* street_name;
	char* unit;
	char* city; 
	char* state; 
	unsigned int zip;
};

struct student {
	char* name; 
	long id;
	struct address where_on_campus;
};

struct address fill_in_my_address();
struct student fill_in_student(struct address);

struct address fill_in_my_address() {
	struct address to_fill;
	to_fill.house_number = 699;
	to_fill.street_name = "W 29th Ave";
	to_fill.unit = "3353";
	to_fill.city = "Denver";
	to_fill.state = "CO";
	to_fill.zip = 80202;
	return to_fill;
}

struct student fill_in_student(struct address his_addr) {
	struct student to_fill;
	to_fill.name = "charles";
	to_fill.id = 123456;
	to_fill.where_on_campus = his_addr;
	return to_fill;
}

void main() {
	struct address the_metro;
	the_metro = fill_in_my_address();
	struct student charles;
	charles = fill_in_student(the_metro);
	printf("What's my name? It's %s!\n", charles.name);
}
