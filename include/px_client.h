#ifndef __PHOENIX_H
#define __PHOENIX_H

#include <stdint.h>

typedef struct px_obj_{
	void *data;
	unsigned long size;
}px_obj;

int px_init(int proc_id);
int px_get(char *key1, uint64_t version, px_obj *retobj);
int px_get_snapshot();
int px_finalize();

#endif
