#!/usr/bin/env python
# a bar plot with errorbars
import csv
import os
import numpy as np

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

color = ['#de2d26','#fc9272','#fee0d2']

__tmpfs = 'tmpfs'
__memcpy = 'memcpy'
__pmfs = 'pmfs'
__nvs = 'nvs'
__dnvs = 'dnvs'


llist = []
llist.append(__tmpfs)
llist.append(__memcpy)
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
    N=1 # number of bar groups
    ind = np.arange(N)

    start = 1.0
    width = 0.2
    ind = [start + x*width for x in range(0,5)]

    rects1 = ax.bar(ind, tuple(y[0]), width,yerr=None,linewidth=0.1)
    legend_l.append(rects1)



    # add some text for labels, title and axes tick
    ax.set_ylabel('normalized snapshot time (s)' , fontsize = '6')
    #ax.set_xlabel('applications',fontsize=6)
    #ax.set_ylim(0,1.2)
    #ax.set_xticks(ind+ width)
    #ax.margins(0.1,0)

    #ax.set_axisbelow(True)
    #ax.tick_params(axis='both', which='major', labelsize=6)



if __name__ == '__main__':

    pp = PdfPages('gtc-write.pdf')

    fig, (ax) = plt.subplots(nrows=1, ncols=1,figsize=(3.5,1.5));
   

    y=[]

    nth = [4]

    snum = 4 #rank to get the sampling data

    for idx1,n in enumerate(nth):
        tlist = []
        for idx2,item in enumerate(llist):
            location = '../gtc/results/' + item + '/gtc_' + item +'_t' + str(n)
            file = 'store_3.txt'
            tlist.append(snap_time(location,file))

        y.append(tlist)


    print y
    bar_plot(ax, y)

    plt.tight_layout(h_pad=0)
    #plt.subplots_adjust(top=0.98, bottom=0.18)
    pp.savefig(figure=fig)
    pp.close()

