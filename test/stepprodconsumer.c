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
		sleep(5);
		px_init(1);
		px_obj obj;
		for(i=0;i<4;i++){

#ifdef DEDUP
			if(!px_deltaget("key1",i,&obj)){
#else
				if(!px_get("key1",i,&obj)){
#endif //compare the output
					int *array = (int *)obj.data;
					printf("content : ");
					for(j=0;j<8*1024;j+=1024){
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

			int touch_ptrn[4][8] = { {1,1,1,1,1,1,1,1},{0,0,1,1,1,1,0,0},{1,0,1,1,0,1,1,1},{1,1,1,1,1,1,1,1}};

			px_init(1);
			px_obj obj;
			px_create("key1",8*1024*sizeof(int),&obj); // 8 pages
			int *array = (int *)obj.data;
			for( i=0;i<4;i++){

			int *array = (int *)obj.data;
				for(j=0;j<8;j++){
					if(touch_ptrn[i][j] == 1){
						array[j*1024] = i;
					}
				}
				//if(px_commit("key1", i)){ assert(0);}
				if(px_snapshot()){ assert(0);}
			}
			sleep(5);
			px_finalize();
		}
		return 0;
	}
