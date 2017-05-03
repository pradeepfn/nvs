#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <openssl/md5.h>

#include <iostream>
#include <string>

#include "px_util.h"
#include "px_debug.h"
#include "px_constants.h"
#include "px_types.h"

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

struct sigaction old_sa;

void install_sighandler(void (*sighandler)(int,siginfo_t *,void *));
void enable_write_protection(void *ptr, size_t size);
long get_nmodified(int *vector, long vsize);

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



extern rcontext_t runtime_context;


/* page write fault executes this hanlder,
 * we find the corresponding variable->page and mark it
 * as modified.
 */
static void dedup_handler(int sig, siginfo_t *si, void *unused){
	if(si != NULL && si->si_addr != NULL){
		var_t *s;
		void *pageptr;
		long offset =0;
		pageptr = si->si_addr;
		std::unordered_map<std::string, var_t *> *var_map = runtime_context.var_map;
		for (auto iter = var_map->begin(); iter != var_map->end(); ++iter) {
			s = iter->second;
			offset = (long)pageptr - (long)s->ptr;
			if(offset >=0 && offset <= s->paligned_size){ // the adress belong to this chunk.
				assert(s != NULL);

				// 1. find the page index
				// 2. update the dedup_vector
				// 3. disable the write protection

				long v_index = offset/PAGE_SIZE;
				s->dedup_vector[v_index] = 1;
				pageptr = (void *)((long)si->si_addr & ~(PAGE_SIZE-1));
				disable_protection(pageptr, PAGE_SIZE);
				return;
			}
		}
		//printf("offending address!!!!!\n");
		call_oldhandler(sig);
	}

}
/*
   static void dedup_handler(int sig, siginfo_t *si, void *unused){
   if(si != NULL && si->si_addr != NULL){
   var_t *s;
   void *pageptr;
   long offset =0;
   pageptr = si->si_addr;
   int pagesize = 4096;


   for(s=varmap;s != NULL;s=s->hh.next) {
   offset = pageptr - s->ptr;
   if(offset >=0 && offset <= s->paligned_size){ // the adress belong to this chunk.
// if(isDebugEnabled()){
//    printf("[%d] starting address of the matching chunk %p\n",lib_process_id, s->ptr);
//}
//get the page start
pageptr = (void *)((long)si->si_addr & ~(pagesize-1));
disable_protection(pageptr, pagesize);
// hopefully we dont acess this while signal handler being called
return;
}
}
printf("offending address!!!!!\n");
call_oldhandler(sig);
}
}
*/

/*
 *  * input char array is size of 20
 *   * we null terminate it at the first space
 *    */
char* null_terminate(char *c_string){
	int i;
	for(i=0;i<19;i++){
		if(c_string[i] == '\0'){return c_string;}
		if(c_string[i] == ' ' || i==10){ // zelectron0 hack for gtc
			c_string[i] = '\0';
			return c_string;
		}
	}
	c_string[i] = '\0';
	return c_string;
}

/* This function allocates a page aligned memory location and write protect it so we can track the
 *  * accesses of its chunks
 *   */

var_t *px_alighned_allocate(size_t size, char *key) {
	long page_aligned_size;
	int status, page_size;
	void *ptr;
	var_t *s;
	key = null_terminate(key); // !!! this logic is very brittle
	page_size = sysconf(_SC_PAGESIZE);

	page_aligned_size = ((size + page_size - 1) & ~(page_size - 1));
	status = posix_memalign(&ptr, page_size, page_aligned_size);
	// introduce a safe memalign??
	if (status != 0) {
		log_err("aligned memory allocation failure");
		exit(EXIT_FAILURE);
		return NULL;
	}

	memset(ptr, 0, page_aligned_size);

	s = (var_t *)SAFEMALLOC(sizeof(var_t));
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
	int *tmpptr = (int *) SAFEMALLOC(s->dv_size*sizeof(int));
	/* initialize the vector to all '1' */
	//int k;
	//for(k=0; k < s->dv_size ; k++){
	//	tmpptr[k] = 1;
	//}
	memset(tmpptr,0,s->dv_size*sizeof(int));
	s->dedup_vector = tmpptr;
	/*install dedup handler and enable write protection*/
	install_sighandler(dedup_handler);
	//enable_write_protection(s->ptr, s->paligned_size);
	s->mod_average = 0;
#endif
	return s;
}


/*
 * print the average modified pages for each variable
 */
void print_dedup_numbers(var_t *s, long iteration){
	long mod_pages = get_nmodified(s->dedup_vector, s->dv_size);
	float mod_frac = (float) mod_pages/s->dv_size;
	s->mod_average  = s->mod_average + (mod_frac - s->mod_average)/iteration;
	if(!runtime_context.process_id){
		printf(" %s , %.4f , %lu KB\n",s->key1, s->mod_average, s->dv_size*4);
	}
}

void install_sighandler(void (*sighandler)(int,siginfo_t *,void *)){
	struct sigaction sa;
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sighandler;
	if (sigaction(SIGSEGV, &sa, &old_sa) == -1){
		handle_error("install sighandler failed");
	}
}


void install_old_handler(){
	struct sigaction temp;
	debug("old signal handler installed");
	if (sigaction(SIGSEGV, &old_sa, &temp) == -1){
		handle_error("sigaction");
	}
}

void call_oldhandler(int signo){
	debug("old signal handler called");
	(*old_sa.sa_handler)(signo);
}

void enable_protection(void *ptr, size_t size) {
	if (mprotect(ptr, size,PROT_NONE) == -1){
		handle_error("read/write protection failed");
	}
}

void enable_write_protection(void *ptr, size_t size) {
	if (mprotect(ptr, size,PROT_READ) == -1){
		handle_error("write protection failed");
	}
}


/*
 * * Disable the protection of the whole aligned memory block related to each variable access
 * * return : size of memory
 * */
long disable_protection(void *page_start_addr,size_t aligned_size){
	if (mprotect(page_start_addr,aligned_size,PROT_WRITE) == -1){
		handle_error("disable page protection failed");
	}
	return aligned_size;
}






void read_configs(ccontext_t *config_context,char *file_path){
	char varname[30];
	char varvalue[32];// we are reading integers in to this

	//initialize with defaults
	config_context->log_size = 1024;
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
/*void md5_digest(unsigned char *digest,void *data, ulong length){
  MD5_CTX mdContext;

  MD5_Init(&mdContext);
  MD5_Update(&mdContext,data,length);
  MD5_Final(digest,&mdContext);
  return;
  }*/


/*
 * return the number of modifed pages since last commit
 */
long get_nmodified(int *vector, long vsize){
	long i;
	long npages=0;
	for(i=0; i<vsize;i++){
		if(vector[i] == 1){ npages++; }
	}
	return npages;
}

/*
 * return the variable size after dedup
 */
long get_varsize(int *vector, long vsize){
	return 4096*get_nmodified(vector, vsize);
}



/*
 * copy chunks to a destination log given, variable and dedup vector
 * return : total copied data in bytes
 */

long nvmmemcpy_dedupv(void *dest_ptr,void *var_ptr,long var_size,
		int *dvector, long dv_size, int nvram_bw){
	int i;
	int chunk_started=0;
	long chunk_size=0;
	long total_copied=0;
	void *src_ptr;
	for(i=0; i<dv_size;i++){
		if(!chunk_started){
			if(dvector[i]){
				chunk_started =1;
				src_ptr = (void *)((long)var_ptr + PAGE_SIZE*i);
				chunk_size++;
				if(i == dv_size-1){
					nvmmemcpy_write(dest_ptr,src_ptr,chunk_size*PAGE_SIZE,nvram_bw);
					total_copied += chunk_size*PAGE_SIZE;
					//we dont need bookkepping anymore
				}
			}else{
				continue;
			}
		}else if(chunk_started){
			if(dvector[i]){
				chunk_size++;
				if(i == dv_size-1){
					nvmmemcpy_write(dest_ptr,src_ptr,chunk_size*PAGE_SIZE,nvram_bw);
					total_copied += chunk_size*PAGE_SIZE;
					//we dont need bookkepping anymore
				}
			}else{ // write the chunk
				nvmmemcpy_write(dest_ptr,src_ptr,chunk_size*PAGE_SIZE,nvram_bw);
				total_copied += chunk_size*PAGE_SIZE;
				dest_ptr = (void *)((long)dest_ptr + chunk_size*PAGE_SIZE);
				chunk_started=0;
				chunk_size=0;
			}
		}
	}
	return total_copied;
}





/*
 * apply diff to a versioned object
 */

long nvmmemcpy_dedup_apply(void *ret_ptr,long size, void *var_ptr,long var_size,int *dvector,
		long dv_size){

	int i;
	int chunk_started=0;
	long chunk_size=0;
	long total_applied=0; // total applied data in bytes
	void *dest_ptr;

	//debug("base object size : %ld , stored object size : %ld", size, var_size);
	assert(size == var_size); // this is a useless check.

	for(i=0;i<dv_size;i++){
		if(!chunk_started){
			if(dvector[i]){
				chunk_started =1;
				dest_ptr = (void *)((long)ret_ptr + PAGE_SIZE*i);
				chunk_size++;
				if(i == dv_size-1){
					memcpy(dest_ptr,var_ptr,chunk_size*PAGE_SIZE);
					total_applied += chunk_size*PAGE_SIZE;
					//we dont need bookkepping anymore
				}
			}else{
				continue;
			}
		}else if(chunk_started){
			if(dvector[i]){
				chunk_size++;
				if(i == dv_size-1){
					memcpy(dest_ptr,var_ptr,chunk_size*PAGE_SIZE);
					total_applied += chunk_size*PAGE_SIZE;
					//we dont need bookkepping anymore
				}
			}else{ // write the chunk
				memcpy(dest_ptr,var_ptr,chunk_size*PAGE_SIZE);
				total_applied += chunk_size*PAGE_SIZE;
				var_ptr = (void *)((long)var_ptr + chunk_size*PAGE_SIZE);
				chunk_started=0;
				chunk_size=0;
			}
		}
	}
	return total_applied;

}












