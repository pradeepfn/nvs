#include "mpi.h"
#include "px_util.h"
#include "px_remote.h"


/*
 * Implements the functions that make use of MPI lib. This seperation allow
 */


void split_checkpoint_data(var_t *list) {
    var_t *s;
    int i;
    long page_aligned_size;
    long page_size = sysconf(_SC_PAGESIZE);

    for(s = list,i=0; s != NULL; s = s->hh.next,i++){
        if(i<split_ratio){
            s->type = DRAM_CHECKPOINT;
            page_aligned_size = ((s->size+page_size-1)& ~(page_size-1));
            if(lib_process_id == 0) {
                log_info("[%d] variable : %s  chosen for DRAM checkpoint\n",
                         lib_process_id, s->varname);
            }
            if(cr_type == ONLINE_CR){
                debug("[%d] allocated remote DRAM pointers for variable %s",
                      lib_process_id , s->varname);
                s->local_remote_ptr = remote_alloc(&s->remote_ptr,page_aligned_size);
            }
        }
    }
}