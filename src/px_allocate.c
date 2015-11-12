/*
 * The class allocates and monitors
 */

#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include <mpi.h>
#include "px_allocate.h"
#include "px_debug.h"
#include "px_util.h"
#include "sys/mman.h"
#include "px_constants.h"
#include "px_remote.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)




void protect_all_other_pages(char *varname);

void enable_write_protection(void *ptr, size_t size);

static void access_monitor_handler(int sig, siginfo_t *si, void *unused);

allocate_t alloc_struct;

int page_size;
int sig_handler_installed = 0;

volatile int done_tracking = 0;




extern int lib_process_id;
/* This function allocates a page aligned memory location and write protect it so we can track the
 * accesses of its chunks
 */

var_t *px_alighned_allocate(size_t size , char *varname) {
    long page_aligned_size;
    int status;
    void *ptr;
    var_t *s;

    page_size = sysconf(_SC_PAGESIZE);
    /*
		for a page size of 4, and a data size of 5, then the aligned data size should be 8.
		0101 + 0011 = 1000,
		and then we cut out the lower two bits.
	*/
    page_aligned_size = ((size + page_size - 1) & ~(page_size - 1));
    //debug("mem aligned allocation %ld : %ld \n", size, page_aligned_size);
    status = posix_memalign(&ptr, page_size, page_aligned_size);
    if (status != 0) {
        return NULL;
    }

    memset(ptr, 0, page_aligned_size);

    s = (var_t *)malloc(sizeof(var_t));
    s->ptr = ptr;
    s->size = size;
    s->paligned_size = page_aligned_size;
    memcpy(s->varname,varname,sizeof(char)*20);

    return s;


}

/*
 * In the singal handler we capture the write protected access,
 * 1. record the access time if its the first fault
 * 2. if its not the first fault then, overwrite the end time
 * 3. release the write protection on current chunk
 * 4. apply write protection on other chunks
 */
extern var_t *varmap;

static void access_monitor_handler(int sig, siginfo_t *si, void *unused){
    if(si != NULL && si->si_addr != NULL){
        var_t *s;
        void *pageptr;
        long offset =0;
        pageptr = si->si_addr;


        for(s=varmap;s != NULL;s=s->hh.next) {
            offset = pageptr - s->ptr;
            if(offset >=0 && offset <= s->size){ // the adress belong to this chunk.
                if(isDebugEnabled()){
                    printf("[%d] starting address of the matching chunk %p\n",lib_process_id, s->ptr);
                }
                if(s != NULL) {
                    if (!s->started_tracking) {
                        s->started_tracking = 1;
                    } else {
                        gettimeofday(&(s->end_timestamp), NULL);
                    }
                    disable_protection(s->ptr, page_size);
                    protect_all_other_pages(s->varname);
                }
                return;
            }
        }
        debug("[%d] offending memory access : %p ",lib_process_id,pageptr);
        call_oldhandler(sig);
    }
}



void start_page_tracking(){
    var_t *s;

    for(s=varmap;s!=NULL;s=s->hh.next){

        if (!sig_handler_installed) {
            install_sighandler(access_monitor_handler);
            sig_handler_installed = 1;
        }
        enable_write_protection(s->ptr, page_size); // only protect one page
    }

    debug("memory tracking started\n");
    return;
}


/*
 * remove page protection on monitored chunks and
 * restore original SIGSEGV signal handler
 */
void stop_page_tracking(){
    var_t *s;
    for(s=varmap;s!=NULL;s=s->hh.next){
        disable_protection(s->ptr,page_size);
    }
    debug("memory tracking disabled\n");
    install_old_handler();
    return;
}

void broadcast_page_tracking(){
    timeoffset_t offset_ary[100];
    int i,j;
    int n_vars=0;

    const int nitems = 3;
    MPI_Datatype timeoffset;
    MPI_Datatype types[3] = {MPI_CHAR, MPI_LONG, MPI_LONG};
    int blocklen[3] = {20, 1, 1};
    MPI_Aint disp[3];

    disp[0] = offsetof(timeoffset_t, varname);
    disp[1] = offsetof(timeoffset_t, seconds);
    disp[2] = offsetof(timeoffset_t, micro);

    MPI_Type_create_struct(nitems, blocklen, disp, types, &timeoffset);
    MPI_Type_commit(&timeoffset);


    if(lib_process_id == 0) {
        //populate the structure
        var_t *s;
        for (s = varmap, i = 0; s != NULL; s = s->hh.next, i++) {
            strncpy(offset_ary[i].varname, s->varname, 20);
            offset_ary[i].seconds = s->earlycopy_time_offset.tv_sec;
            offset_ary[i].micro = s->earlycopy_time_offset.tv_usec;
            n_vars++;
        }
    }

    //broadcast
    MPI_Bcast((void*)&n_vars,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast((void*)offset_ary,n_vars*sizeof(offset_t),timeoffset,0,MPI_COMM_WORLD);

    var_t *s2;
    if(lib_process_id != 0){
      for(j=0;j<n_vars;j++){
          HASH_FIND_STR(varmap,offset_ary[j].varname,s2);
          assert(s2 != NULL);
          s2->earlycopy_time_offset.tv_sec = offset_ary[j].seconds;
          s2->earlycopy_time_offset.tv_usec = offset_ary[j].micro;
      }
    }

    debug("[%d] varname : %s , second : %ld , microseconds : %ld",lib_process_id, offset_ary[0].varname,
          offset_ary[0].seconds,offset_ary[0].micro);
    debug("[%d] varname : %s , second : %ld , microseconds : %ld",lib_process_id, offset_ary[1].varname,
          offset_ary[1].seconds,offset_ary[1].micro);

}


/*
 * we are piggy backing the enabling page protection on memory chunks
 * within page fault itself.
 */
void protect_all_other_pages(char *varname) {
    var_t *s;

    for(s=varmap;s!=NULL;s=s->hh.next){
        if(!strncmp(s->varname,varname,20)){
            continue;
        }
        enable_write_protection(s->ptr,page_size);
        //debug("enabling page protection except for variable %s\n",s->varname);
    }

}



/*
 *  1. write the current values to a log file
 *  2. rest the timing fields
 *  3. re-enable memory protection
 *
 *  we want to capture the timings of the next iterations of the computation
 */
FILE *fp;
extern struct timeval px_start_time;
void flush_access_times(){
    char file_name[50];
    var_t *s;
    DIR* dir = opendir("stats");

    if(dir){
        snprintf(file_name,sizeof(file_name),"stats/variable_access.log");
        fp=fopen(file_name,"a+");
        for(s=varmap;s!=NULL;s=s->hh.next){
            fprintf(fp,"%s ,%lu,%lu ,%lu\n",s->varname,s->size,s->end_timestamp.tv_sec, s->end_timestamp.tv_usec);
            //enable_write_protection(s->pageptr,page_size);
            //s->started_tracking = 0;
        }
        fflush(fp);
    }else{ // directory does not exist
        printf("Error: no stats directory found.\n\n");
        assert(0);
    }
}



/* compare a to b (cast a and b appropriately)
   * return (int) -1 if (a < b)
   * return (int)  0 if (a == b)
   * return (int)  1 if (a > b)
   */
int decending_time_sort(var_t *a, var_t *b){
    if(timercmp(&(a->end_timestamp),&(b->end_timestamp),>)){ // if a timestamp greater than b
        return -1;
    }else if(timercmp(&(a->end_timestamp),&(b->end_timestamp),==)){
        return 0;
    }else if(timercmp(&(a->end_timestamp),&(b->end_timestamp),<)){
        return 1;
    }else{
        assert("wrong execution path");
        exit(1);
    }

}



/*
 * 1. sort the variables decending order on last access time - we need variables in the critical
 * path to be in DRAM
 * 2. select the maximum number of variables that fits in as per the free mem region
 * 3. if online C/R - each DRAM variable needs 2X space , same space if traditional C/R
 */
extern int cr_type;
extern int n_processes;
extern int threshold_size;

void decide_checkpoint_split(var_t *list, long long freemem) {
    char carray[100][20]; //contiguous memory : over provisioned
    var_t *s;
    int j, i = 0;
    int n_vars = 0;


    if (lib_process_id == 0) { // root process does the split
        if (cr_type == ONLINE_CR) {
            freemem = freemem / 2; // double in memory checkpointing
        }
        //lets sort the hashmap
        HASH_SORT(list, decending_time_sort);

        //map after sorting
        for (s = list; s != NULL; s = s->hh.next) {
            if (s->paligned_size < freemem && s->size >= threshold_size) { // it fits in
                freemem -= s->paligned_size; // TODO : restart needs aligned size
                assert(i < 100); // we are working with a overprovisioned array
                strncpy(carray[i], s->varname, 20);
                n_vars++;
                i++;
                log_info("variable : %s  chosen for DRAM checkpoint\n", s->varname);
            }

        }
    }


    // 1. broadcast number of elements
    // 2. broadcast array
    MPI_Bcast((void *)&n_vars,1,MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast((void *)carray,n_vars*20,MPI_CHAR,0,MPI_COMM_WORLD);

    //transfer the learned splits in to internal data structures
    for(j=0;j< n_vars;j++){
        //debug("[%d] variable to match : %s ",lib_process_id, carray[j]);
        HASH_FIND_STR(list,carray[j],s);
        // debug("[%d] matched variable : %s , %s",lib_process_id, carray[j],np->var_name);
                s->type = DRAM_CHECKPOINT;
                if(cr_type == ONLINE_CR) {
                    if (lib_process_id == 0) {
                        debug("[%d] allocated remote DRAM pointers for variable %s",
                              lib_process_id, s->varname);
                    }
                    s->local_remote_ptr = remote_alloc(&s->remote_ptr, s->paligned_size);
                }


    }

}

extern struct timeval px_lchk_time;
/*
 * calculate the time in which the last access took place from the most recent checkpoint.
 */
void calc_early_copy_times(){
    var_t *s;
    for (s = varmap; s != NULL; s = s->hh.next) {
        timersub(&(s->end_timestamp),&px_lchk_time,&(s->earlycopy_time_offset));
    }
    return;
}