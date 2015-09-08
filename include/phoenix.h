#ifndef __PHOENIX_H
#define __PHOENIX_H

int init_c(int proc_id, int nproc);
int finalize_c(void);
void *alloc_c(char *var_name, size_t size, size_t commit_size,int process_id);
void afree_c(void *ptr);
void chkpt_all_c(int process_id);

/*interface for FORTRAN binding*/
int init(int *proc_id, int *nproc);
int finalize(void);
void* alloc(unsigned int* n, char *s, int *iid, int *cmtsize); 
void afree(void* ptr); 
void chkpt_all(int *process_id);

#endif
