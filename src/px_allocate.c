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

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while (0)




void protect_all_other_pages(char *varname);

void enable_write_protection(void *ptr, size_t size);

static void access_monitor_handler(int sig, siginfo_t *si, void *unused);


pagemap_t *pagemap;
long page_size;

/* This function allocates a page aligned memory location and write protect it so we can track the
 * accesses of its chunks
 */

void *px_alighned_allocate(size_t size , char *varname){
    long page_aligned_size;
    int s;
    void *ptr;

    page_size = sysconf(_SC_PAGESIZE);
    /*
		for a page size of 4, and a data size of 5, then the aligned data size should be 8.
		0101 + 0011 = 1000,
		and then we cut out the lower two bits.
	*/
    page_aligned_size = ((size+page_size-1)& ~(page_size-1));
    //debug("mem aligned allocation %ld : %ld \n", size, page_aligned_size);
    s = posix_memalign(&ptr,page_size, page_aligned_size);
    if (s != 0){
        handle_error("posix memalign");
    }
    memset(ptr,0,page_aligned_size);

    install_sighandler(access_monitor_handler);
    enable_write_protection(ptr,page_size); // only protect one page
    pagemap_put(&pagemap,varname,ptr,NULL,size,page_aligned_size,NULL);
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
        pagemap_t *pagenode = pagemap_get(&pagemap,si->si_addr);
        if(!pagenode->started_tracking){
            gettimeofday(&(pagenode->start_timestamp),NULL);
            pagenode->started_tracking = 1;
            //debug("first page access for variable %s",pagenode->varname);
        }else{
            gettimeofday(&(pagenode->end_timestamp),NULL);

        }
        disable_protection(pagenode->pageptr,page_size);
        protect_all_other_pages(pagenode->varname);
    }
}

/*
 * remove page protection on monitored chunks and
 * restore original SIGSEGV signal handler
 */
void stop_monitoring(){
    pagemap_t *s;
    for(s=pagemap;s!=NULL;s=s->hh.next){
        disable_protection(s->pageptr,page_size);
    }
    //TODO: set original SIGSEGV handler
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
            enable_write_protection(s->pageptr,page_size);
            s->started_tracking = 0;
        }
        fflush(fp);
    }else{ // directory does not exist
        printf("Error: no stats directory found.\n\n");
        assert(0);
    }
}



