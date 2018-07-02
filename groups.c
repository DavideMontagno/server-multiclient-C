//* @autore Davide Montagno Bozzone 535910 - Corso A
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <message.h>
#include <groups.h>


group_queue_t* start_queue_group(int MaxNameLength) {
	if(MaxNameLength<1) return NULL;
	group_queue_t* new = malloc(sizeof(group_queue_t));
	if(new == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);		
	}
	new->group_head = NULL;
	new->group_tail = NULL;
	new->MaxNameLength = MaxNameLength;
	new->Ngroups = 0;
	return new;
}

group_t* isAGroup(group_queue_t* group_list, char group_name[]) {
	group_t* ptr = group_list->group_head;
	if(ptr == NULL) return NULL;
	while(ptr != NULL) {
		if(strcmp(ptr->name, group_name) == 0)
			return ptr;
		ptr = ptr->next;
	}
	return NULL;
}

void create_group(group_queue_t* group_list, char creator[], char group_name[]) {
	if(group_list==NULL) return;
	group_t* new = malloc(sizeof(group_t));
	if(new == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->name = malloc(strlen(group_name)+1);
	if(new->name == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->owner = malloc(group_list->MaxNameLength+1);
	if(new->owner == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->member_head = NULL;
	new->member_tail = NULL;
	new->nMembers = 1;
	strncpy(new->owner, creator, group_list->MaxNameLength);
	if(strcmp(new->owner, creator) != 0){
		perror("proprietari diversi diverso");
	}
	
	strncpy(new->name, group_name, strlen(group_name));
	if(strcmp(new->name, group_name) != 0){
		perror("nomi diversi");
	}
	members_group_t* creat = malloc(sizeof(members_group_t));
	if(creat == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	creat->nick = malloc(group_list->MaxNameLength);
	if(creat->nick == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	strncpy(creat->nick, creator, strlen(creator)+1);
	if(strcmp(new->name, group_name) != 0){
		perror("nick diversi");
	}
	creat->next = NULL;
	new->member_head = creat;
	new->member_tail = creat;
	//PRIMO GRUPPO INSERITO
	if(group_list->group_head == NULL) {
		pthread_mutex_lock(&group_lock);
		group_list->group_head = new;
		group_list->group_tail = new;
		group_list->Ngroups++;
		new->next = NULL;
		pthread_mutex_unlock(&group_lock);
	}
	//INSERISCO PER SEMPLICITÀ SEMPRE IN TESTA.
	else {
		pthread_mutex_lock(&group_lock);
		new->next = group_list->group_head;
		group_list->group_head = new;
		pthread_mutex_unlock(&group_lock);
	}
	return;
}

void add_member(group_queue_t* group_list, char group_name[], char member[]) {
	members_group_t* new = malloc(sizeof(members_group_t));
	if(new == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	new->nick = malloc(group_list->MaxNameLength+1);
	if(new->nick == NULL){
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	strncpy(new->nick, member, group_list->MaxNameLength);
	if(strcmp(new->nick, group_name) != 0){
		perror("nick diversi");
	}
	group_t* ptr = group_list->group_head;
	if(ptr == NULL) { 
		
		return; 
	}
	while(ptr != NULL) {
		pthread_mutex_lock(&group_lock);
		if(strcmp(group_name, ptr->name) == 0) {
			if(ptr->member_head == NULL) {
				new->next = NULL;
				ptr->member_head = new;
				ptr->member_tail = new;
			}
			else {
				new->next = ptr->member_head;
				ptr->member_head = new;
			}
		}
		ptr = ptr->next;
		pthread_mutex_unlock(&group_lock);
	}
	return;
}

int already_member(group_queue_t* group_list, char group_name[], char member[]) {
	group_t* ptr = group_list->group_head;
	if(ptr == NULL) return -1;
	while(ptr != NULL) {
		if(strcmp(group_name, ptr->name) == 0) {
			members_group_t* mpt = ptr->member_head;
			while(mpt != NULL) {
				if(strcmp(member, mpt->nick) == 0)
					return 0;
				mpt = mpt->next;
			}
		}
		ptr = ptr->next;
	}
	return -1;
}

void remove_member(group_queue_t* group_list, char group_name[], char member[]) {
	group_t* ptr = group_list->group_head;
	if(ptr == NULL) return;
	while(ptr != NULL) {
		pthread_mutex_lock(&group_lock);
		if(strcmp(group_name, ptr->name) == 0) {
			members_group_t* mpt = ptr->member_head;
			members_group_t* aux = ptr->member_head;
			while(mpt != NULL) {
				if(strcmp(mpt->nick, member) == 0) {
					if(mpt == ptr->member_head) {
						ptr->member_head = ptr->member_head->next;
						free(mpt->nick);
						free(mpt);
						ptr->nMembers--;
					}
					else if (mpt == ptr->member_tail) {
						ptr->member_tail = aux;
						aux->next = NULL;
						free(mpt->nick);
						free(mpt);
						ptr->nMembers--;
					}
					else {
						aux->next = mpt->next;
						free(mpt->nick);
						free(mpt);
						ptr->nMembers--;
					}
				}
				aux = mpt;
				mpt = mpt->next;
			}
		}
		ptr = ptr->next;
		pthread_mutex_unlock(&group_lock);
	}
	return;
}

void remove_group(group_queue_t* group_list, char group_name[]) {
	group_t* ptr = group_list->group_head;
	group_t* aux = group_list->group_head;
	if(ptr == NULL) return;
	while(ptr != NULL) {
		pthread_mutex_lock(&group_lock);
		if(strcmp(ptr->name, group_name) == 0) {
			if(ptr == group_list->group_head) {
				group_list->group_head = group_list->group_head->next;
				group_list->Ngroups--;
			}
			else if (ptr == group_list->group_tail) {
				aux->next = NULL;
				group_list->group_tail = aux;
				group_list->Ngroups--;
			}
			else {
				aux->next = ptr->next;
				group_list->Ngroups--;
			}
			members_group_t* mpt = ptr->member_head;
			while(mpt != NULL) {
				ptr->member_head = ptr->member_head->next;
				free(mpt->nick);
				free(mpt);
				mpt = ptr->member_head;
			}
			free(ptr->name);
			free(ptr->owner);
			free(ptr);
		}
		aux = ptr;
		ptr = ptr->next;
		pthread_mutex_unlock(&group_lock);
	}
	return;
}

void remove_queue_group(group_queue_t* group_list) {
	if(group_list == NULL) return;
	pthread_mutex_lock(&group_lock);
	group_t* ptr = group_list->group_head;
	while(ptr != NULL) {
		group_t* aux = ptr;
		ptr = ptr->next;
		members_group_t* del = aux->member_head;
		while(del != NULL) {
			members_group_t *aux2 = del;
			del = del->next;
			free(aux2->nick);
			free (aux2);
		}
		free(aux->name);
		free(aux->owner);
		free(aux);
	}
	free(group_list);
	pthread_mutex_unlock(&group_lock);
}

void print_members(group_queue_t* group_list, char group_name[]) {
	group_t* ptr = group_list->group_head;
	if(ptr == NULL) return;
	while(ptr != NULL) {
		if(strcmp(ptr->name, group_name) == 0) {
			int i=1;
			members_group_t* mpt = ptr->member_head;
			while(mpt != NULL) {
				printf("%d) %s ",i, mpt->nick);
				mpt = mpt->next;
				i++;
			}
		}
		ptr = ptr->next;
	}
	printf("\n");
	return;
}

void print_groups(group_queue_t* group_list) {
	group_t* ptr = group_list->group_head;
	if(ptr== NULL) return;
	int i=1;
	while(ptr != NULL) {
		printf("%d) %s\n ", i, ptr->name);
		ptr = ptr->next;
		i++;
	}
	printf("\n");
	return;
}
