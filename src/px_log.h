#ifndef __PX_LOG_H
#define __PX_LOG_H

typedef long offset_t;

typedef struct checkpoint_t_{
    char var_name[20];
    int process_id;
    int version;
    offset_t data_size;
    offset_t prv_offset;
    offset_t offset;
}checkpoint_t;

typedef struct headmeta_t_{
    offset_t offset;
    struct timeval timestamp;
}headmeta_t;

typedef struct memmap_t_{
    void *map_file_ptr;
	char file_name[256];
    headmeta_t *head;
    checkpoint_t *meta;
}memmap_t;

typedef struct log_t_{
	memmap_t m[2];
	memmap_t *current;
	offset_t log_size;
	offset_t offset;
}log_t;


void log_init(log_t *, long, int);
int log_write(log_t *);
void log_read();

#endif
