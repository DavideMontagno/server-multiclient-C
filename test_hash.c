#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/un.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <pthread.h>
#include <config.h>
#include <message.h>
#include <connections.h>
#include <queue.h>
#include <hash.h>



void rand_str(char *dest, size_t length) {
    char charset[] =
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}


int main(int argc, char const *argv[]) {

	int connessi=0;
	char * str;

	tableAllClients_t *Hash = create();

	insertClientHash(Hash, "davide");
	insertClientHash(Hash, "riccardo");

	message_t *mex = malloc(sizeof(message_t));
	mex->hdr.op = 1;
	strcpy(mex->hdr.sender, "davide");
	strcpy(mex->data.hdr.receiver, "riccardo");
	mex->data.hdr.len = 5;
	mex->data.buf = malloc(sizeof(char)*5);
	strcpy(mex->data.buf, "ciao");

	insertMessage(Hash, mex);


	

	
	char nick[6];
	for(int i=0; i<10; i++) {
		rand_str(nick, 5);
		insertClientHash(Hash, nick);
	}

	printAll(Hash);


	removeTable(Hash);
	free(mex->data.buf);
	free(mex);
	return 0;
}
