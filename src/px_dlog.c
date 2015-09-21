//
// Created by pradeep on 9/17/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "px_dlog.h"
#include "px_checkpoint.h"
#include "px_debug.h"
#include "px_remote.h"


int dlog_write(dlog_t *dlog, listhead_t *lhead,int process_id, dim_type type);

/*
 * This is a dram based implementation of the volatile log. Responsible for keeping
 * both local and remote checkpoint data in memory.
 */

void dlog_init(dlog_t *dlog){
    dlog->map[LOCAL] = NULL;
    dlog->map[REMOTE] = NULL;
}
/*
 * writes the selected (marked as REMOTE) variables to remote buddy nodes
 * DRAM memory and then index it in the hash map strucutre.
 * this is the remote checkpoint procedure of the double in memory checkpoint
 */
int dlog_remote_write(dlog_t *dlog, listhead_t *lhead,int process_id) {
    entry_t *np = NULL;
    int status;
    //remote DRAM write
    //copy each local variable to remote peer..
    for (np = lhead->lh_first; np != NULL; np = np->entries.le_next) {
        if (np->type == REMOTE) {
            status = remote_write(np->ptr, np->rmt_ptr, np->size);
            if (status) {
                printf("Error: failed variable copy to peer node\n");
            }
        }
    }
    remote_barrier();
    return dlog_write(dlog, lhead,process_id, DOUBLE_IN_MEMORY_DIM_REMOTE);
}

/*
 * writes the selected (marked as REMOTE) vairables to the local DRAM
 * memory and indext it in a the hash map structure.
 * This is the local checkpoint procedure of the double in memory checkpoint
 */
int dlog_local_write(dlog_t *dlog, listhead_t *lhead,int process_id){
    return dlog_write(dlog,lhead,process_id, DOUBLE_IN_MEMORY_LOCAL);
}


int dlog_write(dlog_t *dlog, listhead_t *lhead,int process_id, dim_type type) {
    entry_t *np;
    dcheckpoint_map_entry_t *s;
    //iterate the list
    for (np = lhead->lh_first; np != NULL; np = np->entries.le_next) {
        if(np->type != REMOTE){
            continue;
        }
        //first we check wether this entry already in our hash table
        HASH_FIND_STR(dlog->map[type], np->var_name, s);
        if (s == NULL) {
            //create a new entry
            s = (dcheckpoint_map_entry_t *) malloc(sizeof(dcheckpoint_map_entry_t));
            strncpy(s->var_name, np->var_name, 20);
            s->process_id = np->process_id;
            if (type == DOUBLE_IN_MEMORY_LOCAL) {
                void *data_ptr = malloc(np->size);// allocate local DRAM memory
                s->data_ptr = data_ptr;
                memcpy(s->data_ptr, np->ptr, np->size);
            } else if (type == DOUBLE_IN_MEMORY_DIM_REMOTE) {
                s->data_ptr = np->local_ptr;// we use the group allocated memory directly
            }
            HASH_ADD_STR(dlog->map[type], var_name, s); //now we have a complete data entry for a variable.add it!
        }
        // sanity check
        assert(strncmp(s->var_name, np->var_name,np->size) == 0);
        assert(s->process_id == np->process_id);
    }
    return 0;
}


/*
 * return a checkpoint_t structure if the variable found in the
 * map. Else NULL will be return.
 */
dcheckpoint_map_entry_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, checkpoint_type type) {

    dcheckpoint_map_entry_t *tmap = NULL;
    dcheckpoint_map_entry_t *s;
    /*select the remote/local map to traverse*/
    tmap = dlog->map[type];

    HASH_FIND_STR(tmap,var_name,s);
    if(s != NULL){
        assert(s->process_id == process_id);
    }
    return s; // s can be NULL or found value
}