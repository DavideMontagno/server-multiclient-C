//* @autore Davide Montagno Bozzone 535910 - Corso A

#ifndef QUEUE_H_
#define QUEUE_H_

#include <message.h>
#include <stats.h>


typedef struct SingleClient {
    char *name;
    struct SingleClient *next;
    struct SingleMessage *history_head;
    struct SingleMessage *history_tail;
    unsigned long history_length;
} SingleClient_t;

typedef struct SingleMessage {
	message_t *msg;
	struct SingleMessage *next;
} SingleMessage_t;

typedef struct list_members {
    SingleClient_t *queue_head;
    SingleClient_t *queue_tail;
    unsigned long  queue_length;
    int Num_Online;
    int MaxHistMsg;
    int MaxNameLength;
} list_members_t;


/**
@function start_new_List
@brief Alloca una coda annidata di utenti con i relativi messaggi
@param MaxHistMsg, massima lunghezza della history per ogni utente
@param MaxNameLength, massima lunghezza possibile per i nicknames
@return Puntatore alla struttura allocata
*/
list_members_t *start_new_List(int MaxHistMsg, int MaxNameLength);

/**
@function remove_all_list
@brief Dealloca una coda annidata creata con start_new_List
@param MaxHistMsg, massima lunghezza della history per ogni utente
@param MaxNameLength, massima lunghezza possibile per i nicknames
*/
void remove_all_list(list_members_t *q);


/**
@function insertClient
@brief Registra un nuovo utente, inserendolo nella coda
@param q, coda nella quale inserire il nuovo elemento
@param nick, nickname del nuovo utente
*/
void insertClient(list_members_t *q, char nick[]);

/**
@function insertMessage
@brief Inserisce un messaggio nella history di un utente
@param q, coda nella quale inserire il nuovo elemento
@param msg, messaggio da inserire nella coda
@return 0 in caso di successo, -1 altrimenti
*/
int insertMessage(list_members_t *q, message_t *msg);


/**
@function insertMessageBroadcast
@brief Inserisce un messaggio nella history di tutti gli utenti registrati
@param q, coda nella quale inserire i messaggi
@param msg, messaggio da inserire nella coda
*/
void insertMessageBroadcast(list_members_t *q, message_t *messaggio);


/**
@function removeClient
@brief Rimuove un utente dalla coda
@param q, coda nella quale inserire il nuovo elemento
@param nick, nickname dell'utente da deregistrare
*/
void removeClient(list_members_t *q, char nick[]);


/**
@function alreadyRegistered
@brief Controlla se un nickname è già registato oppure no
@param q, coda dalla quale controllare la presenza
@param nick, nickname da controllare
@return 0 se è già registrato, -1 altrimenti
*/
int alreadyRegistered(list_members_t *q, char nick[]);


/**
@function length
@brief Calcola il numero di utenti registrati
@param q, coda nella quale contare gli utenti
@return Numero di utenti registrati
*/
unsigned long length(list_members_t *q);


/**
@function getNodefromNick
@brief Restituisce il puntatore al nodo della coda relativo all'utente di nome nick
@param q, coda nella quale cercare il nickname
@param nick, nickaname da ricercare
@return Puntatore al nodo se trovato, NULL altrimenti
*/

SingleClient_t *getNodefromNick(list_members_t *q, char nick[]);

//Funzione ausiliaria per il debug visuale
void printQueue(list_members_t *q);

#endif /* QUEUE_H_ */
