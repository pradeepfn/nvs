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
    cmd = 'grep "iteration snapshot time" ' +  location + '/' + dfile + ' | awk \'{$1=$2=$3=$4=$5="";print $0}\' >' + dfile

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

    ind = []
    for x in range(0,5):
        if(x==2):
            start+= 0.05
        temp = start + x*width
        ind.append(temp)



    y_list = [(x/y[0][0]) for x in y[0]]
    rects1 = ax.bar(ind, tuple(y_list), width,yerr=None,linewidth=0.1, color =[__cmemcpy,__ctmpfs,__cpmfs, __cnvs,__cdnvs])
    patterns = ('','','////','','\\\\\\\\')
    for bar, pattern in zip(rects1,patterns):
        bar.set_hatch(pattern)
    legend_l.append(rects1)



    # add some text for labels, title and axes tick
    ax.set_ylabel('normalized snapshot time' , fontsize = '6')
    ax.set_xlabel('I/O type',fontsize=6)
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    ax.tick_params(axis='x',which='both',top='off',bottom=False, labelbottom=False)
    ax.tick_params(axis='y',which='both',right='off',labelsize='6')
    ax.margins(0.1,0)




if __name__ == '__main__':

    pp = PdfPages('cm1-write.pdf')

    fig, (ax) = plt.subplots(nrows=1, ncols=1,figsize=(3.5,1.5));


    y=[]

    nth = [4]

    snum = 4 #rank to get the sampling data

    for idx1,n in enumerate(nth):
        tlist = []
        for idx2,item in enumerate(llist):
            location = '../CM1/results/' + item + '/cm1_' + item +'_t' + str(n)
            file = 'store_3.txt'
            tlist.append(snap_time(location,file))

        y.append(tlist)


    print y
    bar_plot(ax, y)

    plt.legend( (legend_l[0][0], legend_l[0][1], legend_l[0][2],
                 legend_l[0][3],legend_l[0][4]),
                ('memcpy', 'tmpfs', 'pmfs', 'nvs','dnvs'),
                fontsize='6',ncol=3,bbox_to_anchor=(1.05, 1.2))

    plt.tight_layout(h_pad=0)
    #plt.subplots_adjust(top=0.98, bottom=0.18)
    pp.savefig(figure=fig)
    pp.close()

