#!/usr/bin/env python
import os
import sys
import argparse
import math
import numpy as np

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

DBG = 1
color = ['#d73027', '#fc8d59' ,'#fee090', '#ffffbf', '#e0f3f8', '#91bfdb', '#4575b4' ]

__ctmpfs = '#636363'
__cmemcpy = '#4575b4'
__cpmfs = '#ce1256'
__cnvs = '#d73027'


#color codes for each of the strategies

__memcpy = 'memcpy'
__tmpfs = 'tmpfs'
__pmfs = 'pmfs'
__nvs = 'nvs'

_result_h = '../data/micro-results'


workload_l=[]
workload_l.append(__memcpy)
workload_l.append(__tmpfs)
workload_l.append(__pmfs)
workload_l.append(__nvs)

__empty = 'empty'

__32k = '32k'
__64k = '64k'
__512k = '512k'
__1m = '1m'
__10m = '10m'
__50m = '50m'
__100m = '100m'

gone_l = []
gone_l.append(__32k)
gone_l.append(__64k)
gone_l.append(__512k)
gone_l.append(__1m)
gone_l.append(__10m)
gone_l.append(__50m)
gone_l.append(__100m)

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

def time(file,tmpfile):
    ave_t = 0.0
    cmd = 'grep "average put_all time" ' + file +' | awk \'{print $6}\' >' + tmpfile
    sh(cmd)

    with open(tmpfile) as f:
        time_l=f.read().splitlines()
    os.remove(tmpfile)

    time_l = [float(x) for x in time_l]
    print time_l
    if(len(time_l) == 5):
        time_l.sort()
        ave_t = (time_l[1] + time_l[2] + time_l[3]) /3
        return ave_t
    elif len(time_l) == 0:
        return 0.0
    else:
        for x in time_l:
            ave_t += float(x)
        return ave_t/len(time_l)

glegnd_l= []
def plot(ax, y_l):
    N=7
    ind = np.arange(1,N+1)

    rects1 = ax.plot(ind, tuple(y_l[0]),  color = __ctmpfs, linewidth=1,ms=5,marker = 's',markerfacecolor="None",markeredgecolor= __ctmpfs)
    rects2 = ax.plot(ind , tuple(y_l[1]), color = __cmemcpy, linewidth=1,ms=7,marker = '*',markerfacecolor="None",markeredgecolor= __cmemcpy)
    rects3 = ax.plot(ind , tuple(y_l[2]), color = __cpmfs, linewidth=1,ms=5,marker = 's')
    rects4 = ax.plot(ind , tuple(y_l[3]),  color = __cnvs, linewidth=1,ms=5 ,marker = 'D',markerfacecolor="None",markeredgecolor= __cnvs)

    ax.set_ylabel('time (microsec)' , fontsize = '6')
    ax.set_xlabel('varibale size', fontsize='7')
    #ax.yaxis.grid(True)
    #ax.set_xticklabels(tick_l, rotation=45)
    ax.tick_params(labelsize=6)
    ax.set_xlim(0.8, 7.2)
    ax.set_xticks([p for p in ind])
    ax.set_xticklabels(['32k','64k' , '512k' , '1m', '10m' , '50m', '100m'])

    glegnd_l.append(rects1)
    glegnd_l.append(rects2)
    glegnd_l.append(rects3)
    glegnd_l.append(rects4)


if __name__ == '__main__':

    time_l = []

    for idx, item in enumerate(workload_l):
        tmp_l = []
        for idx2, item2 in enumerate(gone_l):
            loc = _result_h + '/' +item + '/' + item + '_v' + item2 + '_s1g.txt'
            msg(loc)
            tmp_l.append(time(loc,item2))

        time_l.append(tmp_l)



    print time_l
    pp = PdfPages('microresults.pdf')
    fig, (ax1) = plt.subplots(nrows=1, ncols=1,figsize=(3,1.5))

    plot(ax1,time_l)


    print glegnd_l
    plt.legend(
        (glegnd_l[0][0],
         glegnd_l[1][0],
         glegnd_l[2][0],
         glegnd_l[3][0]),
        ('tmpfs','memcpy','pmfs','nvs'),
        fontsize='6',ncol=2)


    plt.yscale('log')

    plt.tight_layout(h_pad=0)
    pp.savefig(figure=fig)
    pp.close()
