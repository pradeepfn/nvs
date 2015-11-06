//
// Created by pradeep on 9/17/15.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "px_dlog.h"
#include "px_debug.h"
#include "px_remote.h"
#include "px_constants.h"

extern int lib_process_id;

int dlog_write(dlog_t *dlog, listhead_t *lhead,int process_id,long version, dim_type type);

/*
 * This is a dram based implementation of the volatile log. Responsible for keeping
 * both local and remote checkpoint data in memory.
 */

void dlog_init(dlog_t *dlog){
    dlog->map[NVRAM_CHECKPOINT] = NULL; // data destined to NVRAM
    dlog->map[DRAM_CHECKPOINT] = NULL;   // data destined to remote DRAM
}
/*
 * writes the selected (marked as REMOTE) variables to remote buddy nodes
 * DRAM memory and then index it in the hash map strucutre.
 * this is the remote checkpoint procedure of the double in memory checkpoint
 */
int dlog_remote_write(dlog_t *dlog, listhead_t *lhead,int process_id,long version) {
    entry_t *np = NULL;
    int status;

    dlog->is_dlog_remote_valid = 0;
    //remote DRAM write
    //copy each local variable to remote peer..
    for (np = lhead->lh_first; np != NULL; np = np->entries.le_next) {
        if (np->type == DRAM_CHECKPOINT) {
            status = remote_write(np->ptr, np->rmt_ptr, np->size);
            if (status) {
                printf("Error: failed variable copy to peer node\n");
            }
        }
    }
    remote_barrier();
    status =  dlog_write(dlog, lhead,process_id,version, DOUBLE_IN_MEMORY_REMOTE);
    dlog->dlog_remote_checkpoint_version = version;
    dlog->is_dlog_remote_valid = 1;
    return status;
}

/*
 * writes the selected (marked as REMOTE) vairables to the local DRAM
 * memory and indext it in a the hash map structure.
 * This is the local checkpoint procedure of the double in memory checkpoint
 */
int dlog_local_write(dlog_t *dlog, listhead_t *lhead,int process_id,long version){
    //invalidate the current data. we assume ordered commits from the CPU
    dlog->is_dlog_valid = 0;
    int status = dlog_write(dlog,lhead,process_id, version, DOUBLE_IN_MEMORY_LOCAL);
    dlog->dlog_checkpoint_version = version;
    dlog->is_dlog_valid=1;
    return status;
}

extern long local_dram_checkpoint_size;
extern long remote_dram_checkpoint_size;

int dlog_write(dlog_t *dlog, listhead_t *lhead,int process_id,long version, dim_type type) {
    entry_t *np;
    dcheckpoint_map_entry_t *s;




    //iterate the list
    for (np = lhead->lh_first; np != NULL; np = np->entries.le_next) {
        if(np->type != DRAM_CHECKPOINT){
            continue;
        }
        //first we check wether this entry already in our hash table
        HASH_FIND_STR(dlog->map[type], np->var_name, s);
        if (s == NULL) { //create a new entry
            s = (dcheckpoint_map_entry_t *) malloc(sizeof(dcheckpoint_map_entry_t));
            strncpy(s->var_name, np->var_name, 20);
            s->process_id = np->process_id;
            s->size = np->size;
            s->version = version;

            if (type == DOUBLE_IN_MEMORY_LOCAL) {
                void *data_ptr = malloc(np->size);// allocate local DRAM memory
                s->data_ptr = data_ptr;
                if(isDebugEnabled()){
                    printf("[%d] new DRAM memory location for : %s \n",lib_process_id, s->var_name);
                }
                local_dram_checkpoint_size+=s->size;

            } else if (type == DOUBLE_IN_MEMORY_REMOTE) {
                s->data_ptr = np->local_ptr;// we use the group allocated memory directly
                if(isDebugEnabled()){
                    printf("[%d] remote variable mapped to ARMCI group allocated pointer for : %s \n",lib_process_id, s->var_name);
                }
                remote_dram_checkpoint_size+=s->size;
            }
            HASH_ADD_STR(dlog->map[type], var_name, s); //now we have a complete data entry for a variable.add it!
        }
        // we either created a s or, found older version from our hash map
        s->version = version;

        if(type == DOUBLE_IN_MEMORY_LOCAL){
            memcpy(s->data_ptr, np->ptr, np->size);
            if(isDebugEnabled()){
                printf("[%d] dram local checkpoint : varname : %s , process_id :  %d , version : %ld ,"
                               "size : %ld , pointer : %p \n",lib_process_id, s->var_name, process_id, s->version, s->size, s->data_ptr);
            }
        }
        if(type == DOUBLE_IN_MEMORY_REMOTE){ // nothing to do. we have already set the group pointer

        }
        // sanity check
        assert(strncmp(s->var_name, np->var_name,np->size) == 0);
        assert(s->process_id == np->process_id);

        /*if(type == DOUBLE_IN_MEMORY_REMOTE){

            printf("[%d] data vlaue ******************  : %f \n",lib_process_id, ((float *)s->data_ptr)[0]);
        }*/
    }

    return 0;
}


/*
 * return a checkpoint_t structure if the variable found in the
 * map. Else NULL will be return.
 */
dcheckpoint_map_entry_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, long version, checkpoint_type type) {
    if(!dlog->is_dlog_valid  || dlog->dlog_checkpoint_version != version){
        return  NULL;
    }


    dcheckpoint_map_entry_t *tmap = NULL;
    dcheckpoint_map_entry_t *s;
    /*select the remote/local map to traverse*/
    tmap = dlog->map[type];

    HASH_FIND_STR(tmap,var_name,s);
    if(s != NULL){
        assert(s->process_id == process_id && s->version == version);
    }
    return s; // s can be NULL or found value
}

