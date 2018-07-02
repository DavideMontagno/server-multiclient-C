//* @autore Davide Montagno Bozzone 535910 - Corso A

#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <time.h>
#include "message.h"
#include "connections.h"



#ifndef SYSCALL
#define SYSCALL(a,b,c)    if((a=b) < 0 ){perror(c);return a;}
#endif

#ifndef CHECK_CALL
#define CHECK_CALL(a,b,c)    if((a=b) == 0){perror(c);return -1;}
#endif

#ifndef CHECK_CALL_PTR
#define CHECK_CALL_PTR(a,b)    if((a) == NULL){perror(b);return -1;}
#endif


int openConnection(char* path, unsigned int ntimes, unsigned int secs) {

	int fd_skt;
	struct sockaddr_un sa;
	CHECK_CALL_PTR(strncpy(sa.sun_path, path, UNIX_PATH_MAX), "strncpy in openConnection == NULL");
	sa.sun_family = AF_UNIX;
	SYSCALL(fd_skt, socket(AF_UNIX, SOCK_STREAM, 0), "socket in openConnection == -1");

	for(int i=0; i<ntimes; i++) {
		if(connect(fd_skt, (struct sockaddr*)&sa, sizeof(sa)) == 0)
			return fd_skt;
		else 
			sleep(secs);
	}
	return -1;
}


int readHeader(long fd, message_hdr_t *hdr) {

	if(fd<0 || hdr==NULL) {
		errno = EINVAL;
    	return -1;	
	}

	int ret,Nread=0;

	SYSCALL(ret, read(fd, &(hdr->op), sizeof(op_t)), "reading OP");

	Nread=ret;
	SYSCALL(ret, read(fd, (hdr->sender), MAX_NAME_LENGTH+1), "reading sender");

	Nread+=ret;
	return Nread;
}


int readDataHeader(long fd, message_data_hdr_t *hdr) {

	if(fd<0 || hdr==NULL) {
		errno = EINVAL;
    	return -1;	
	}

	int ret,Nread=0;
	SYSCALL(ret, read(fd, (hdr->receiver), MAX_NAME_LENGTH+1), "reading reciver");
	Nread+=ret;
	
	SYSCALL(ret, read(fd, &(hdr->len), sizeof(unsigned int)), "reading length");
	Nread+=ret;	
	return Nread;	
} 

int readData(long fd, message_data_t *data) {

	if(fd<0 || data==NULL) {
		errno = EINVAL;
    	return -1;	
	}

	int ret, done = 0,Nread=0;

	memset(data,0,sizeof(*data));

	Nread+=readDataHeader(fd, &(data->hdr));
	if((data->hdr).len != 0)
		CHECK_CALL_PTR(data->buf = malloc((data->hdr).len+1), "malloc data length");

	char *ptr = data->buf;
	while(done < (data->hdr).len) {
		SYSCALL(ret, read(fd, ptr, (data->hdr).len-done), "reading data");
		done = done + ret;
		ptr = &((data->buf)[done]);
	}
	if( done != (data->hdr).len){
		return -1;
	}
	
	Nread+=(data->hdr).len;
	return Nread;
}

int readMsg(long fd, message_t *msg) {

	if(fd<0 || msg==NULL) {
		errno = EINVAL;
    	return -1;	
	}

	int ret,Nread=0;
	SYSCALL(ret, readHeader(fd, &(msg->hdr)), "readHeader");
	Nread+=ret;
	SYSCALL(ret, readData(fd, &(msg->data)), "readData");
	Nread+=ret;

	return Nread;
}

int sendRequest(long fd, message_t *msg) {
	
	if(fd<0 || msg==NULL) {
		errno = EINVAL;
    	return -1;	
	}
	
	int ret, done = 0;
	errno = 0;
	
	SYSCALL(ret, write(fd, &(msg->hdr).op, sizeof(op_t)), "writing op sendRequest");
	SYSCALL(ret, write(fd, (msg->hdr).sender, MAX_NAME_LENGTH+1), "writing sender sendRequest");
	SYSCALL(ret, write(fd, ((msg->data).hdr).receiver, MAX_NAME_LENGTH+1 ), "writing reciver sendRequest");
	SYSCALL(ret, write(fd, &((msg->data).hdr.len), sizeof(unsigned int)), "writing length sendRequest");

	char *ptr = (msg->data).buf;

	while(done < ((msg->data).hdr).len && ptr != NULL) {
		SYSCALL(ret, write(fd, ptr, (msg->data).hdr.len - done), "writing data sendRequest");
		done = done + ret;
		ptr = &(msg->data).buf[done];
	}
	if(errno != 0)
		return -1;
	return 0;
	
}

int sendData(long fd,message_data_t * data){
	
	if(fd<0 || data==NULL) {
		errno = EINVAL;
    	return -1;	
	}	
	
	int ret,done=0;
	SYSCALL(ret,write(fd,&((data->hdr).receiver),MAX_NAME_LENGTH+1),"writing Data receiver");
	SYSCALL(ret,write(fd,&((data->hdr).len),sizeof(unsigned int)),"writing Data len");


	char *ptr = data->buf;
	while(done < ((data->hdr).len) && ptr != NULL) {
		SYSCALL(ret, write(fd, ptr, ((data->hdr).len) - done), "writing data");
		done = done + ret;
		ptr = &((data->buf)[done]);
	}
	if(errno != 0)
		return -1;
	return 0;
}


int sendHeader(long fd,message_hdr_t *hdr){

	if(fd<0 || hdr==NULL) {
		errno = EINVAL;
    	return -1;	
	}
	
	int ret;
	if(fd == -1 && hdr == NULL)
		return -1;
	
	SYSCALL(ret, write(fd, &(hdr->op), sizeof(op_t)), "writing op sendHeader");
	SYSCALL(ret, write(fd, &(hdr->sender), MAX_NAME_LENGTH+1), "writing sender sendHeader");
	
	return 0;
}
