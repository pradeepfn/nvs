#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <openssl/md5.h>

#include "px_util.h"
#include "px_debug.h"
#include "px_constants.h"

// read bandwidth to constant maching
// 2048Mb/s -> 600
// 1024Mb/s -> 300
// 512Mb/s -> 150
// 256Mb/s -> 75
// 128Mb/s -> 38
// 64Mb/s ->19
// -1 relates to DRAM -> no delay


#define handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while (0)



unsigned long calc_delay_ns(size_t datasize,int bandwidth){
	unsigned long delay;
	double data_MB, sec;

	data_MB = (double)((double)datasize/(double)pow(10,6));
	sec =(double)((double)data_MB/(double)bandwidth);
	delay = sec * pow(10,9);
	return delay;
}

int __nsleep(const struct timespec *req, struct timespec *rem)
{
	struct timespec temp_rem;
	if(nanosleep(req,rem)==-1){
		__nsleep(rem,&temp_rem);
	}
	return 1;
}

int msleep(unsigned long nanosec)
{
	struct timespec req={0},rem={0};
	time_t sec=(int)(nanosec/1000000000);
	req.tv_sec=sec;
	req.tv_nsec=nanosec%1000000000;
	__nsleep(&req,&rem);
	return 1;
}


int nvmmemcpy_read(void *dest, void *src, size_t len, int rbw) {
	if(rbw > 0){
		unsigned long lat_ns;
		//read bandwidth is taken as the two times, write bandwidth
		lat_ns = calc_delay_ns(len,rbw);
		msleep(lat_ns);
	}
	memcpy(dest,src,len);
	return 0;
}

int nvmmemcpy_write(void *dest, void *src, size_t len, int wbw) {
	if(wbw > 0){
		unsigned long lat_ns;
		lat_ns = calc_delay_ns(len,wbw);
		msleep(lat_ns);
	}
	memcpy(dest,src,len);
	return 0;
}

/* This function allocates a page aligned memory location and write protect it so we can track the
 *  * accesses of its chunks
 *   */

var_t *px_alighned_allocate(size_t size, char *key) {
	long page_aligned_size;
	int status, page_size;
	void *ptr;
	var_t *s;

	page_size = sysconf(_SC_PAGESIZE);

	page_aligned_size = ((size + page_size - 1) & ~(page_size - 1));
	status = posix_memalign(&ptr, page_size, page_aligned_size);
	if (status != 0) {
		return NULL;
	}

	memset(ptr, 0, page_aligned_size);

	s = (var_t *)malloc(sizeof(var_t));
	s->ptr = ptr;
	s->size = size;
	s->paligned_size = page_aligned_size;
	s->process_id = 0;
	s->early_copied = 0;
	s->earlycopy_time_offset.tv_sec = 0;
	s->earlycopy_time_offset.tv_usec = 0;
	strncpy(s->key1,key,sizeof(char)*20);

#ifdef DEDUP
	s->dv_size= page_aligned_size/page_size;
	void *tmpptr = (int *) malloc(s->dv_size);
	memset(tmpptr,0,s->dv_size);
	s->dedup_vector = tmpptr;
	/*install dedup handler and enable write protection*/

#endif

	return s;
}



void read_configs(ccontext_t *config_context,char *file_path){
	char varname[30];
	char varvalue[32];// we are reading integers in to this

	//initialize with defaults
	config_context->log_size = 2*1024*1024;
	config_context->chunk_size = 4096;
	config_context->copy_strategy = 1;
	config_context->nvram_wbw = -1;
	config_context->restart_run = 0;
	config_context->remote_checkpoint = 0;
	config_context->remote_restart = 0;
	config_context->helper_core_size =0;
	config_context->buddy_offset = 1;
	config_context->split_ratio = 0;
	config_context->free_memory = -1;
	config_context->threshold_size = 4096;
	config_context->max_checkpoints = -1;
	config_context->early_copy_enabled = 0;
	config_context->ec_offset_add = 0;

	debug("reading phoenix config file : %s ", file_path);
	FILE *fp = fopen(file_path,"r");
	if(fp == NULL){
		char cwd[1024];
		if (getcwd(cwd, sizeof(cwd)) != NULL){
			log_info("Current working dir: %s\n", cwd);
		}
		log_err("Error while opening the configuration file\n");
		exit(1);
	}
	while (fscanf(fp, "%s =  %s", varname, varvalue) != EOF) { // TODO: space truncate
		if (varname[0] == '#') {
			// consuming the input line starting with '#'
			fscanf(fp, "%*[^\n]");
			fscanf(fp, "%*1[\n]");
			continue;
		} else if (!strncmp(NVM_SIZE, varname, sizeof(varname))) {
			config_context->log_size = atoi(varvalue) * 1024 * 1024;
		} else if (!strncmp(CHUNK_SIZE, varname, sizeof(varname))) {
			config_context->chunk_size = atoi(varvalue);
		} else if (!strncmp(COPY_STRATEGY, varname, sizeof(varname))) {
			config_context->copy_strategy = atoi(varvalue);
		} else if (!strncmp(DEBUG_ENABLE, varname, sizeof(varname))) {
			if (atoi(varvalue) == 1) {
				enable_debug();
			}
		} else if (!strncmp(PFILE_LOCATION, varname, sizeof(varname))) {
			strncpy(config_context->pfile_location, varvalue,
					sizeof(config_context-> pfile_location));
		} else if (!strncmp(LCKFILE_LOCATION, varname, sizeof(varname))) {
			strncpy(config_context->lckfile, varvalue,
					sizeof(config_context-> lckfile));
		} else if (!strncmp(NVRAM_WBW, varname, sizeof(varname))) {
			config_context->nvram_wbw = atoi(varvalue);
		} else if (!strncmp(RSTART, varname, sizeof(varname))) {
			config_context->restart_run = atoi(varvalue);
		} else if (!strncmp(REMOTE_CHECKPOINT_ENABLE, varname, sizeof(varname))) {
			if (atoi(varvalue) == 1) {
				config_context->remote_checkpoint = 1;
			}
		} else if (!strncmp(REMOTE_RESTART_ENABLE, varname, sizeof(varname))) {
			if (atoi(varvalue) == 1) {
				config_context->remote_restart = 1;
			}
		} else if (!strncmp(BUDDY_OFFSET, varname, sizeof(varname))) {
			config_context->buddy_offset = atoi(varvalue);
		} else if (!strncmp(SPLIT_RATIO, varname, sizeof(varname))) {
			config_context->split_ratio = atoi(varvalue);
		}  else if (!strncmp(FREE_MEMORY, varname, sizeof(varname))) {
			config_context->free_memory = atol(varvalue) * 1024 * 1024;
		} else if (!strncmp(THRESHOLD_SIZE, varname, sizeof(varname))) {
			config_context->threshold_size = atoi(varvalue);
		} else if (!strncmp(MAX_CHECKPOINTS, varname, sizeof(varname))){
			config_context->max_checkpoints = atol(varvalue);
		}else if (!strncmp(EARLY_COPY_ENABLED, varname, sizeof(varname))){
			config_context->early_copy_enabled = atoi(varvalue);
		} else if (!strncmp(EARLY_COPY_OFFSET, varname, sizeof(varname))){
			config_context->ec_offset_add = atol(varvalue);
		}else if (!strncmp(HELPER_CORES, varname, sizeof(varname))){
			config_context->helper_cores[config_context->helper_core_size++] = atoi(varvalue);
		} else{
			log_err("unknown varibale : %s  please check the config",varname);
			exit(1);
		}
	}
	fclose(fp);
}

/*
 * returns md5 hash of given data
 *
 * digest - char array with lenth MD5_DIGEST_LENGTH - 16
 */
void md5_digest(unsigned char *digest,void *data, ulong length){
	MD5_CTX mdContext;

	MD5_Init(&mdContext);
	MD5_Update(&mdContext,data,length);
	MD5_Final(digest,&mdContext);
	return;
}
