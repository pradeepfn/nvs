#ifndef __PX_DEDUP_H
#define __PX_DEDUP_H


static void dedup_handler(int sig, siginfo_t *si, void *unused); 
int* get_pvector(char *key); /* return the dedup vector */

#endif
