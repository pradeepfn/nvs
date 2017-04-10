#ifndef __PHOENIX_H
#define __PHOENIX_H

#include <stdint.h>



#ifdef __cplusplus
extern "C" {
#endif

	typedef struct px_obj_{
		void *data;
		unsigned long size;
		long version;
	}px_obj;

	int px_init(int proc_id);
	int px_create(char *key1, unsigned long size,px_obj *retobj);
	int px_get(char *key1, uint64_t version, px_obj *retobj);
	int px_deltaget(char *key1,uint64_t version, px_obj *retobj);
	int px_commit(char *key1,int version);
	int px_snapshot();
	int px_get_snapshot();
	int px_delete(char *key1);
	int px_finalize();

#ifdef __cplusplus
}
#endif

#endif
