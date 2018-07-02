//* @autore Davide Montagno Bozzone 535910 - Corso A

#include <queue.h>
#include <pthread.h>
#include <stdlib.h>
#include <queue.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <limits.h>
#include <hash.h>



unsigned int hashFunction(void* key) {
    char *datum = (char *)key;
    unsigned int hash_value, i;

    if(!datum) return 0;

    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ((int) (( sizeof(int) * CHAR_BIT ) / 8))) + *datum;
        if ((i = hash_value & ( ~((unsigned int)(~0) >>((int) (( sizeof(int) * CHAR_BIT ) / 8)) ))) != 0)
            hash_value = (hash_value ^ (i >>  ((int) ((( sizeof(int) * CHAR_BIT ) * 3) / 4)))) & ~( ~((unsigned int)(~0) >>((int) (( sizeof(int) * CHAR_BIT ) / 8)) ));
    }
    return (hash_value)%HASH_DIM;
}


tableAllClients_t *create(int MaxHistMsgs, int MaxNameLength) {
	tableAllClients_t *tableAllClients = malloc(sizeof(tableAllClients_t)*HASH_DIM);
	if(!tableAllClients) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
	for(int i=0; i<HASH_DIM; i++) {
		tableAllClients[i].q = start_new_List(MaxHistMsgs, MaxNameLength);
		pthread_mutex_init(&tableAllClients[i].lock, NULL);
		pthread_cond_init(&tableAllClients[i].cond, NULL);
	}
	return tableAllClients;
}

void removeTable(tableAllClients_t *tableAllClients) {
	for(int i=0; i<HASH_DIM; i++) {
		pthread_mutex_lock(&tableAllClients[i].lock);
		remove_all_list(tableAllClients[i].q);
		pthread_mutex_unlock(&tableAllClients[i].lock);
	}
	free(tableAllClients);
}

void insertClientHash(tableAllClients_t *tableAllClients, char nick[]) {
	int key = hashFunction(nick);
	if(key==0) return;
	pthread_mutex_lock(&tableAllClients[key].lock);
	insertClient(tableAllClients[key].q,nick);
	pthread_mutex_unlock(&tableAllClients[key].lock);
	return;
}

void removeClientHash(tableAllClients_t *tableAllClients, char nick[]) {
	int key = hashFunction(nick);
	pthread_mutex_lock(&tableAllClients[key].lock);
	removeClient(tableAllClients[key].q, nick);
	pthread_mutex_unlock(&tableAllClients[key].lock);
	return;
}

void insertMessageHash(tableAllClients_t *tableAllClients, message_t *msg) {
	int key = hashFunction(msg->data.hdr.receiver);
	if(key==0) return;
	pthread_mutex_lock(&tableAllClients[key].lock);
	insertMessage(tableAllClients[key].q, msg);
	pthread_mutex_unlock(&tableAllClients[key].lock);
	return;
}

void insertMessageBroadcastHash(tableAllClients_t *tableAllClients, message_t *msg) {
	for(int i=0; i<HASH_DIM; i++) {
		pthread_mutex_lock(&tableAllClients[i].lock);
		insertMessageBroadcast(tableAllClients[i].q, msg);
		pthread_mutex_unlock(&tableAllClients[i].lock);
	}
	return;
}

int checkRegistered(tableAllClients_t *tableAllClients, char nick[]) {
	int key = hashFunction(nick);
	if(key==0) return -2;
	pthread_mutex_lock(&tableAllClients[key].lock);
	int ret = alreadyRegistered(tableAllClients[key].q, nick);
	pthread_mutex_unlock(&tableAllClients[key].lock);
	if(ret == -1) return -1;
	else return 0;
}

void printAll(tableAllClients_t *tableAllClients) {
	for(int i=0; i<HASH_DIM; i++) {
		pthread_mutex_lock(&tableAllClients[i].lock);
		printf("%d: \n", i);
		printQueue(tableAllClients[i].q);
		pthread_mutex_unlock(&tableAllClients[i].lock);
		
	}
	return;
}


SingleClient_t *getClientFromNick(tableAllClients_t *tableAllClients, char nick[]) {
	int key = hashFunction(nick);
	if(key==0) return -1;
	pthread_mutex_lock(&tableAllClients[key].lock);
	SingleClient_t *head = getNodefromNick(tableAllClients[key].q, nick);
	pthread_mutex_unlock(&tableAllClients[key].lock);
	return head;
}
