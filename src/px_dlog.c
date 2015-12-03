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


int dlog_write(dlog_t *dlog, var_t *list,int process_id,long version, dim_type type);

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
int dlog_remote_write(dlog_t *dlog, var_t *list,int process_id,long version) {
    var_t *s = NULL;
    int status;

    dlog->is_dlog_remote_valid = 0;
    //remote DRAM write
    //copy each local variable to remote peer..
    for (s = list; s != NULL; s = s->hh.next) {
        if (s->type == DRAM_CHECKPOINT) {
            status = remote_write(s->ptr, s->remote_ptr, s->size);
            if (status) {
                printf("Error: failed variable copy to peer node\n");
            }
        }
    }
    remote_barrier();
    status =  dlog_write(dlog, list,process_id,version, DOUBLE_IN_MEMORY_REMOTE);
    dlog->dlog_remote_checkpoint_version = version;
    dlog->is_dlog_remote_valid = 1;
    return status;
}

/*
 * writes the selected (marked as REMOTE) vairables to the local DRAM
 * memory and indext it in a the hash map structure.
 * This is the local checkpoint procedure of the double in memory checkpoint
 */
int dlog_local_write(dlog_t *dlog, var_t *list,int process_id,long version){
    //invalidate the current data. we assume ordered commits from the CPU
    dlog->is_dlog_valid = 0;
    int status = dlog_write(dlog, list,process_id, version, DOUBLE_IN_MEMORY_LOCAL);
    dlog->dlog_checkpoint_version = version;
    dlog->is_dlog_valid=1;
    return status;
}


int dlog_write(dlog_t *dlog, var_t *list,int process_id,long version, dim_type type) {
    var_t *np;
    var_t *s;




    //iterate the list
    for (np = list; np != NULL; np = np->hh.next) {
        if(np->type != DRAM_CHECKPOINT){
            continue;
        }
        //first we check wether this entry already in our hash table
        HASH_FIND_STR(dlog->map[type], np->varname, s);
        if (s == NULL) { //create a new entry
            s = (var_t *) malloc(sizeof(var_t));
            strncpy(s->varname, np->varname, 20);
            s->process_id = np->process_id;
            s->size = np->size;
            s->version = version;

            if (type == DOUBLE_IN_MEMORY_LOCAL) {
                void *data_ptr = malloc(np->size);// allocate local DRAM memory
                s->ptr = data_ptr;
                if(isDebugEnabled()){
                    printf("[%d] new DRAM memory location for : %s \n",lib_process_id, s->varname);
                }
                local_dram_checkpoint_size+=s->size;

            } else if (type == DOUBLE_IN_MEMORY_REMOTE) {
                s->ptr = np->local_remote_ptr;// we use the group allocated memory directly
                if(isDebugEnabled()){
                    printf("[%d] remote variable mapped to ARMCI group allocated pointer for : "
                                   "%s \n",lib_process_id, s->varname);
                }
                remote_dram_checkpoint_size+=s->size;
            }
            HASH_ADD_STR(dlog->map[type], varname, s); //now we have a complete data entry for a variable.add it!
        }
        // we either created a s or, found older version from our hash map
        s->version = version;

        if(type == DOUBLE_IN_MEMORY_LOCAL){
            memcpy(s->ptr, np->ptr, np->size);
            if(isDebugEnabled()){
                printf("[%d] dram local checkpoint : varname : %s , process_id :  %d , version : %ld ,size : %ld , "
                               "pointer : %p \n",lib_process_id, s->varname, process_id, s->version, s->size, s->ptr);
            }
        }
        if(type == DOUBLE_IN_MEMORY_REMOTE){ // nothing to do. we have already set the group pointer

        }
        // sanity check
        assert(strncmp(s->var_name, np->varname,np->size) == 0);
        assert(s->process_id == np->process_id);

    }

    return 0;
}


/*
 * return a checkpoint_t structure if the variable found in the
 * map. Else NULL will be return.
 */
var_t *dlog_read(dlog_t *dlog, char *var_name, int process_id, long version, checkpoint_type type) {
    if(!dlog->is_dlog_valid  || dlog->dlog_checkpoint_version != version){
        return  NULL;
    }


    var_t *tmap = NULL;
    var_t *s;
    /*select the remote/local map to traverse*/
    tmap = dlog->map[type];

    HASH_FIND_STR(tmap,var_name,s);
    if(s != NULL){
        assert(s->process_id == process_id && s->version == version);
    }
    return s; // s can be NULL or found value
}

