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
                disable_protection(s->ptr, s->page_size);
                return;
            }
        }
        debug("[%d] offending memory access : %p ",runtime_context.process_id,pageptr);
        call_oldhandler(sig);
    }

}



