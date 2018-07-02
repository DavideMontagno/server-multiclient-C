#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fd_queue.h>
#include <assert.h>


int main(int argc, char const *argv[]) {

	// --- INIT --- //
	fds_t * fdl = initFdArray(10, 10);

	// --- SETONLINE --- //
	setOnline(fdl, "davide", 4);
	setOnline(fdl, "riccardo", 6);
	setOnline(fdl, "simone", 3);


	char * str = printConnected(fdl);
	printf("%s\n", str);
	free(str);

	// --- ISONLINE --- //
	int check = isOnline(fdl, "simone");
	assert(check != -1);

	setOnline(fdl, "davide", 4);
	setOnline(fdl, "riccardo", 6);
	setOnline(fdl, "simone", 3);


	// --- SETOFFLINE --- //
	setOffline(fdl, 4); //davide

	// --- DESTROY --- //
	destroyFd(fdl);

	return 0;
}
