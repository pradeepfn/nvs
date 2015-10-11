/*
 * The class allocates and monitors
 */

#include <unistd.h>
#include <sys/time.h>
#include <dirent.h>
#include "px_allocate.h"
#include "px_debug.h"
#include "px_util.h"
#include "signal.h"
#include "sys/mman.h"
#include "px_constants.h"
#include "px_remote.h"

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)




void protect_all_other_pages(char *varname);

void enable_write_protection(void *ptr, size_t size);

static void access_monitor_handler(int sig, siginfo_t *si, void *unused);

allocate_t alloc_struct;

pagemap_t *pagemap;
int page_size;
int sig_handler_installed = 0;

volatile int done_tracking = 0;




extern int lib_process_id;
/* This function allocates a page aligned memory location and write protect it so we can track the
 * accesses of its chunks
 */

void *px_alighned_allocate(size_t size , char *varname) {
    long page_aligned_size;
    int s;
    void *ptr;

    page_size = sysconf(_SC_PAGESIZE);
    /*
		for a page size of 4, and a data size of 5, then the aligned data size should be 8.
		0101 + 0011 = 1000,
		and then we cut out the lower two bits.
	*/
    page_aligned_size = ((size + page_size - 1) & ~(page_size - 1));
    //debug("mem aligned allocation %ld : %ld \n", size, page_aligned_size);
    s = posix_memalign(&ptr, page_size, page_aligned_size);
    if (s != 0) {
        handle_error("posix memalign");
    }
    memset(ptr, 0, page_aligned_size);

    if (lib_process_id == 0) {  // we track access only in first process
        if (!sig_handler_installed) {
            install_sighandler(access_monitor_handler);
            sig_handler_installed = 1;
        }
        enable_write_protection(ptr, page_size); // only protect one page
        pagemap_put(&pagemap, varname, ptr, NULL, size, page_aligned_size, NULL);
    }

    return ptr;


}

/*
 * In the singal handler we capture the write protected access,
 * 1. record the access time if its the first fault
 * 2. if its not the first fault then, overwrite the end time
 * 3. release the write protection on current chunk
 * 4. apply write protection on other chunks
 */
static void access_monitor_handler(int sig, siginfo_t *si, void *unused){
    if(si != NULL && si->si_addr != NULL){
        pagemap_t *pagenode, *tmp;
        void *pageptr;
        long offset =0;
        pageptr = si->si_addr;


        HASH_ITER(hh, pagemap, pagenode, tmp) {
            offset = pageptr - pagenode->pageptr;
            if(offset >=0 && offset <= pagenode->size){ // the adress belong to this chunk.
                if(isDebugEnabled()){
                    printf("[%d] starting address of the matching chunk %p\n",lib_process_id,pagenode->pageptr);
                }
                if(pagenode != NULL) {
                    if (!pagenode->started_tracking) {
                        gettimeofday(&(pagenode->start_timestamp), NULL);
                        pagenode->started_tracking = 1;
                        //debug("first page access for variable %s",pagenode->varname);
                    } else {
                        gettimeofday(&(pagenode->end_timestamp), NULL);

                    }
                    disable_protection(pagenode->pageptr, page_size);
                    protect_all_other_pages(pagenode->varname);
                }
                return;
            }
        }
        debug("[%d] offending memory access : %p ",lib_process_id,pageptr);
        call_oldhandler(sig);
    }
}


static void temp_handler(int sig, siginfo_t *si, void *unused){
    if(si != NULL && si->si_addr != NULL){
        pagemap_t *pagenode, *tmp;
        void *pageptr;
        long offset =0;
        pageptr = si->si_addr;


        HASH_ITER(hh, pagemap, pagenode, tmp) {
            offset = pageptr - pagenode->pageptr;
            if(offset >=0 && offset <= pagenode->size){ // the adress belong to this chunk.
                if(isDebugEnabled()){
                    printf("[%d] starting address of the matching chunk %p variable name %s\n",
                           lib_process_id,pagenode->pageptr,pagenode->varname);
                }
                if(pagenode != NULL) {
                    if (mprotect(pagenode->pageptr,page_size,PROT_WRITE) == -1){
                        handle_error("mprotect");
                    }
                }
                return;
            }
        }
        debug("[%d] offending memory access : %p ",lib_process_id,pageptr);
        call_oldhandler(sig);
    }
}



/*
 * remove page protection on monitored chunks and
 * restore original SIGSEGV signal handler
 */
void stop_page_tracking(){
    pagemap_t *s;
    for(s=pagemap;s!=NULL;s=s->hh.next){
        debug("disabling protection of %s , page size : %d ",s->varname,page_size);
        disable_protection(s->pageptr,page_size);
    }
    debug("memory tracking disabled\n");
    install_old_handler();
    return;
}


/*
 * we are piggy backing the enabling page protection on memory chunks
 * within page fault itself.
 */
void protect_all_other_pages(char *varname) {
    pagemap_t *s;

    for(s=pagemap;s!=NULL;s=s->hh.next){
        if(!strncmp(s->varname,varname,20)){
            continue;
        }
        enable_write_protection(s->pageptr,page_size);
        //debug("enabling page protection except for variable %s\n",s->varname);
    }

}

void enable_write_protection(void *ptr, size_t size) {
    if (mprotect(ptr, size,PROT_READ) == -1){
        handle_error("mprotect");
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
    pagemap_t *s;
    DIR* dir = opendir("stats");

    if(dir){
        snprintf(file_name,sizeof(file_name),"stats/variable_access.log");
        fp=fopen(file_name,"a+");
        for(s=pagemap;s!=NULL;s=s->hh.next){
            fprintf(fp,"%s ,%lu ,%lu\n",s->varname,s->end_timestamp.tv_sec, s->end_timestamp.tv_usec);
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
int end_time_sort(pagemap_t * a, pagemap_t *b){
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



void decide_checkpoint_split(listhead_t *head,long long freemem) {
    pagemap_t *s;
    entry_t *np;
    int i;
    long page_aligned_size;

    if(cr_type == ONLINE_CR){
        freemem = freemem/2; // double in memory checkpointing
    }
    //lets sort the hashmap
    HASH_SORT(pagemap,end_time_sort);
    //map after sorting
    for(s=pagemap;s!=NULL;s=s->hh.next){

        if(s->paligned_size < freemem){ // it fits in
            for(np = head->lh_first,i=0; np != NULL; np = np->entries.le_next,i++){
                if(!strncmp(s->varname,np->var_name,20)){
                    np->type = DRAM_CHECKPOINT;
                    page_aligned_size = ((np->size+page_size-1)& ~(page_size-1));
                    if(cr_type == ONLINE_CR){
                        debug("allocated remote DRAM pointers for variable %s",np->var_name);
                        np->local_ptr = remote_alloc(&np->rmt_ptr,page_aligned_size);
                    }else if(cr_type == TRADITIONAL_CR){
                        //TODO
                    }
                    debug("variable : %s  chosen for DRAM checkpoint\n",s->varname);
                }
            }
        }

    }


}
