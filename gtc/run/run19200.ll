## To submit the job:
## > llsubmit medium.bat

#@ job_name        = gtc19200IPM
#@ class           = debug 
#@ shell           = /usr/bin/csh
#@ account_no      = mpccc
#@ node            = 2400
#@ total_tasks     = 19200
#@ output          = 19200.out
#@ error           = 19200.err
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

cp gtc.input.19200p gtc.input

env IPM_REPORT=full ../gtcmpi



