#ifndef __PHOENIX_H
#define __PHOENIX_H

int px_init(int proc_id, int nproc);
int px_finalize(void);
void *alloc_c(char *varname, size_t size, size_t commit_size,int process_id);
void afree(void *ptr);
void app_snapshot();

/*interface for FORTRAN binding*/
int init_(int *proc_id, int *nproc);
int finalize_(void);
void* alloc(unsigned int* n, char *s, int *iid, int *cmtsize);
void afree_(void* ptr); 
void app_snapshot_();

#endif
