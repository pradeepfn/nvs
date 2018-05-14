#!/usr/bin/env python
import os
import sys
import argparse
import numpy as np

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

DBG = 1

__home = os.getcwd()
_result_h = '../results'

fs = '16g'
ws = '1m'

cs = ['256', '1k' , '4k', '16k', '64k', '256k', '1m']

__empty = 'empty'

__tmpfsnewfile = 'tmpfsnewfile'
__tmpfsbigfile = 'tmpfsbigfile'

__newfile = 'newfile'
__bigfile = 'bigfile'

__mmap = 'mmap'
__mmapnoflush = 'mmapnoflush'
__mmapstream = 'mmapstream'


list = []
#list.extend((__tmpfsnewfile,__tmpfsbigfile, __newfile, __bigfile, __mmap, __mmapnoflush, __mmapstream))
list.extend((__tmpfsnewfile,__tmpfsbigfile, __newfile, __bigfile, __mmap, __mmapnoflush))


def dbg(s):
    if DBG==1:
        print s

def msg(s):
    print '\n' + '>>>' +s + '\n'

def cd(dirt):

    dbg(dirt)


    if dirt == __home:
        os.chdir(__home);
    else:

        path = __home + '/' + dirt
        dbg(path)

        try:
            os.chdir(path)
        except:
            print 'invalid directory ', path
            sys.exit(0)

def sh(cmd):

    #msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)

def time_append(loc, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "Time" ' + loc +'/'+ dfile +' | awk \'{print $3}\' >' + dfile
    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().splitlines()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    return min(time_l)
    #for x in time_l:
    #    ave_t += float(x)

    #dbg(time_l)
    #return ave_t/len(time_l)

def time_mem(loc, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "mb/s" ' + loc +'/'+ dfile +' | awk \'{print $3}\' >' + dfile
    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().splitlines()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    #return min(time_l)
    for x in time_l:
        ave_t += float(x)

    #dbg(time_l)
    return ave_t/len(time_l)


def time_fn(loc, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "writefile1" ' + loc +'/'+ dfile +' | awk \'{print substr($4, 1, length($4)-4)}\' >' + dfile
    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().splitlines()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    #return min(time_l)
    for x in time_l:
        ave_t += float(x)

    #dbg(time_l)
    return ave_t/len(time_l)

def time_fb(loc, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "write-file" ' + loc +'/'+ dfile +' | awk \'{print substr($4, 1, length($4)-4)}\' >' + dfile
    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().splitlines()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    #return min(time_l)
    for x in time_l:
        ave_t += float(x)

    #dbg(time_l)
    return ave_t/len(time_l)



if __name__ == '__main__':

    marker_l = ['+', 'x', '.', ',', '^', '*' , 'p']
    mi=0;

    pp = PdfPages('motivation.pdf')
    fig, (ax1) = plt.subplots(nrows=1, ncols=1,figsize=(3,1.5))
    legend = []
    for i in list:
        tmp_l = []

        for j in cs:
            d = _result_h + '/' + i;
            fname = i + '_f' + fs + '_s1m' + '_c' + j + '.txt'

            if i == __tmpfsbigfile or i == __bigfile:
               tmp_l.append(time_fb(d, fname))
            elif i == __tmpfsnewfile or i == __newfile:
               tmp_l.append(time_fn(d,fname))
            else:
                tmp_l.append(time_mem(d,fname))

        print tmp_l
        ret = ax1.semilogy(tmp_l, marker=marker_l[mi], linewidth=2)
        mi=mi+1

        legend.append(ret)
        ax1.set_xlabel('chunk size', fontsize='8')
        ax1.set_ylabel('throughput (mb/s)', fontsize='7')
        ax1.set_xticklabels(cs)
        ax1.tick_params(labelsize=7)



    plt.legend((legend[0][0], legend[1][0],legend[2][0],legend[3][0],legend[4][0],legend[5][0],
                #legend[6][0]
                ),
               ('tmpfs-newfile',
                'tmpfs-append',
                'pmfs-newfile',
                'pmfs-append',
                'memcpy+clflush',
                'mempy',
                #'stream+fence'
                ), fontsize='6',ncol=3, loc='upper center', bbox_to_anchor=(0.5, 1.38),frameon=False)

    #plt.tight_layout( pad=0.2, h_pad=1, w_pad=0.1)
    plt.subplots_adjust(left=0.2, right=0.8, top=0.8, bottom=0.22)
    pp.savefig(figure=fig)
    pp.close()

