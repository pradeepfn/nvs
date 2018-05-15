#!/usr/bin/env python
import os
import sys
import argparse
import math
import numpy as np

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

DBG = 1
#__cmemcpy = '#fee090'
#__ctmpfs = '#636363'
#__cpmfs = '#ce1256'
#__cnvs = '#2ca25f'
#__cdnvs = '#2ca25f'

__cmemcpy = '#fee090'
__ctmpfs = '#1f78b4'
__cpmfs = '#1f78b4'
__cnvs = '#33a02c'
__cdnvs = '#33a02c'

#color codes for each of the strategies

__tmpfs = 'tmpfs'
__memcpy = 'memcpy'
__pmfs = 'pmfs'
__nvs = 'nvs'
__dnvs = 'dnvs'

_result_h = '../data/micro-results'


llist = []
llist.append(__memcpy)
llist.append(__tmpfs)
llist.append(__pmfs)
llist.append(__nvs)
llist.append(__dnvs)

__empty = 'empty'

__32k = '32k'
__64k = '64k'
__128k = '128k'
__512k = '512k'
__1m = '1m'
__5m = '5m'
__10m = '10m'
__20m = '20m'
__100m = '100m'

gone_l = []
gone_l.append(__32k)
gone_l.append(__64k)
gone_l.append(__128k)
gone_l.append(__512k)
gone_l.append(__1m)
gone_l.append(__5m)
gone_l.append(__10m)
gone_l.append(__20m)
#gone_l.append(__100m)


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



def snap_time(fpath, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "iteration snapshot time" ' +  fpath + ' | awk \'{$1=$2=$3=$4=$5="";print $0}\' >' + dfile

    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().strip().split()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    ave_t = sum(time_l)/len(time_l)
    return ave_t

glegnd_l= []
def plot(ax, y_l):
    N=len(y_l[0])
    ind = np.arange(1,N+1)

    rects1 = ax.plot(ind, tuple(y_l[0]),  color = __cmemcpy, linewidth=1,ms=7,marker = '*')
    print y_l[0]
    rects2 = ax.plot(ind , tuple(y_l[1]), color = __ctmpfs, linewidth=1,ms=5,marker = 's',markerfacecolor="None",markeredgecolor=__ctmpfs)
    print y_l[1]
    rects3 = ax.plot(ind , tuple(y_l[2]), color = __cpmfs, linewidth=1,ms=4,marker = 'D',markerfacecolor="None", markeredgecolor=__cpmfs)
    print y_l[2]
    rects4 = ax.plot(ind , tuple(y_l[3]),  color = __cnvs, linewidth=1,ms=7,marker = '.')
    print y_l[3]
    #rects5 = ax.plot(ind , tuple(y_l[4]),  color = __cdnvs, linewidth=1,ms=7,marker = '.',markerfacecolor="None")

    ax.set_ylabel('time (microsec)' , fontsize = '7')
    ax.set_xlabel('variable size', fontsize='7')
    #ax.yaxis.grid(True)
    #ax.set_xticklabels(tick_l, rotation=45)
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    ax.tick_params(axis='x',which='both',top=False,labelsize='6')
    ax.tick_params(axis='y',which='both',right=False,labelsize='6')
    ax.set_xlim(0.8, 8.2)
    ax.set_xticks([p for p in ind])
    ax.set_xticklabels(['32k','64k' ,'128k', '512k' , '1m', '5m', '10m','20m'])
    plt.yscale('log')

    glegnd_l.append(rects1)
    glegnd_l.append(rects2)
    glegnd_l.append(rects3)
    glegnd_l.append(rects4)
   # glegnd_l.append(rects5)


if __name__ == '__main__':

    time_l = []

    for idx, item in enumerate(llist):
        tmp_l = []
        for idx2, item2 in enumerate(gone_l):
            loc = '../microbench/results/'+ item + '/micro_' + item + '_v' + item2 + '/store_0.txt'
            tmp_l.append(snap_time(loc,item2))

        time_l.append(tmp_l)




    pp = PdfPages('micro-write.pdf')
    fig, (ax1) = plt.subplots(nrows=1, ncols=1,figsize=(3.5,1.3))

    plot(ax1,time_l)



    plt.legend(
        (glegnd_l[0][0],
         glegnd_l[1][0],
         glegnd_l[2][0],
         glegnd_l[3][0],
        # glegnd_l[4][0]
         ),
        ('memcpy','tmpfs','pmfs','nvs'),
        fontsize='7',ncol=2,bbox_to_anchor=(1, 1)).get_frame().set_linewidth(0.1)


    plt.yscale('log')

    plt.tight_layout(rect = [0.01,-0.1,0.98,1.09],h_pad=0)
    pp.savefig(figure=fig)
    pp.close()
