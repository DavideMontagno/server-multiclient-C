//* @autore Davide Montagno Bozzone 535910 - Corso A



#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <fd_queue.h>
#include <string.h>
#include <param.h>


fds_t *initFdArray(int MAXCONNECTIONS, int MAX_NAME_LENGTH) {
	fds_t *fds = malloc(sizeof(fds_t));
	if(!fds) {
		perror("malloc");
		return NULL;
	}

	fds->NumOnline = 0;
	fds->array = malloc(sizeof(fd_node_t)*MAXCONNECTIONS);

	if (!fds->array) {
		perror("malloc");
		return NULL;
	}
	for (int i = 0; i < MAXCONNECTIONS; i++) {
		pthread_mutex_init(&fds->array[i].fd_lock, NULL);
		fds->array[i].online = 0;
		fds->array[i].fd = -1;
		fds->array[i].nick = malloc((MAX_NAME_LENGTH+1)*sizeof(char));
		if (!fds->array[i].nick) {
			perror("malloc");
			return NULL;
		}
		memset(fds->array[i].nick, 0, (MAX_NAME_LENGTH+1)*sizeof(char));
		fds->dim = MAXCONNECTIONS;
		fds->name_length = MAX_NAME_LENGTH;
	}
	return fds;
}

void printArray(fds_t *fds) {
	int i=fds->dim;
	fds->dim--;
	while(fds->dim>=0){
		if(fds->array[fds->dim].online ==1){
			printf("%d %s %d\n", i, fds->array[fds->dim].nick, fds->array[fds->dim].fd);		
		}
		fds->dim--;
	}
	fds->dim=i;
	return;
	
}

void destroyFd(fds_t *fds) {
	fds->dim--;
	while(fds->dim>=0){
		free(fds->array[fds->dim].nick);
		fds->dim--;
	}
	free(fds->array);
	free(fds);
}

void setOnline(fds_t *fds, char nick[], int fd) {
	
	if(fds == NULL || fd < 1)
		return;
	
	int added = isOnline(fds, nick);
	if(added == -2) return; //fds non riconosciuto 
	if (added == -1) {
		for (int i = 0; i < fds->dim && added != 1; i++) {
			pthread_mutex_lock(&(fds->array[i].fd_lock));
			if(fds->array[i].online == 0) {
				strcpy(fds->array[i].nick, nick);
				fds->array[i].online = 1;
				fds->NumOnline++;
				fds->array[i].fd = fd;
				added = 1;
				
			}
			pthread_mutex_unlock(&(fds->array[i].fd_lock));
		}
	}
	
	//else non sensato poichè è già online!
	return;
}


int isOnline(fds_t *fds, char nick[]) {
	if(fds == NULL) {
		return -2;	
	}
	for (int i = 0; i < fds->dim; i++) {
		pthread_mutex_lock(&(fds->array[i].fd_lock));
		if( strcmp(fds->array[i].nick, nick) == 0 ) {
			if(fds->array[i].online == 1) {
				pthread_mutex_unlock(&(fds->array[i].fd_lock));
				return i;
			}
		}
		pthread_mutex_unlock(&(fds->array[i].fd_lock));
	}
	return -1;
}

void setOffline(fds_t *fds, int fd) {
	
	if(fds == NULL || fd < 1)
		return;
	
	for (int i = 0; i < fds->dim; i++) {
		pthread_mutex_lock(&(fds->array[i].fd_lock));
		if(fds->array[i].fd == fd) {
			fds->NumOnline--;
			fds->array[i].online = 0;
			fds->array[i].fd = -1;
			memset(fds->array[i].nick, 0, fds->name_length+1);
			pthread_mutex_unlock(&(fds->array[i].fd_lock));
			return;
		}
		pthread_mutex_unlock(&(fds->array[i].fd_lock));
	}
	return;
}





char *printConnected(fds_t *fds) {

	if(fds == NULL)
		return NULL;

	char *str = calloc(fds->NumOnline*(fds->name_length+1), sizeof(char));
	if (!str) {
		perror("calloc");
		return NULL;
	}
	
	char *s = str;
	for (int i = 0; i < fds->dim; i++) {
		pthread_mutex_lock(&(fds->array[i].fd_lock));
		if(fds->array[i].online == 1) {
			strcpy(s, fds->array[i].nick);
			s += (fds->name_length + 1);
		}
		pthread_mutex_unlock(&(fds->array[i].fd_lock));
	}
	return str;
}

int lengthFd(fds_t *fds) {
	if(fds == NULL)
		return 0;
	return fds->NumOnline;
}
