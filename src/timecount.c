#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include "timecount.h"

#define MICROSEC 1000000

FILE *fp;
int irun;
int process_id;
struct timeval t_start;
struct timeval t_end;
unsigned long tot_etime;

unsigned long get_elapsed_time(struct timeval *end, struct timeval *start){
	unsigned long diff = (end->tv_sec - start->tv_sec)*MICROSEC + end->tv_usec - start->tv_usec;
	return diff;
}

void start_time_(int *prefix,int *processes, int *mype, int *mpsi, int *restart){
	irun=*restart;
	process_id=*mype;
	if(irun == 1){
		char file_name[50];
		snprintf(file_name,sizeof(file_name),"stats/nvram%d_n%d_p%d_mpsi%d.log",*prefix,*processes,*mype,*mpsi);
		fp=fopen(file_name,"a+");
	}
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
	if(irun == 1){//write upon valid data reads
		fprintf(fp,"%lu\n", tot_etime);
		fclose(fp);
	}
	if(process_id == 0){
		printf("batch read time:  %zd \n",tot_etime);
	}
}



void start_timestamp_(int *processes, int *mype, int *mpsi, int *restart){
	char file_name[50];
	snprintf(file_name,sizeof(file_name),"stats/tot_n%d_p%d_mpsi%d.log",*processes,*mype,*mpsi);
	fp=fopen(file_name,"w");
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	fprintf(fp,"%lu:%lu\n",current_time.tv_sec, current_time.tv_usec);
	fclose(fp);
}

void end_timestamp_(int *processes, int *mype, int *mpsi, int *restart){
	char file_name[50];
	snprintf(file_name,sizeof(file_name),"stats/tot_n%d_p%d_mpsi%d.log",*processes,*mype,*mpsi);
	fp=fopen(file_name,"a");
	struct timeval current_time;
	gettimeofday(&current_time,NULL);
	fprintf(fp,"%lu:%lu\n",current_time.tv_sec, current_time.tv_usec);
	fclose(fp);
}
