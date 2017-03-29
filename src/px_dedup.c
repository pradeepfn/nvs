#include "px_constants.h"
#include "px_debug.h"

extern 
extern


/* page write fault executes this hanlder,
 * we find the corresponding variable->page and mark it
 * as modified. 
 */
static void dedup_handler(int sig, siginfo_t *si, void *unused){
    if(si != NULL && si->si_addr != NULL){
        var_t *s;
        void *pageptr;
        long offset =0;
        pageptr = si->si_addr;

        for(s = varmap; s != NULL; s = s->hh.next){
            offset = pageptr - s->ptr;
            if (offset >= 0 && offset <= s->size) { // the adress belong to this chunk.
                assert(s != NULL);
				/* 
				 * 1. find the page index	 
				 * 2. update the dedup_vector 
				 * 3. disable the write protection
				 */
				long v_index = offset/PAGE_SIZE;
				s->dedup_vector[v_index] = 1;
				pageptr = (void *)((long)si->si_addr & ~(PAGE_SIZE-1));
                disable_protection(pageptr, PAGE_SIZE);
                return;
            }
        }
        debug("[%d] offending memory access : %p ",runtime_context.process_id,pageptr);
        call_oldhandler(sig);
    }

}



