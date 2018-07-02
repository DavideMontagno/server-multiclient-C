/*
 * membox Progetto del corso di LSO 2017
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */

/**
*
*
* @autore Davide Montagno Bozzone 535910 - Corso A
*/



#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <queue.h>
#include <groups.h>
#include <ops.h>
#include <param.h>
#include <config.h>
#include <message.h>
#include <connections.h>
#include <hash.h>
#include <fd_queue.h>
#include <stats.h>




//Configurazione server
char UnixPath[100];
char DirName[100];
char StatFileName[100];
int MaxFileSize;
int MaxHistMsgs;
int MaxNameLength;
int MaxConnections;
int ThreadsInPool;
int MaxMsgSize;



#define SOCKNAME "./socket"
#define SA_INIT(SA) { strncpy(SA.sun_path, SOCKNAME, UNIX_PATH_MAX); SA.sun_family=AF_UNIX; }

#ifndef VERIFY
#define VERIFY(a,b,c)  \
  if((a=b) < 0) {perror(c);}
#endif


struct statistics chattyStats = { 0,0,0,0,0,0,0 };

static int *works;
static int works_index=0;

//Chiavi pthread
static pthread_mutex_t works_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  condit_worker = PTHREAD_COND_INITIALIZER;
static fd_set set;
volatile sig_atomic_t cicle = 1;

//strutture necessarie 
static tableAllClients_t* Hash; //struttura di tutti gli utenti
static fds_t* fd_list; //array di utenti online!
static group_queue_t* group_list; //struttura per i gruppi





static void *handler_segnali(void *val){
	
	int tempSign;
	sigset_t tempSigSet;
	FILE * tempFile;

	sigemptyset(&tempSigSet);
	sigaddset(&tempSigSet, SIGUSR1);
	sigaddset(&tempSigSet, SIGTERM);
	sigaddset(&tempSigSet, SIGINT);
	sigaddset(&tempSigSet, SIGQUIT);

	//Attesa
	while(cicle== 1){
		sigwait(&tempSigSet, &tempSign);

		//Controllo segnali
		switch (tempSign) {
			//Segnale per stampa statistiche
			case SIGUSR1:{
				//Controllo consistenza del file statiche
				if(StatFileName!="Errore"){
					if((tempFile=fopen(StatFileName, "w")) == NULL){
						perror("fileStatistiche");
						exit(EXIT_FAILURE);
					}

					if(printStats(tempFile) == -1){
						write(2,"errore in printStats\n",30);
						_exit(EXIT_FAILURE);
					}
					fclose(tempFile);
				}
				continue;
			}

			//Segnali di terminazione
			case SIGQUIT:
			case SIGTERM:
			case SIGINT: {
				cicle=0;
				pthread_cond_broadcast(&condit_worker);
				return (void*) NULL;
			}
		}
	}
	return (void*) NULL;
}

void gestioneSegnali(struct sigaction strSigAction, sigset_t tempSets, 	pthread_t tempThread, pthread_attr_t tempThreadAt){

		memset(&strSigAction, 0, sizeof(strSigAction));

		strSigAction.sa_handler = SIG_IGN;

		//Imposto la nuova gestione di SIGPIPE con strSigAction 
		sigaction(SIGPIPE, &strSigAction, NULL);
		
		//Azzero tempSets
		sigemptyset(&tempSets);

		//Metto a 1 il segnale in tempSets
		sigaddset(&tempSets, SIGUSR1);
		sigaddset(&tempSets, SIGQUIT);
		sigaddset(&tempSets, SIGTERM);
		sigaddset(&tempSets, SIGINT);

		//Maschero i segnali tempSets con SIG_SETMASK
		pthread_sigmask(SIG_SETMASK,&tempSets,NULL);

		//Inizializzo tempThreadAt con il valore predefinito
		pthread_attr_init(&tempThreadAt);
		//Controllo se il thread è stato creato in uno stato disconnesso
		pthread_attr_setdetachstate(&tempThreadAt, PTHREAD_CREATE_DETACHED);
		//Creo un nuovo thread con attributi specificati da tempThreadAt con esecuzione gestioneSegnaliStat
		pthread_create(&tempThread, &tempThreadAt, handler_segnali, NULL);
}










void *main_thread_function(void *arg) {
	int fd=0;
	fd_set *setP = (fd_set*) arg;
	message_t messaggio;
	while(1){
		pthread_mutex_lock(&works_lock);
		while(works_index == 0) {
			if(cicle == 0) {
				pthread_mutex_unlock(&works_lock);
				break;
			}
			pthread_cond_wait(&condit_worker, &works_lock);
		}

		if(cicle == 0) {
			pthread_mutex_unlock(&works_lock);
			break;
		}

		fd = works[works_index-1];
		works_index--;
		pthread_mutex_unlock(&works_lock);
		
		
		if(readMsg(fd, &messaggio) <= 0){
			setOffline(fd_list, fd);
			chattyStats.nonline--;
			close(fd);
		}
		else {
			
			switch(messaggio.hdr.op) {
				
				
				case REGISTER_OP: {
					
					int ret = checkRegistered(Hash, messaggio.hdr.sender);
					//utente già presente
					if (ret == 0) {
						
						setHeader(&messaggio.hdr, OP_NICK_ALREADY, "server");
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						chattyStats.nerrors++;
						break;
					}
					//controllo dim sender
					if( strlen(messaggio.hdr.sender) > MaxNameLength ) {
						
						setHeader(&messaggio.hdr, OP_FAIL, "server");
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						chattyStats.nerrors++;
						break;
					}
					setOnline(fd_list, messaggio.hdr.sender, fd);
					chattyStats.nonline++;
					int index = isOnline(fd_list, messaggio.hdr.sender);

					insertClientHash(Hash, messaggio.hdr.sender);
					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[index].fd_lock));

					char *str = printConnected(fd_list);
					setData(&(messaggio.data), "", str, (MAX_NAME_LENGTH+1)*(fd_list->NumOnline));
					
					pthread_mutex_lock(&(fd_list->array[index].fd_lock));
					VERIFY(ret, sendData(fd,&(messaggio.data)), "to senData");
					pthread_mutex_unlock(&(fd_list->array[index].fd_lock));

					chattyStats.nusers++;
					free(str);

				}break;

				
				case CONNECT_OP: {
				
					
					int ret = checkRegistered(Hash, messaggio.hdr.sender);
					if (ret != 0) {
						setHeader(&messaggio.hdr, OP_NICK_UNKNOWN, "server");
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						chattyStats.nerrors++;
						break;
					}
					setOnline(fd_list, messaggio.hdr.sender, fd);
					int index = isOnline(fd_list, messaggio.hdr.sender);

					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[index].fd_lock));
					char * str = printConnected(fd_list);
					setData(&(messaggio.data),"",str, (MAX_NAME_LENGTH+1)*(fd_list->NumOnline));
					pthread_mutex_lock(&(fd_list->array[index].fd_lock));
					VERIFY(ret, sendData(fd,&(messaggio.data)), "to senData");
					pthread_mutex_unlock(&(fd_list->array[index].fd_lock));
					chattyStats.nonline++;
					free(str);
				}break;

				
				case POSTTXT_OP: {
					int ret = checkRegistered(Hash, messaggio.data.hdr.receiver);
					int index_sender = isOnline(fd_list, messaggio.hdr.sender);
					int index_receiver = isOnline(fd_list, messaggio.data.hdr.receiver);

					if( messaggio.data.hdr.len > MaxMsgSize) {
						setHeader(&messaggio.hdr, OP_MSG_TOOLONG, "server");
						pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
						chattyStats.nerrors++;
						free(messaggio.data.buf);
						break;
					}

					messaggio.hdr.op = TXT_MESSAGE;

					if (ret != 0) {
						group_t* ptr = isAGroup(group_list, messaggio.data.hdr.receiver);
						ret = already_member(group_list, messaggio.data.hdr.receiver, messaggio.hdr.sender);
						if(ptr == NULL || ret == -1) {
							//DESTINATARIO INESISTENTE.
							setHeader(&messaggio.hdr, OP_NICK_UNKNOWN, "server");
							pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
							VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
							pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
							chattyStats.nerrors++;
							free(messaggio.data.buf);
							break;
						}
						else {
							members_group_t* mem = ptr->member_head;
							while(mem != NULL) {
								strncpy(messaggio.data.hdr.receiver, mem->nick, MAX_NAME_LENGTH+1);
								insertMessageHash(Hash, &messaggio);
								int mem_online = isOnline(fd_list, mem->nick);
								if (mem_online != -1) {
									pthread_mutex_lock(&(fd_list->array[mem_online].fd_lock));
									VERIFY(ret, sendHeader(fd_list->array[mem_online].fd, &messaggio.hdr), "to send Header");
									VERIFY(ret, sendData(fd_list->array[mem_online].fd, &messaggio.data), "to senData");
									pthread_mutex_unlock(&(fd_list->array[mem_online].fd_lock));
									chattyStats.ndelivered++;
								}
								mem = mem->next;
							}

							setHeader(&messaggio.hdr, OP_OK, "server");
							pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
							VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
							pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
						}
					}

					else {
						if( index_receiver != -1 && fd_list->array[index_receiver].online == 1 ) {
							pthread_mutex_lock(&(fd_list->array[index_receiver].fd_lock));
							VERIFY(ret, sendHeader(fd_list->array[index_receiver].fd, &messaggio.hdr), "to send Header");
							VERIFY(ret, sendData(fd_list->array[index_receiver].fd, &messaggio.data), "to senData");
							pthread_mutex_unlock(&(fd_list->array[index_receiver].fd_lock));
							chattyStats.ndelivered++;
						}
						else {
							insertMessageHash(Hash, &messaggio);
							chattyStats.nnotdelivered++;
						}

						setHeader(&messaggio.hdr, OP_OK, "server");
						pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
					}
					free(messaggio.data.buf);
				}break;

			
				case POSTTXTALL_OP: {
					int ret=0;
					int index = isOnline(fd_list, messaggio.hdr.sender);
					
					if(strlen(messaggio.data.buf) > MaxMsgSize) {
						setHeader(&messaggio.hdr, OP_MSG_TOOLONG, "server");
						pthread_mutex_lock(&(fd_list->array[index].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[index].fd_lock));
						chattyStats.nerrors++;
						free(messaggio.data.buf);
						break;
					}

					messaggio.hdr.op = TXT_MESSAGE;
					insertMessageBroadcastHash(Hash, &messaggio);
					chattyStats.nnotdelivered += chattyStats.nusers-1;

					message_t toall;
					memcpy(&toall, &messaggio, sizeof(message_t));
					toall.data.buf = strndup(messaggio.data.buf, messaggio.data.hdr.len);

					for (int i=0; i<MaxConnections; i++) {
						if(fd_list->array[i].online == 1 && fd_list->array[i].fd != -1) {
							if(strcmp(fd_list->array[i].nick, messaggio.hdr.sender) != 0) {
								strncpy(toall.data.hdr.receiver, fd_list->array[i].nick, MAX_NAME_LENGTH+1);
								pthread_mutex_lock(&(fd_list->array[i].fd_lock));
								VERIFY(ret, sendRequest(fd_list->array[i].fd, &toall), "sendRequest");
								pthread_mutex_unlock(&(fd_list->array[i].fd_lock));
								chattyStats.nnotdelivered--;
								chattyStats.ndelivered++;
							}
						}
					}

					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[index].fd_lock));

					free(toall.data.buf);
					free(messaggio.data.buf);
				}break;


				
				case POSTFILE_OP: {

					int ret = 0;
					int ret2 = checkRegistered(Hash, messaggio.data.hdr.receiver);
					group_t* ptr = isAGroup(group_list, messaggio.data.hdr.receiver);
					int index_sender = isOnline(fd_list, messaggio.hdr.sender);
					
					if (ret2 == -1 && ptr == NULL) {
						setHeader(&messaggio.hdr, OP_NICK_UNKNOWN, "server");
						pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
						chattyStats.nerrors++;
						free(messaggio.data.buf);
						break;
					}

					char *path = calloc(100, sizeof(char));
					if(!path) {
				        perror("malloc");
				        return -1;
				    }
					char *name = malloc(100);
					if(!name) {
				        perror("malloc");
				        return -1;
				    }
					strcat(path, DirName);
					strcat(path, "/");

					char *justName = strstr(messaggio.data.buf, "/");
					if(justName != NULL) {
						justName++;
						strcat(path, justName);
						strncpy(name, justName, strlen(messaggio.data.buf)+1);
					}
					else {
						strcat(path, messaggio.data.buf);
						strncpy(name, messaggio.data.buf, strlen(messaggio.data.buf)+1);
					}
					
					int fp;
					if((fp = open( path, O_CREAT | O_WRONLY, 0777)) == -1) {
						perror("Aprendo il file");
						exit(EXIT_FAILURE);
					}
					message_data_t file;
					readData(fd, &file);

					if( file.hdr.len > MaxFileSize*1024 ) {
						setHeader(&messaggio.hdr, OP_MSG_TOOLONG, "server");
						pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
						close(fp);
						chattyStats.nerrors++;
						free(file.buf);
						free(messaggio.data.buf);
						free(path);
						free(name);
						break;						
					}

					free(messaggio.data.buf);
					messaggio.data.buf = name;

					messaggio.hdr.op = FILE_MESSAGE;
					write(fp, file.buf, file.hdr.len);
					close(fp);

					int index_receiver = isOnline(fd_list, messaggio.data.hdr.receiver);
					//utente
					if( ret2 == 0 ) {
						insertMessageHash(Hash, &messaggio);
						if( index_receiver != -1 && fd_list->array[index_receiver].online == 1 ) {
							messaggio.hdr.op = FILE_MESSAGE;
							messaggio.data.hdr.len = strlen(name)+1;
							pthread_mutex_lock(&fd_list->array[index_receiver].fd_lock);
							VERIFY(ret, sendHeader(fd_list->array[index_receiver].fd, &messaggio.hdr), "to send Header");
							VERIFY(ret, sendData(fd_list->array[index_receiver].fd, &messaggio.data), "to senData");
							pthread_mutex_unlock(&fd_list->array[index_receiver].fd_lock);
							chattyStats.nfiledelivered++;
						}
						else chattyStats.nfilenotdelivered++;
					}
					//gruppo
					else {
						members_group_t* mpt = ptr->member_head;
						setHeader(&messaggio.hdr, FILE_MESSAGE, messaggio.hdr.sender);
						setData(&messaggio.data, messaggio.data.hdr.receiver, name, strlen(name)+1);
						while(mpt != NULL) {
							strncpy(messaggio.data.hdr.receiver, mpt->nick, MaxNameLength);
							insertMessageHash(Hash, &messaggio);
							int index = isOnline(fd_list, mpt->nick);
							if(index != -1) {
								pthread_mutex_lock(&fd_list->array[index_receiver].fd_lock);
								VERIFY(ret, sendHeader(fd_list->array[index_receiver].fd, &messaggio.hdr), "to send Header");
								VERIFY(ret, sendData(fd_list->array[index_receiver].fd, &messaggio.data), "to senData");
								pthread_mutex_unlock(&fd_list->array[index_receiver].fd_lock);
							}
							mpt = mpt->next;
						}
					}

					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
					
					free(file.buf);
					free(name);
					free(path);
				}break;

			
				case GETFILE_OP: {
				

					char nome_file[strlen(messaggio.data.buf)+1];
					strncpy(nome_file, messaggio.data.buf, strlen(messaggio.data.buf)+1);
					int ret = 0;
					int index_sender = isOnline(fd_list, messaggio.hdr.sender);

					char *path = calloc(100, sizeof(char));
					strncpy(path, DirName, strlen(DirName)+1);
					strcat(path, "/");
					strcat(path, messaggio.data.buf);

					int fp;
					//file inesistente.
					if( (fp= open(path , O_RDONLY)) < 0 ) {
						
						setHeader(&messaggio.hdr, OP_NO_SUCH_FILE, "server");
						pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
						chattyStats.nerrors++;
						free(path);
						break;
					}

					message_t reply;
					memset(&reply, 0, sizeof(message_t));

					setHeader(&(reply.hdr), OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendHeader(fd, &reply.hdr), "sendHeader GETFILE_OP");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));

					reply.data.buf = calloc(MaxFileSize*1024, sizeof(char));
					ret = read(fp, reply.data.buf, MaxFileSize*1024);
					reply.data.hdr.len = ret;

					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendData(fd_list->array[index_sender].fd, &reply.data), "sendData GETFILE_OP");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
					chattyStats.nfilenotdelivered--;
					chattyStats.nfiledelivered++;
					
					free(reply.data.buf);
					free(messaggio.data.buf);
					close(fp);
					free(path);
				}break;

				case GETPREVMSGS_OP: {
					
					int ret = 0;
					int index_sender = isOnline(fd_list, messaggio.hdr.sender);
					
					SingleClient_t *head = getClientFromNick(Hash, messaggio.hdr.sender);
					
					int key = hashFunction(head->name);
					if(key == 0){
						setHeader(&messaggio.hdr, OP_HASH_FAIL, "server");
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						chattyStats.nerrors++;
						break;					
					}
					pthread_mutex_lock(&Hash[key].lock);
					
					message_t reply;
					memset(&reply, 0, sizeof(message_t));
					reply.hdr.op = OP_OK;
					reply.data.hdr.len = sizeof(size_t);
					size_t dim = head->history_length;
					reply.data.buf = (char*) &dim;

					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendRequest(fd, &reply), "sendRequest");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));

					if(head->history_length > 0 && index_sender != -1 && fd_list->array[index_sender].online == 1) {
						for( SingleMessage_t *ptr=head->history_head; ptr != NULL; ptr = ptr->next ) {
							setHeader(&messaggio.hdr, ptr->msg->hdr.op, ptr->msg->hdr.sender);
							setData(&messaggio.data, ptr->msg->data.hdr.receiver, ptr->msg->data.buf, ptr->msg->data.hdr.len);
							
							pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
							VERIFY(ret, sendRequest(fd, &messaggio), "sendRequest");
							pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
							chattyStats.ndelivered++;
						}
					}
					pthread_mutex_unlock(&Hash[key].lock);
					
				}break;

				
				case USRLIST_OP : {
					int ret = 0;
					int index_sender = isOnline(fd_list, messaggio.hdr.sender);

					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
					char *str = printConnected(fd_list);
					setData(&(messaggio.data), messaggio.data.hdr.receiver, str, (MAX_NAME_LENGTH+1)*(fd_list->NumOnline));
					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendData(fd,&(messaggio.data)), "to senData");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));

					free(str);
				}break;

				
				case UNREGISTER_OP: {
					
					int ret = 0;
					int index_sender = isOnline(fd_list, messaggio.hdr.sender);
					removeClientHash(Hash, messaggio.hdr.sender);
					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[index_sender].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[index_sender].fd_lock));
					chattyStats.nusers--;
					}break;


				case DISCONNECT_OP: {
					int ret = 0;
					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
					chattyStats.nonline--;
				}break;

				
				case CREATEGROUP_OP: {
					int ret = checkRegistered(Hash, messaggio.data.hdr.receiver);
					group_t* ptr = isAGroup(group_list, messaggio.data.hdr.receiver);
					if(ptr == NULL) continue;
					if(ptr != NULL || ret == 0) {
						setHeader(&messaggio.hdr, OP_NICK_ALREADY, "server");
						pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
						chattyStats.nerrors++;
					}
					else {
						create_group(group_list, messaggio.hdr.sender, messaggio.data.hdr.receiver);
						setHeader(&messaggio.hdr, OP_OK, "server");
						pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
					}
				} break;

				case ADDGROUP_OP: {
					int ret = 0;
					group_t* ptr = isAGroup(group_list, messaggio.data.hdr.receiver);
					
					if( ptr == NULL) {
						setHeader(&messaggio.hdr, OP_NICK_UNKNOWN, "server");
						pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
						chattyStats.nerrors++;
					}
					else {
						ret = already_member(group_list, messaggio.data.hdr.receiver, messaggio.hdr.sender);
						if (ret == 0) {
							setHeader(&messaggio.hdr, OP_FAIL, "server");
							pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
							VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
							pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
							chattyStats.nerrors++;
						}
						add_member(group_list, messaggio.data.hdr.receiver, messaggio.hdr.sender);
						print_members(group_list, messaggio.data.hdr.receiver);
						setHeader(&messaggio.hdr, OP_OK, "server");
						pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
					}
				}break;

				
				case DELGROUP_OP: {
					int ret = already_member(group_list, messaggio.data.hdr.receiver, messaggio.hdr.sender);
					if( ret == -1) {
						setHeader(&messaggio.hdr, OP_NICK_UNKNOWN, "server");
						pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
						chattyStats.nerrors++;
					}
					//controllo che sia stato rimosso.
					ret = already_member(group_list, messaggio.data.hdr.receiver, messaggio.hdr.sender);
					if (ret != 0) {
						setHeader(&messaggio.hdr, OP_FAIL, "server");
						pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
						VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
						pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
						chattyStats.nerrors++;
					}
					remove_member(group_list, messaggio.data.hdr.receiver, messaggio.hdr.sender);
					setHeader(&messaggio.hdr, OP_OK, "server");
					pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
				}break;
				//OP NON RICONOSCIUTA.
				default: {
					int ret = 0;
					setHeader(&messaggio.hdr, OP_UNKNOWN, "server");
					pthread_mutex_lock(&(fd_list->array[fd].fd_lock));
					VERIFY(ret, sendHeader(fd, &messaggio.hdr), "to send Header");
					pthread_mutex_unlock(&(fd_list->array[fd].fd_lock));
					chattyStats.nerrors--;
				}break;
			}
			FD_SET(fd,setP);
		}
	}

	pthread_exit(NULL);
}






int main(int argc, char *argv[]) {
    	if(argc != 3) {
		printf("Usage: %s -f config_file_path\n", argv[0]);
		return 0;
	}

	if(strcmp(argv[1], "-f") != 0) {
		printf("Usage: %s -f config_file_path\n", argv[0]);
		return 0;
	}
	//Reading configuration file tramite get_configuration definita in param.c
	char *par = get_configuration(argv[2], "UnixPath", "/tmp/chatty_socket");
	strncpy(UnixPath, par, strlen(par)+1);
	free(par);

	par = get_configuration(argv[2], "MaxConnections", "32");
	MaxConnections = atoi(par);
	free(par);

	par = get_configuration(argv[2], "ThreadsInPool", "10");
	ThreadsInPool = atoi(par);
	free(par);

	par = get_configuration(argv[2], "MaxMsgSize", "1000");
	MaxMsgSize = atoi(par);
	free(par);

	par = get_configuration(argv[2], "MaxFileSize", "1024");
	MaxFileSize = atoi(par);
	free(par);

	par = get_configuration(argv[2], "MaxHistMsg", "16");
	MaxHistMsgs = atoi(par);
	free(par);

	par = get_configuration(argv[2], "MaxNameLength", "15");
	MaxNameLength = atoi(par);
	free(par);

	par = get_configuration(argv[2], "DirName", "/tmp/chatty_dir ");

	char copy[100];
	int n = strlen(par)+1;
	for(int i=0; i<n; i++) {
		if(isspace(par[i]) == 0) {
			copy[i] = par[i];
		}
	}
	free(par);
	strncpy(DirName, copy, strlen(copy)+1);

	par = get_configuration(argv[2], "StatFileName", "Not Used");
	strncpy(StatFileName, par, strlen(par)+1);
	free(par);

	unlink(UnixPath);

	struct stat st = {0};

	if(stat(DirName, &st) == -1) 
		mkdir(DirName, 0777);


	pthread_t thread_worker[ThreadsInPool];
	struct sockaddr_un sa; //indirizzo del socket
	//gestione file descriptor
	int fd_socket,fd_c,fd_num=0,fd;
	int pid,err;
	fd_set rdset;

	struct timeval tempo;
	tempo.tv_sec=0;
	tempo.tv_usec=1;

	//strutture necessarie per l'intero progetto.
	Hash = create(MaxHistMsgs, MaxNameLength);
	fd_list = initFdArray(MaxConnections, 32);
	group_list = start_queue_group(MaxNameLength);

	works = malloc(sizeof(int)*MaxConnections);
	if (!works) {
		perror("malloc");
		return -1;
	}
	
	//setto l'indirizzo del socket.
	strncpy(sa.sun_path,UnixPath,UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;

	//INIZIO GESTIONE SEGNALI.

	struct sigaction tempSigAction;
	sigset_t tempSigSet;
	//Thread segnali
  	pthread_t tempThreadSignal;
  	pthread_attr_t tempThreadAttr;
	
	//Funzione per la gestione dei segnali
	gestioneSegnali(tempSigAction, tempSigSet, tempThreadSignal, tempThreadAttr);


	
	
	//INIZIO CODICE PER LA CREAZIONE DI UN SERVER E LA CREAZIONE E GESTIONE DEI THREAD. 

	//creo il socket
	if( (fd_socket=socket(AF_UNIX,SOCK_STREAM,0)) == -1){
			perror("socket non creato!");
			exit(EXIT_FAILURE);
	}

	//collego il socket ad un indirizzo specifico. 
	if( bind(fd_socket,(struct sockaddr *) &sa,sizeof(sa)) == -1){
		perror("bind");
		exit(EXIT_FAILURE);
	}

	//accetto nuove connessioni
	if( listen(fd_socket,SOMAXCONN) == -1){
		perror("creando il socket");
		exit(EXIT_FAILURE);
	}

	//creo i thread workers
	for(int i=0; i<ThreadsInPool ;i++)
		if( (err=pthread_create(&thread_worker[i],NULL, &main_thread_function,&set)) != 0){
			errno=err;
			perror("creando il thread");
			exit(errno);
		}

	//fd_num deve essere il massimo tra i descrittori che ho aperti
	if(fd_socket > fd_num)
		fd_num=fd_socket;

	//azzero la maschera set
	FD_ZERO(&set);

	//metto a 1 il bit di maschera relativo al socket del server
	FD_SET(fd_socket,&set);

	//il thread main si mette in attesa perenne dei clients
	while(cicle == 1){
		rdset=set;
		//controllo il risultato della select. le variabili sono settate per indicare il 
		switch(select(fd_num+1,&rdset,NULL,NULL,&tempo)){
			case -1:
					//errore select
				return -1;
					
			case 0: {
				//errore select
				break;
			}
			
			default:{
				//server esce dalla select, un client è pronto a scrivere. ovvero fd_set è pronto per essere utato.
				for(fd=0;fd<=fd_num;fd++){
					//controllo il bid di fd in rdset
					if(FD_ISSET(fd,&rdset)){

							if(fd == fd_socket){ //metto in attesa il server per eventuali client //fd = server
								//quindi attesa sulla accept
								fd_c=accept(fd_socket,NULL,0);
						 
								FD_SET(fd_c,&set);
								if(fd_c > fd_num)
									fd_num=fd_c;
							}

							else{
								//limite connessioni non superato. 
								if(works_index < MaxConnections){
									FD_CLR(fd,&set);
									pthread_mutex_lock(&works_lock);
									
									works[works_index]=fd;
									works_index++;
									
									//sveglio worker in attesa. 
									pthread_cond_signal(&condit_worker);
									pthread_mutex_unlock(&works_lock);
									
								}
								//troppe connessioni
								else {
									
									message_t reply;
									setHeader(&reply.hdr, OP_FAIL, "server");
									if(sendHeader(fd, &reply.hdr) > 0)
										chattyStats.nerrors++;
								}
							}
						}	
				}
			}
		}
	}

	pthread_cond_broadcast(&condit_worker);

	removeTable(Hash); //cancello tutti i clienti
	destroyFd(fd_list); //cancello la struttura degli utenti online
	remove_queue_group(group_list); //rimuovo tutti i gruppi
	free(works);

	void *status;
	for(int i=0; i<ThreadsInPool; i++) {
		pthread_join(thread_worker[i], &status);
	}

	return 0;
}
