#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <groups.h>


int main(int argc, char const *argv[]) {
	
	group_queue_t* list = start_queue_group(32);

	create_group(list, "davide", "cazzeggio");
	create_group(list, "riccardo", "cazzeggio2");
	create_group(list, "simone", "cazzeggio3");

	print_groups(list);

	add_member(list, "cazzeggio2",  "giacomo");
	add_member(list, "cazzeggio",  "davide");
	add_member(list, "cazzeggio",  "simone");

	print_members(list, "cazzeggio");
	remove_member(list, "cazzeggio", "davide");
	print_members(list, "cazzeggio");
	
	remove_queue_group(list);

	printf("\n*****************\n");
	printf("*  TEST PASSED  *\n");
	printf("*****************\n");

	return 0;
}
