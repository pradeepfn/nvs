#!/usr/bin/env python
# a bar plot with errorbars
import csv
import os
import numpy as np

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

color = ['#de2d26','#fc9272','#fee0d2']

__nocheckpoint = 'nocheckpoint'
__tmpfs = 'tmpfs'
__memcpy = 'memcpy'
__pmfs = 'pmfs'
__nvs = 'nvs'
__dnvs = 'dnvs'

__cmemcpy = '#fee090'
__ctmpfs = '#636363'
__cpmfs = '#ce1256'
__cnvs = '#2ca25f'
__cdnvs = '#2ca25f'



llist = []
llist.append(__memcpy)
llist.append(__tmpfs)
llist.append(__pmfs)
llist.append(__nvs)
llist.append(__dnvs)

def sh(cmd):

    #msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)


def snap_time(location, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "iteration snapshot time" ' +  location + '/' + dfile + ' | awk \'{$1=$2=$3=$4="";print $0}\' >' + dfile

    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().strip().split()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    ave_t = sum(time_l)/len(time_l)
    return ave_t



legend_l=[]
def bar_plot(ax,y):
    start = 1.0
    width = 0.2

    l_ticks = [x[0] for x in y]
    l_tmpfs = [float(x[1])/1000 for x in y]
    l_pmfs = [float(x[2])/1000 for x in y]
    l_nvs = [float(x[3])/1000 for x in y]

    print l_tmpfs
    print l_pmfs
    print l_nvs

    ind = np.arange(len(l_ticks))
    rects1 = ax.bar(ind, tuple(l_tmpfs), width,yerr=None,linewidth=0.1,color=__ctmpfs)
    rects2 = ax.bar(ind+width, tuple(l_pmfs), width,yerr=None,linewidth=0.1, color = __cpmfs)
    rects3 = ax.bar(ind+2*width, tuple(l_nvs), width,yerr=None,linewidth=0.1, color = __cnvs)


    legend_l.append(rects1)
    legend_l.append(rects2)
    legend_l.append(rects3)

    # add some text for labels, title and axes tick
    ax.set_ylabel('time(milli-sec)' , fontsize = '6')
    ax.set_xlabel('GTC variables',fontsize=6)

    ax.set_xticks(ind+ ((width*3)/2))
    ax.set_xticklabels(tuple(l_ticks),rotation=35)

    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    ax.tick_params(axis='x',which='both',top='off',bottom=False,labelsize='6')
    ax.tick_params(axis='y',which='both',right='off',labelsize='6')
    ax.margins(0.1,0)




if __name__ == '__main__':

    pp = PdfPages('gtc-read.pdf')

    fig, (ax) = plt.subplots(nrows=1, ncols=1,figsize=(3.5,1.4));


    y=[]

    with open('gtc-read-input.txt') as f:
        content = f.readlines()
    content = [y.append(x.strip().split()) for x in content]

    bar_plot(ax, y)

    plt.legend( (legend_l[0][1], legend_l[1][1], legend_l[2][1] ),
                ('tmpfs', 'pmfs', 'nvs'),
                fontsize='6',ncol=1,bbox_to_anchor=(1, 1))

    plt.tight_layout(h_pad=0)
    #plt.subplots_adjust(top=0.98, bottom=0.18)
    pp.savefig(figure=fig)
    pp.close()

