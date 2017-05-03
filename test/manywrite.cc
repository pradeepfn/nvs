#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>

#include "phoenix.h"

int main(){
	long N = 10000;
	char key[50];
	long i;
	px_init(1);
	px_obj obj;
	for(i = 0; i < N; i++){
		snprintf(key, 50, "key%lu", i);
		int size = rand() %50;
		px_create(key,size,&obj);
	}
	for( i=0;i<10;i++){
		if(px_snapshot()){ assert(0);}
		printf("checkpointed...\n");
	}
	px_finalize();
	return 0;
}
