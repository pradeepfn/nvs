#include "minunit.h"
#include "px_types.h"
#include "px_log.h"

#define PROCESS_ID 1



ccontext_t c_context;
rcontext_t r_context;
log_t nvlog;

char *test_loginit(){
    log_info("log init test\n");
    c_context.log_size = 300;
    snprintf(c_context.pfile_location,32,"/home/pradeep/temp");

    r_context.config_context = &c_context;
    nvlog.runtime_context = &r_context;

    long s = log_init(&nvlog,PROCESS_ID);
    return (char *)s;
}

char *test_logwrite(){
    int i;
    char data[50];
    log_info("writing to log file");
    for(i=0;i<4;i++){
        var_t var;
        var.size = 50;
        snprintf(var.varname,20,"var%d",i);
        snprintf(data,50,"data of var%d , version %ld",i,i);
        var.ptr = data;
        int s  = log_write(&nvlog,&var,PROCESS_ID,i);
        if(s==-1){
            return -1;
        }
        log_commitv(&nvlog,i);
    }
    return 0;
}

char *test_logread(){
    log_info("reading from log file");
    char varname[20];
    int i;
    for(i=3;i>=0;i--){
        snprintf(varname,20,"var%d",i);
        checkpoint_t *checkpoint = log_read(&nvlog,varname,PROCESS_ID,i);
        if(checkpoint == NULL){
            continue;
        }
        char *data = (char *)log_ptr(&nvlog,checkpoint->start_offset);
        log_info("%s , %d , %ld , %s ",checkpoint->var_name , checkpoint->process_id, checkpoint->version ,data);
    }
    return 0;
}


char *all_tests(){
    mu_suite_start();
    mu_run_test(test_loginit);
    mu_run_test(test_logwrite);
    mu_run_test(test_logread);

    return NULL;
}

RUN_TESTS(all_tests)