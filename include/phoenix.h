#ifndef __PHOENIX_H
#define __PHOENIX_H

int init(int proc_id, int nproc);
int finalize(void);
void *alloc_c(char *varname, size_t size, size_t commit_size,int process_id);
void afree(void *ptr);
void chkpt_all(int process_id);

/*interface for FORTRAN binding*/
int init_(int *proc_id, int *nproc);
int finalize_(void);
void* alloc(unsigned int* n, char *s, int *iid, int *cmtsize);
void afree_(void* ptr); 
void chkpt_all_(int *process_id);

#endif
