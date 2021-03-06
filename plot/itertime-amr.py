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

llist = []
llist.append(__nocheckpoint)
#llist.append(__memcpy)
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


def iter_time(location, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "iteration time (mirco-sec)" ' +  location + '/' + dfile + ' | awk \'{$1=$2=$3=$4=$5="";print $0}\' >' + dfile

    sh(cmd)

    with open(dfile) as f:
        time_l=f.read().strip().split()
    os.remove(dfile)

    time_l = [float(x) for x in time_l]
    return time_l



legend_l=[]
def line_plot(ax,y):
    N=len(y[0][0]) # number of bar groups
    ind = np.arange(N)
    pmfs_ind = np.arange(len(y[0][2]))
    rects1 = ax.plot(ind , tuple(y[0][0]), color = __cmemcpy, linewidth=1,ms=7,marker = '*')
    rects2 = ax.plot(ind , tuple(y[0][1]), color = __ctmpfs, linewidth=1,ms=5,marker = 's',markerfacecolor="None",markeredgecolor=__ctmpfs)
    rects3 = ax.plot(pmfs_ind , tuple(y[0][2]), color = __cpmfs, linewidth=1,ms=4,marker = 'D',markerfacecolor="None",markeredgecolor=__cpmfs)
    rects4 = ax.plot(ind , tuple(y[0][3]), color = __cnvs, linewidth=1,ms=7,marker = '.')
    rects5 = ax.plot(ind , tuple(y[0][4]), color = __cdnvs, linewidth=1,ms=7,marker = '.',markerfacecolor="None",markeredgecolor=__cdnvs)



    legend_l.append(rects1)
    legend_l.append(rects2)
    legend_l.append(rects3)
    legend_l.append(rects4)
    legend_l.append(rects5)



    # add some text for labels, title and axes tick
    ax.set_ylabel('time(milli-sec)' , fontsize = '7')
    ax.set_xlabel('iteration number',fontsize='7',labelpad=1)
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)
    ax.tick_params(axis='x',which='both',top='off',bottom=False,labelsize='6')
    ax.tick_params(axis='y',which='both',right='off',labelsize='6')
    ax.margins(0.1,0)
    plt.yscale('log')




if __name__ == '__main__':

    pp = PdfPages('itertime-amr.pdf')

    fig, (ax) = plt.subplots(nrows=1, ncols=1,figsize=(3.5,1.4));




    nth = [4]

    snum = 4 #rank to get the sampling data

    y=[]
    for idx1,n in enumerate(nth):
        tlist = []
        for idx2,item in enumerate(llist):
            location = '../amr/results/' + item + '/amr_' + item +'_t' + str(n)
            file = 'store_3.txt'
            tlist.append(iter_time(location,file))

        y.append(tlist)


    print y[0][0]
    print y[0][1]
    print y[0][2]
    print y[0][3]
    print y[0][4]
    #print y[0][5]
    line_plot(ax, y)


    plt.legend( (legend_l[0][0], legend_l[1][0], legend_l[2][0],
                 legend_l[3][0],legend_l[4][0]),
                ('nocheckpoint', 'tmpfs', 'pmfs', 'nvs','nvs+delta'),
                fontsize='6',ncol=3,bbox_to_anchor=(1.07, .38)).get_frame().set_linewidth(0.1)


    plt.tight_layout(rect = [0.01,-0.1,0.99,1.08],h_pad=0)
    #plt.subplots_adjust(top=0.98, bottom=0.18)
    pp.savefig(figure=fig)
    pp.close()

