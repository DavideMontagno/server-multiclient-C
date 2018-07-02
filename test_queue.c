#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <message.h>
#include <queue.h>


int main(int argc, char const *argv[]) {
	
	int ret;
	list_members_t *q = start_new_List(15, 15);
	if(q == NULL)
		printf("start_new_List error\n");

	// --- TEST PUSHCLIENT --- //
	printf("\n --- Test insertClient ---\n");

	insertClient(q, "davide");
	insertClient(q, "riccardo");
	insertClient(q, "simone");

	printQueue(q);

	// --- TEST PUSHMESSAGE --- //
	printf("\n --- Test insertMessage ---\n");
	message_t *messaggio = malloc(sizeof(message_t));
	memset(messaggio, 0, sizeof(message_t));
	strcpy(messaggio->hdr.sender, "davide");
	strcpy(messaggio->data.hdr.receiver, "simone");
	messaggio->data.hdr.len = 5;
	messaggio->data.buf = malloc(sizeof(char)*10); 
	memset(messaggio->data.buf, 0, sizeof(char)*10);
	strcpy(messaggio->data.buf, "ciao");

	message_t *messaggio2 = malloc(sizeof(message_t));
	memset(messaggio2, 0, sizeof(message_t));
	strcpy(messaggio2->hdr.sender, "riccardo");
	strcpy(messaggio2->data.hdr.receiver, "simone");
	messaggio2->data.hdr.len = 5;
	messaggio2->data.buf = malloc(sizeof(char)*10); 
	memset(messaggio2->data.buf, 0, sizeof(char)*10);
	strcpy(messaggio2->data.buf, "ciaone");

	message_t *messaggio3 = malloc(sizeof(message_t));
	memset(messaggio3, 0, sizeof(message_t));
	strcpy(messaggio3->hdr.sender, "riccardo");
	strcpy(messaggio3->data.hdr.receiver, "davide");
	messaggio3->data.hdr.len = 5;
	messaggio3->data.buf = malloc(sizeof(char)*10); 
	memset(messaggio3->data.buf, 0, sizeof(char)*10);
	strcpy(messaggio3->data.buf, "ciao");

	// --- TEST CHECKREGISTERED --- //
	printf("\n --- Test alreadyRegistered ---\n");
	if( alreadyRegistered(q, "nome_non_registrato") != 0)
		printf("nome_non_registrato non è registrato\n");

	// --- TEST LENGTH --- //
	printf("\n --- Test Length ---\n");
	printf("Client number: %lu\n", length(q));

	// --- TEST PUSHTOALL --- //
	printf("\n --- Test insertMessageBroadcast ---\n");

	for(int i=0; i<100; i++) {
		insertMessageBroadcast(q, messaggio);
	}

	for(int i=0; i<1000; i++) {
		insertMessage(q, messaggio);
	}

	for(int i=0; i<100; i++) {
		insertMessageBroadcast(q, messaggio3);
	}

	for(int i=0; i<100; i++) {
		insertMessageBroadcast(q, messaggio2);
	}
	
	printQueue(q);

	
	
	remove_all_list(q);
	free(messaggio->data.buf);
	free(messaggio);
	
	free(messaggio2->data.buf);
	free(messaggio2);

	free(messaggio3->data.buf);
	free(messaggio3);

	printf(" --- TEST PASSED ---\n");

	return 0;
}
