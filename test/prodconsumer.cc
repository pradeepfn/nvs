#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "phoenix.h"

/* testing phoenix for producer consumer */
int main(){

	char *str[5] = {"hello", "world", "i" , "am", "phoenix"};
	pid_t pid;

	pid = fork();
	if(pid > 0){
		sleep(1);
		printf("consumer parent\n");
		px_init(1);
		px_obj obj;
		if(!px_get("key1",1,&obj)){
			//compare the output
			assert(strncmp((char *)obj.data,"hello",6) == 0);
		}else{
			printf("Error: object not found\n");
		}

	}else{
		printf("producer child\n");
		px_init(1);
		px_obj obj;
		px_create("key1",6,&obj);
		strncpy((char *)obj.data,"hello",6);
		px_obj cmtobj;
		if(px_commit("key1", 1)){ assert(0);}
		sleep(5);
		px_finalize();
	}
	return 0;
}
