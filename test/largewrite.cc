#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>

#include "phoenix.h"

/* testing phoenix for producer consumer */
int main(){

	//child
	int i,j;
	px_init(1);
	px_obj obj;
	px_create("key1",10000*1024*sizeof(int),&obj); // 8 pages
	px_create("key2",5000*1024*sizeof(int),&obj); // 8 pages
	px_create("key3",7000*1024*sizeof(int),&obj); // 8 pages
	px_create("key4",10000*1024*sizeof(int),&obj); // 8 pages
	px_create("key5",10000*1024*sizeof(int),&obj); // 8 pages
	int *array = (int *)obj.data;
	for( i=0;i<10;i++){

		/*int *array = (int *)obj.data;
		  for(j=0;j<8;j++){
		  if(touch_ptrn[i][j] == 1){
		  array[j*1024] = i;
		  }
		  }
		  if(px_snapshot()){ assert(0);}
		  }*/
		if(px_snapshot()){ assert(0);}
		printf("checkpointed...\n");
}
px_finalize();
return 0;
}
