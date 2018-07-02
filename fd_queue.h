//* @autore Davide Montagno Bozzone 535910 - Corso A
#include <pthread.h>

typedef struct fd_node {
	pthread_mutex_t fd_lock;
	char *nick;
	int online;
	int fd;
} fd_node_t;

typedef struct fds {
	fd_node_t *array;
	int NumOnline;
	int dim;
	int name_length;
} fds_t;

/**
@function initFdArray
@brief Alloca e inizializza una struttura contenente il numero di utenti online, i loro fd e nick
@return Puntatore alla struttura creata
*/

fds_t *initFdArray(); 


/**
@function destroyFd
@brief Dealloca la struttura passata come argomento
@param Puntatore alla struttura da deallocare
*/
void destroyFd(fds_t *fds);


/**
@function setOnline
@brief Setta la variabile online relativa al nick passato come argomento
@param fds, struttura dati su cui agire
@param nick, nickname da settare online
@param fd, file descriptor relativo alla connessione
*/
void setOnline(fds_t *fds, char nick[], int fd);


/**
@function setOffline
@brief Resetta la variabile online relativa al nick passato come argomento
@param fds, struttura dati su cui agire
@param fd, file descriptor relativo alla connessione
*/
void setOffline(fds_t *fds, int fd);

//Ritorna -1 se il client non è online, l'fd altrimenti

/**
@function isOnline
@brief Controlla se un utente è online oppure no
@param fds, struttura dati su cui agire
@param nick, nome dell'utente da controllare
@return -1 se non è online, l'fd della connessione altrimenti
*/
int isOnline(fds_t *fds, char nick[]);


/**
@function lengthFd
@brief Conta il numero di utenti online
@param fds, struttura dati su cui agire
@return numero di utenti online
*/
int lengthFd(fds_t *fds);


/**
@function printConnected
@brief Stampa una stringa contenente tutti i nickname degli utenti online
@param fds, struttura dati su cui agire
@return stringa contenente tutti gli utenti online
*/
char *printConnected(fds_t *fds);

//Funzione ausiliaria per il debug visuale
void printArray(fds_t *fds);
