#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "phoenix.h"

/* testing phoenix for producer consumer */
int main(){

	pid_t pid;

	pid = fork();
	if(pid > 0){
		int i,j;
		//parent
		sleep(1);
		px_init(1);
		px_obj obj;
		for(i=0;i<4;i++){
			if(!px_get("key1",i,&obj)){
				//compare the output
				int *array = (int *)obj.data;
				printf("content : ");
				for(j=0;j<4*1024;j+=1024){
					printf("%d ",array[j]);
					//assert(strncmp(obj.data,"hello",6) == 0);
				}
				printf("\n");
			}else{
				printf("Error: object not found\n");
			}
		}

	}else{
		//child
		int i,j;
		px_init(1);
		px_obj obj;
		//allocate 4 pages
		px_create("key1",4*1024*sizeof(int),&obj); // 4 pages
		int *array = (int *)obj.data;
		for( i=0;i<4;i++){
			for(j=0;j<(4-i)*1024;j+=1024){
				array[j] = i;
			}
			if(px_commit("key1", i)){ assert(0);}
		}
		sleep(5);
		px_finalize();
	}
	return 0;
}
