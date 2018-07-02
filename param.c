//* @autore Davide Montagno Bozzone - Corso A

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
#include <fcntl.h>
#include <param.h>


char *get_configuration(char path[], char parameter[], char standard[]) {
	FILE * fp;
	char buf[500];
	char *delim;
	if((fp = fopen(path, "r")) != NULL){
		rewind(fp);
	    while(feof(fp) == 0) {
	    	if(fgets(buf, 500, fp)) {
	    		int comment = 0;
	    		int i = 0;
	    		while(buf[i] != '\n' || i == sizeof(buf)-1) {
	    			if(buf[i] == '#') {
	    				comment++;
	    			}
	 				i++;
	    		}
	    		if( comment == 0 ) {
	    			delim = strstr(buf, parameter);
	    			if(delim != NULL) {
	    				delim = strstr(delim, "=");
	    				int j=1;
	    				while(delim[j] == ' ' || delim[j] == '	' || j == sizeof(delim))
	    					j++;
	    				delim += j;
	    				char *s = calloc(strlen(delim), sizeof(char));
	    				strncpy(s, delim, strlen(delim)-1);
	    				rewind(fp);
	    				char *new = malloc(strlen(delim)+1);
	    				//rimuovo gli spazi bianchi!
						int count=0;
						for (int i = 0; s[i]; i++)
							if (s[i] != ' ')
								s[count++] = s[i]; 
						s[count] = '\0';
						strcpy(new,s);
	    				free(s);
	      				fclose(fp);
	      				return new;
	    			}
	    		}
	    	}
	    }
	    rewind(fp);
	    char *s = calloc(strlen(standard)+1, sizeof(char));
	    strncpy(s, standard, strlen(standard));
	    fclose(fp);
	    return s;
	}
	else {
	   	char *s = calloc(strlen(standard)+1, sizeof(char));
	    strncpy(s, standard, strlen(standard));
	    return s;		
	}
}
