//* @autore Davide Montagno Bozzone 535910 - Corso A
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <queue.h>
#include <message.h>

static pthread_mutex_t group_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct members_group {
	char *nick;
	struct members_group* next;
} members_group_t;

typedef struct group {
	char *name;
	char *owner;
	struct members_group* member_head;
	struct members_group* member_tail;
	int nMembers;
	struct group* next;
} group_t;

typedef struct group_queue {
	struct group* group_head;
	struct group* group_tail;
	int Ngroups;
	int MaxNameLength;
} group_queue_t;

/**
@function start_queue_group
@brief Alloca una lista di gruppi
@param MaxNameLenght, stabilisce la lunghezza massima per i nomi dei gruppi
@return Puntatore alla struttura allocata
*/
group_queue_t* start_queue_group(int MaxNameLength);


/**
@function create_group
@brief Aggiunge un gruppo alla coda
@param group_list, lista alla cui aggiungere il gruppo
@param owner, proprietario del gruppo
@param group_name, nome del gruppo
*/
void create_group(group_queue_t* group_list, char owner[], char group_name[]);


/**
@function add_member
@brief Aggiunge un membro ad un gruppo
@param group_list, lista alla cui aggiungere il membro
@param group_name, nome del gruppo
@param member, nickname del membro da aggiungere
*/
void add_member(group_queue_t* group_list, char group_name[], char member[]);


/**
@function remove_member
@brief Rimuove un membro ad un gruppo
@param group_list, lista alla cui aggiungere il membro
@param group_name, nome del gruppo
@param member, nickname del membro da rimuovere
*/
void remove_member(group_queue_t* group_list, char group_name[], char member[]);


/**
@function remove_group
@brief Rimuove un gruppo dalla coda
@param group_list, lista dalla cui rimuovere il gruppo
@param owner, proprietario del gruppo
@param group_name, nome del gruppo
*/
void remove_group(group_queue_t* group_list, char group_name[]);


/**
@function remove_queue_group
@brief Dealloca tutta la struttura allocata con start_queue_group
@param group_list, lista da deallocare
*/
void remove_queue_group(group_queue_t* group_list);

/**
@function isAGroup
@brief Controlla se un gruppo esiste già oppure no
@param group_list, lista da controllare
@param group_name, nome del gruppo da cercare
@return Puntatore all'elemento se trovato, NULL altrimenti
*/
group_t* isAGroup(group_queue_t* group_list, char group_name[]);


//Funzioni ausiliarie per il debug visuale
void print_groups(group_queue_t* group_list);

/**
@function already_member
@brief Controlla se un membro appartiene già ad un gruppo oppure no
@param group_list, lista da controllare
@param group_name, nome del gruppo da cercare
@param member, nickname del membro da cercare
@return 0 se il membro appartiene al gruppo, -1 altrimenti

*/
int already_member(group_queue_t* group_list, char group_name[], char member[]);

void print_members(group_queue_t* group_list, char group_name[]);
