## To submit the job:
## > llsubmit medium.bat

#@ job_name        = gtc64IPM
#@ class           = debug 
#@ shell           = /usr/bin/csh
#@ account_no      = mpccc
#@ node            = 8  
#@ total_tasks     = 64
#@ output          = 64.out
#@ error           = 64.err
#@ wall_clock_limit= 00:45:00
#@ job_type        = parallel
#@ network.MPI     = csss,not_shared,US
#@ rset           = RSET_MCM_AFFINITY
#@ mcm_affinity_options = mcm_distribute,mcm_mem_pref,mcm_sni_none
#@ notification    = always
#@ environment     = COPY_ALL
#@ checkpoint     = no
#@ queue

module load ipm

cp gtc.input.64p gtc.input

env IPM_REPORT=full ../gtcmpi



