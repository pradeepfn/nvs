#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "timecount.h"

#define MICROSEC 1000000

FILE *fp;
FILE *fp2;
int irun;
int process_id;
struct timeval t_start;
struct timeval t_end;
unsigned long tot_etime;

unsigned long get_elapsed_time(struct timeval *end, struct timeval *start){
	unsigned long diff = (end->tv_sec - start->tv_sec)*MICROSEC + end->tv_usec - start->tv_usec;
	return diff;
}

void start_time_(int *mype){
	process_id = *mype;
	char file_name[50];
	snprintf(file_name,sizeof(file_name),"stats/nvram_p%d.log",*mype);
	fp=fopen(file_name,"w");
	gettimeofday(&t_start,NULL);
	tot_etime=0;
}

void pause_time_(){
	gettimeofday(&t_end,NULL);
	tot_etime+=get_elapsed_time(&t_end,&t_start);

}

void resume_time_(){
	gettimeofday(&t_start,NULL);
}

void end_time_(){
	gettimeofday(&t_end,NULL);
	tot_etime+=get_elapsed_time(&t_end,&t_start);
	fprintf(fp,"%lu\n", tot_etime);
	fflush(fp);
	fclose(fp);

	if(process_id == 0){
		printf("batch read time:  %zd \n",tot_etime);
	}
}



void start_timestamp_(int *mype){
	char file_name[50];
	snprintf(file_name,sizeof(file_name),"stats/time_p%d.log",*mype);
	fp2=fopen(file_name,"w");
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	fprintf(fp2,"%lu:%lu\n",current_time.tv_sec, current_time.tv_usec);
	fflush(fp2);
}

void make_timestamp_(int *mype){
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	fprintf(fp2,"%lu:%lu\n",current_time.tv_sec, current_time.tv_usec);
	fflush(fp2);
}

void end_timestamp_(int *mype){
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	fprintf(fp2,"%lu:%lu\n",current_time.tv_sec, current_time.tv_usec);
	fflush(fp2);
	fclose(fp2);
}
