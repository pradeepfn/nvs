//
// Created by pradeep on 9/17/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "px_dlog.h"
#include "px_checkpoint.h"



/*
 * This is a dram based implementation of the volatile log. Responsible for keeping
 * both local and remote checkpoint data in memory.
 */

void dlog_init(dlog_t *dlog){
    dlog->map[LOCAL] = NULL;
    dlog->map[REMOTE] = NULL;
}

int dlog_write(dlog_t *dlog, listhead_t *lhead, checkpoint_type type) {
    dcheckpoint_map_entry_t *tmap = NULL;
    entry_t *np;
    dcheckpoint_map_entry_t *s;

    tmap = dlog->map[type];

    //iterate the list
    for(np = lhead->lh_first;np!=NULL;np=np->entries.le_next){
        if(np->type == REMOTE){ // this should be locally checkpointed in to DRAM
            //first we check wether this entry already in our hash table
            HASH_FIND_STR(tmap,np->var_name,s);
            if(s==NULL) {
                //create a new entry
                s = (dcheckpoint_map_entry_t) malloc(sizeof(dcheckpoint_map_entry_t));
                strncpy(s->var_name, np->var_name, 20);
                s->process_id = np->process_id;
                // allocate memory to hold data and copy data over
                void *data_ptr = malloc(np->size);
                s->data_ptr = data_ptr;
                //now we have a complete data entry for a variable.add it!
                HASH_ADD_STR(tmap, var_name, s);
            }
            // sanity check
            assert(strncmp(s->var_name,np->var_name) == 0);
            assert(s->process_id == np->process_id);

            memcpy(s->data_ptr,np->ptr,np->size);
        }
    }
    return 0;
}



/*
 * return a checkpoint_t structure if the variable found in the
 * map. Else NULL will be return.
 */
dcheckpoint_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, checkpoint_type type) {
    
    dcheckpoint_map_entry_t *tmap = NULL;
    dcheckpoint_map_entry_t *s;
    /*select the remote/local map to traverse*/
    tmap = dlog->map[type];

    HASH_FIND_STR(tmap,var_name,s);
    if(s != NULL){
        assert(s->process_id == np->process_id);
    }
    return s; // s can be NULL or found value
}