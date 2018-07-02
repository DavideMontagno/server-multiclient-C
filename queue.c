//* @autore Davide Montagno Bozzone - Corso A

#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <queue.h>
#include <message.h>

list_members_t *start_new_List(int MaxHistMsg, int MaxNameLength) {
    if(MaxNameLength <=0 || MaxNameLength <= 0)
        return NULL;
    list_members_t *q = malloc(sizeof(list_members_t));
    if (!q) return NULL;
    q->queue_head = NULL;
    q->queue_tail = NULL;
    q->queue_length = 0;
    q->MaxHistMsg = MaxHistMsg;
    q->MaxNameLength = MaxNameLength;
    return q;
}



void remove_all_list(list_members_t *q) {
    if(q == NULL)
        return;

    while(q->queue_head != NULL) {
        SingleClient_t *c = q->queue_head;
        while(c->history_head != NULL) {
            SingleMessage_t *m = c->history_head;
            c->history_head = c->history_head->next;
            free(m->msg->data.buf);
            free(m->msg);
            free(m);
        }
        q->queue_head = q->queue_head->next;
        free(c->name);
        free(c);
    }
    free(q);
}


void printQueue(list_members_t *q) {

    if(q == NULL)
        return;

    SingleClient_t *p = q->queue_head;
	while(p != NULL) {
    	SingleMessage_t *m = p->history_head;
    	printf("%s ", p->name);
        while(m != NULL) {
        	printf("[%s]: ", m->msg->hdr.sender);
            printf("{%s}, ", m->msg->data.buf);
            m = m->next;
        }
        printf("\n");
        p = p->next;
    }
    return;
}


int alreadyRegistered(list_members_t *q, char nick[]) {

	if(q == NULL)
	return -1;
	SingleClient_t *ptr = q->queue_head;
	while(ptr != NULL && ((strcmp(nick, ptr->name) != 0))) {
		
		ptr = ptr->next;
	}
	if(ptr == NULL) return -1;
	else return 0;
    
}


void insertClient(list_members_t *q, char nick[]) {
	if (q == NULL)
		return;

    
    if(alreadyRegistered(q, nick) == 0)
    	return;

    SingleClient_t *client = malloc(sizeof(SingleClient_t));
    if(client == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    client->name = malloc(sizeof(char)*q->MaxNameLength+1);
    if(client->name==NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strcpy( client->name, nick );

    if( q->queue_head == NULL ) {
        q->queue_head = client;
        q->queue_tail = client;
        q->queue_length = 1;
        client->history_head = NULL;
        client->history_tail = NULL;
        client->next = NULL;
        client->history_length = 0;
        return;
    }
    else {
        q->queue_tail->next = client;
        q->queue_tail = client;
        q->queue_length++;
        client->history_head = NULL;
        client->history_tail = NULL;
        client->next = NULL;
        client->history_length = 0;
        return;
    }
    free(client->name);
    free(client);
    return;
}

int insertMessage(list_members_t *q, message_t* msg) {
    if(q == NULL || msg == NULL)
        return -1;

    SingleClient_t *ptr = q->queue_head;
    int added=0;

    message_t *new = malloc(sizeof(message_t));
    if(new == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    new->hdr.op = msg->hdr.op;
    strcpy(new->hdr.sender, msg->hdr.sender);
    strcpy(new->data.hdr.receiver, msg->data.hdr.receiver);
    new->data.hdr.len = msg->data.hdr.len;
    new->data.buf = malloc(msg->data.hdr.len);
    if(new->data.buf == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    strcpy(new->data.buf, msg->data.buf);

    for(ptr; ptr!=NULL; ptr=ptr->next) {
        if(strcmp(ptr->name, msg->data.hdr.receiver) == 0)  {
            if(ptr->history_head == NULL) {
            	SingleMessage_t *messaggio = malloc(sizeof(SingleMessage_t));
                if(messaggio==NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
    			messaggio->msg = new;
                messaggio->next = NULL;
                ptr->history_head = messaggio;
                ptr->history_tail = messaggio;
                ptr->history_length++;
                added++;
            }
            else {
            	SingleMessage_t *messaggio = malloc(sizeof(SingleMessage_t));
                if(!messaggio) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
    			messaggio->msg = new;
                messaggio->next = ptr->history_head;
                ptr->history_head = messaggio;
                ptr->history_length++;
                added++;
            }
 
            if(ptr->history_length > q->MaxHistMsg) {
                SingleMessage_t *del = ptr->history_head;
                while(del->next != ptr->history_tail)
                    del = del->next;
                ptr->history_tail = del;
            	del = del->next;
            	ptr->history_tail->next = NULL;
                free(del->msg->data.buf);
               	free(del->msg);
                free(del);
            	ptr->history_length--;
        	}
            if (added == 1) {
                break;
                return 0;
            }
        }
    }
    return -1;
}

void removeClient(list_members_t *q, char nick[]) {

	if(q == NULL)
		return;

    SingleClient_t *p = q->queue_head;
    SingleClient_t *aux = q->queue_head;

    for(p; p != NULL; p = p->next) {
    	if(strcmp(p->name, nick) == 0)
    		break;
    	else aux = p;
    }

    if(p == NULL) {
    	printf("Impossibile rimuovere il client - non è registrato.\n");
        return;
    }
    if(p == q->queue_head) {
    	q->queue_head = q->queue_head->next;
    	q->queue_length--;
    }
    else if (p->next == NULL) {
    	aux->next = NULL;
        q->queue_tail = aux;
    }
    else {
    	aux->next = p->next;
    	q->queue_length--;
    }

    while(p->history_head != p->history_tail) {
    	SingleMessage_t *m = p->history_head;
        p->history_head = p->history_head->next;
        free(m);
    }

    free(p->history_head);
    free(p->name);
    free(p);
    return;
}


unsigned long length(list_members_t *q) {
    unsigned long len = q->queue_length;
    return len;
}

void insertMessageBroadcast(list_members_t *q, message_t *messaggio) {
    if(q == NULL || messaggio == NULL)
        return;

    int added = 0;
    for(SingleClient_t *ptr = q->queue_head; ptr != NULL; ptr = ptr->next) {
        if(strcmp(ptr->name, messaggio->hdr.sender) != 0) { //verifico che non sia il mittente. 
            message_t *new = malloc(sizeof(message_t));
            if(!new) {
                perror("malloc");
                exit(EXIT_FAILURE);
            }
            added = 0;
            new->hdr.op = messaggio->hdr.op;
            strcpy(new->hdr.sender, messaggio->hdr.sender);
            strcpy(new->data.hdr.receiver, ptr->name);
           	new->data.hdr.len = messaggio->data.hdr.len;
            new->data.buf = calloc(messaggio->data.hdr.len, sizeof(char));
            strcpy(new->data.buf, messaggio->data.buf);
            SingleMessage_t *newMess = calloc(1, sizeof(SingleMessage_t));
            newMess->msg = new;

            if(ptr->history_head == NULL) {
                ptr->history_head = newMess;
                ptr->history_tail = newMess;
                ptr->history_length++;
           	}
            else {
               	newMess->next = ptr->history_head;
              	ptr->history_head = newMess;
     			ptr->history_length++;
            }

            if(ptr->history_length > q->MaxHistMsg) {
            	SingleMessage_t *del = ptr->history_head;
                while(del->next != ptr->history_tail)
           	        del = del->next;
                ptr->history_tail = del;
                del = del->next;
                ptr->history_tail->next = NULL;
                ptr->history_length--;
                free(del->msg->data.buf);
                free(del->msg);
                free(del);
            }
        }
    }
    return;
}


SingleClient_t *getNodefromNick(list_members_t *q, char nick[]) {
    if(q == NULL)
        return NULL;
    SingleClient_t *head;
    for(SingleClient_t *ptr = q->queue_head; ptr != NULL; ptr = ptr->next) {
        if(strcmp(ptr->name, nick) == 0) {
            head = ptr;
        }
    }
    return head;
}
