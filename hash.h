//* @autore Davide Montagno B. 535910 - Corso A

#ifndef hash_h
#define hash_h
#include <stdio.h>
#endif

#include <pthread.h>
#include <queue.h>
#include <stats.h>

#define HASH_DIM 20


typedef struct tableAllClients {
	list_members_t *q;
	pthread_mutex_t lock;
	pthread_cond_t cond;
} tableAllClients_t;

/*
@function create
@brief Alloca una nuova tabella tableAllClients e restituisce il puntatore ad essa
@param MaxHistMsgs, numero massimo di messaggi della history, dopo il quale il più vecchio viene eliminato
@param MaxNameLength, massima lunghezza possibile per i nicknames
@return il puntatore alla tabella allocata, NULL in caso di errore
*/
tableAllClients_t *create(int MaxHistMsgs, int MaxNameLength);


/*
@function removeTable
@brief Dealloca l'intera tabella tableAllClients
@param tableAllClients, puntatore alla tabella tableAllClients da eliminare
*/
void removeTable(tableAllClients_t *tableAllClients);


/**
@function hashFunction
@brief Applica la funzione tableAllClients alla key e restituisce la chiave risultante
@param key, elemento su cui applicare la funzione tableAllClients
@return risultato della funzione tableAllClients
*/
unsigned int hashFunction(void* key);


/**
@function insertClient
@brief Inserisce un nuovo utente nella tabella tableAllClients
@param tableAllClients, tableAllClients nella quale inserire l'elemento
@param nick, nickname del nuovo utente
*/
void insertClientHash(tableAllClients_t *tableAllClients, char nick[]);

/**
@function removeClient
@brief Rimuove un utente dalla tabella tableAllClients e libera la memoria occupata
@param tableAllClients, tableAllClients dalla quale rimuovere l'elemento	
@param nick, nickname dell'utente da rimuovere
*/
void removeClientHash(tableAllClients_t *tableAllClients, char nick[]);

/**
@function insertMessage
@brief Inserisce un messaggio nella history dell'utente specificato dal receiver del messaggio
@param tableAllClients, tableAllClients nella quale inserire l'elemento
@param msg, messaggio da inserire
*/
void insertMessageHash(tableAllClients_t *tableAllClients, message_t *msg);


/**
@function insertMessageBroadcast
@brief Inserisce un messaggio nella history di tutti gli utenti registrati
@param tableAllClients, tableAllClients nella quale inserire l'elemento
@param msg, messaggio da inserire
*/
void insertMessageBroadcastHash(tableAllClients_t *tableAllClients, message_t *msg);


/**
@function checkRegistered
@brief Controlla se un utente è già registrato
@param tableAllClients, tableAllClients nella quale inserire l'elemento
@param nick, nickname da verificare
@return 0 o -1 se il nick è presente oppure no
*/
int checkRegistered(tableAllClients_t *tableAllClients, char nick[]);


/**
@function getClientFromNick
@brief Restituisce il puntatore all'utente di nickname nick
@param tableAllClients, tableAllClients nella quale cercare l'utente
@param nick, nickname da cercare
@return puntatore all'utente se registrato, NULL altrimenti
*/
SingleClient_t *getClientFromNick(tableAllClients_t *tableAllClients, char nick[]);


//Funzione ausiliaria per il debug visuale
void printAll(tableAllClients_t *tableAllClients);
