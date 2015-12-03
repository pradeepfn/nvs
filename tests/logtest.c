#include "minunit.h"

#define PROCESS_ID 1



char *test_loginit(){
    printf("log init test\n");
    return 0;
}

char *test_logwrite(){
    return 0;
}

char *test_logread(){
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