#ifndef __PHOENIX_H
#define __PHOENIX_H

void *alloc(char *var_name, size_t size, size_t commit_size,int process_id);
void chkpt_all(int process_id);

/*interface for FORTRAN binding*/
void* alloc_(unsigned int* n, char *s, int *iid, int *cmtsize); 
void afree_(char* arr); 
void chkpt_all_(int *process_id);

#endif
