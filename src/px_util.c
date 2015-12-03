#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "px_util.h"
#include "px_debug.h"
#include "px_constants.h"

// read bandwidth to constant maching
// 2048Mb/s -> 600
// 1024Mb/s -> 300 
// 512Mb/s -> 150
// 256Mb/s -> 75
// 128Mb/s -> 38
// 64Mb/s ->19
// -1 relates to DRAM -> no delay


#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)


struct sigaction old_sa; 

unsigned long calc_delay_ns(size_t datasize,int bandwidth){
        unsigned long delay;
        double data_MB, sec;

        data_MB = (double)((double)datasize/(double)pow(10,6));
        sec =(double)((double)data_MB/(double)bandwidth);
        delay = sec * pow(10,9);
        return delay;
}

int __nsleep(const struct timespec *req, struct timespec *rem)
{
    struct timespec temp_rem;
    if(nanosleep(req,rem)==-1){
        __nsleep(rem,&temp_rem);
    }
        return 1;
}

int msleep(unsigned long nanosec)
{
    struct timespec req={0},rem={0};
    time_t sec=(int)(nanosec/1000000000);
    req.tv_sec=sec;
    req.tv_nsec=nanosec%1000000000;
    __nsleep(&req,&rem);
    return 1;
}


int nvmmemcpy_read(void *dest, void *src, size_t len, int rbw) {
	if(rbw > 0){
		unsigned long lat_ns;
		//read bandwidth is taken as the two times, write bandwidth
		lat_ns = calc_delay_ns(len,rbw);
		msleep(lat_ns);
	}
    memcpy(dest,src,len);
    return 0;
}

int nvmmemcpy_write(void *dest, void *src, size_t len, int wbw) {
	if(wbw > 0){
		unsigned long lat_ns;
		lat_ns = calc_delay_ns(len,wbw);
		msleep(lat_ns);
	}
    memcpy(dest,src,len);
    return 0;
}


int timeval_subtract (struct timeval *result, struct timeval *x, struct timeval *y) 
{
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / MICROSEC + 1;
    y->tv_usec -= MICROSEC * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > MICROSEC) {
    int nsec = (x->tv_usec - y->tv_usec) / MICROSEC;
    y->tv_usec += MICROSEC * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}


void install_sighandler(void (*sighandler)(int,siginfo_t *,void *)){
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sighandler;
    if (sigaction(SIGSEGV, &sa, &old_sa) == -1){ 
        handle_error("sigaction");
	}
}


void install_old_handler(){
    struct sigaction temp;
    debug("old signal handler installed");
    if (sigaction(SIGSEGV, &old_sa, &temp) == -1){
        handle_error("sigaction");
    }
}

void call_oldhandler(int signo){
    debug("old signal handler called");
    (*old_sa.sa_handler)(signo);
}

void enable_protection(void *ptr, size_t size) {
	if (mprotect(ptr, size,PROT_NONE) == -1){
        handle_error("mprotect");
	}
}

void enable_write_protection(void *ptr, size_t size) {
    if (mprotect(ptr, size,PROT_READ) == -1){
        handle_error("mprotect");
    }
}


/*
* Disable the protection of the whole aligned memory block related to each variable access
* return : size of memory
*/
long disable_protection(void *page_start_addr,size_t aligned_size){
	if (mprotect(page_start_addr,aligned_size,PROT_WRITE) == -1){
        handle_error("mprotect");
	}
    return aligned_size;
}



int get_mypeer(rcontext_t *rcontext, int myrank){
    int mypeer;

    if(myrank % 2 == 0) { //FIXME: This logic fails when offset is a even number
        mypeer = (myrank + rcontext->config_context->buddy_offset) % rcontext->nproc;
        return mypeer;
    }else {
        if(myrank >= rcontext->config_context->buddy_offset){
            mypeer = myrank - rcontext->config_context->buddy_offset;
            return mypeer;
        } else{
            mypeer = rcontext->nproc + myrank - rcontext->config_context->buddy_offset;
            return mypeer;
        }
    }
}


/*
 * input char array is size of 20
 * we null terminate it at the first space
 */
char* null_terminate(char *c_string){
    int i;
    for(i=0;i<19;i++){
        if(c_string[i] == ' '){
            c_string[i] = '\0';
            return c_string;
        }
    }
    c_string[i] = '\0';
    return c_string;
}

/*
 * find whether we have dram destined data. we partition the variable space.
 */
int is_dlog_checkpoing_data_present(var_t *list){
    var_t *s;
    int i;

    for(s = list,i=0; s != NULL; s = s->hh.next,i++){
        if(s->type == DRAM_CHECKPOINT){
            return 1;
        }
    }
    return 0;
}

void read_configs(ccontext_t *config_context,char *file_path){
    char varname[30];
    char varvalue[32];// we are reading integers in to this

    //initialize with defaults
    config_context->log_size = 2*1024*1024;
    config_context->chunk_size = 4096;
    config_context->copy_strategy = 1;
    config_context->nvram_wbw = -1;
    config_context->restart_run = 0;
    config_context->remote_checkpoint = 0;
    config_context->remote_restart = 0;
    config_context->helper_core_size =0;
    config_context->buddy_offset = 1;
    config_context->split_ratio = 0;
    config_context->cr_type = TRADITIONAL_CR;
    config_context->free_memory = -1;
    config_context->threshold_size = 4096;
    config_context->max_checkpoints = -1;
    config_context->early_copy_enabled = 0;
    config_context->ec_offset_add = 0;


    FILE *fp = fopen(file_path,"r");
    if(fp == NULL){
        printf("error while opening the file\n");
        exit(1);
    }
    while (fscanf(fp, "%s =  %s", varname, varvalue) != EOF) { // TODO: space truncate
        if (varname[0] == '#') {
            // consuming the input line starting with '#'
            fscanf(fp, "%*[^\n]");
            fscanf(fp, "%*1[\n]");
            continue;
        } else if (!strncmp(NVM_SIZE, varname, sizeof(varname))) {
            config_context->log_size = atoi(varvalue) * 1024 * 1024;
        } else if (!strncmp(CHUNK_SIZE, varname, sizeof(varname))) {
           config_context->chunk_size = atoi(varvalue);
        } else if (!strncmp(COPY_STRATEGY, varname, sizeof(varname))) {
            config_context->copy_strategy = atoi(varvalue);
        } else if (!strncmp(DEBUG_ENABLE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 1) {
                enable_debug();
            }
        } else if (!strncmp(PFILE_LOCATION, varname, sizeof(varname))) {
            strncpy(config_context->pfile_location, varvalue,
                    sizeof(config_context-> pfile_location));
        } else if (!strncmp(NVRAM_WBW, varname, sizeof(varname))) {
            config_context->nvram_wbw = atoi(varvalue);
        } else if (!strncmp(RSTART, varname, sizeof(varname))) {
            config_context->restart_run = atoi(varvalue);
        } else if (!strncmp(REMOTE_CHECKPOINT_ENABLE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 1) {
                config_context->remote_checkpoint = 1;
            }
        } else if (!strncmp(REMOTE_RESTART_ENABLE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 1) {
                config_context->remote_restart = 1;
            }
        } else if (!strncmp(BUDDY_OFFSET, varname, sizeof(varname))) {
            config_context->buddy_offset = atoi(varvalue);
        } else if (!strncmp(SPLIT_RATIO, varname, sizeof(varname))) {
            config_context->split_ratio = atoi(varvalue);
        } else if (!strncmp(CR_TYPE, varname, sizeof(varname))) {
            if (atoi(varvalue) == 0) {
                config_context->cr_type = TRADITIONAL_CR;
            } else if (atoi(varvalue) == 1) {
                config_context->cr_type = ONLINE_CR;
            }
        } else if (!strncmp(FREE_MEMORY, varname, sizeof(varname))) {
            config_context->free_memory = atol(varvalue) * 1024 * 1024;
        } else if (!strncmp(THRESHOLD_SIZE, varname, sizeof(varname))) {
            config_context->threshold_size = atoi(varvalue);
        } else if (!strncmp(MAX_CHECKPOINTS, varname, sizeof(varname))){
            config_context->max_checkpoints = atol(varvalue);
        }else if (!strncmp(EARLY_COPY_ENABLED, varname, sizeof(varname))){
            config_context->early_copy_enabled = atoi(varvalue);
        } else if (!strncmp(EARLY_COPY_OFFSET, varname, sizeof(varname))){
            config_context->ec_offset_add = atol(varvalue);
        }else if (!strncmp(HELPER_CORES, varname, sizeof(varname))){
            config_context->helper_cores[config_context->helper_core_size++] = atoi(varvalue);
        } else{
            log_err("unknown varibale : %s  please check the config",varname);
            exit(1);
        }
    }
    fclose(fp);
}