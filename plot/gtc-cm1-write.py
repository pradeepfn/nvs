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


def snap_time(location, nth):
    gl=[]
    for n in np.arange(nth):
        dfile = 'store_'+ str(n) + '.txt'
        time_l = []
        ave_t = 0.0
        cmd = 'grep "iteration snapshot time" ' +  location + '/' + dfile + ' | awk \'{$1=$2=$3=$4=$5="";print $0}\' >' + dfile

        sh(cmd)

        with open(dfile) as f:
            time_l=f.read().strip().split()
        os.remove(dfile)
        time_l = [float(x) for x in time_l]
        gl.extend(time_l)

    #print time_l
    ave_t = sum(gl)/len(gl)
    return ave_t



legend_l=[]
def bar_plot(ax,y):
    N=1 # number of bar groups
    ind = np.arange(N)

    start = 1.0
    width = 0.4

    ind = []
    for x in range(0,len(y[0])):
        if(x==2):
            start+= 0.05
        temp = start + x*width
        ind.append(temp)

    group_start = temp
    xl=[]

    y_list = [(x/y[0][0]) for x in y[0]]
    xl.append(ind[0])
    rects1 = ax.bar(ind, tuple(y_list), width,yerr=None,linewidth=0.1, color =[__cmemcpy,__ctmpfs,__cpmfs, __cnvs,__cdnvs])
    patterns = ('','','////','','\\\\\\\\')
    for bar, pattern in zip(rects1,patterns):
        bar.set_hatch(pattern)

    ind = [x+group_start for x in ind]
    y_list = [(x/y[1][0]) for x in y[1]]
    xl.append(ind[0])
    rects2 = ax.bar(ind, tuple(y_list), width,yerr=None,linewidth=0.1, color =[__cmemcpy,__ctmpfs,__cpmfs, __cnvs,__cdnvs])
    patterns = ('','','////','','\\\\\\\\')
    for bar, pattern in zip(rects2,patterns):
        bar.set_hatch(pattern)

    ind = [x+group_start for x in ind]
    y_list = [(x/y[2][0]) for x in y[2]]
    xl.append(ind[0])
    rects3 = ax.bar(ind, tuple(y_list), width,yerr=None,linewidth=0.1, color =[__cmemcpy,__ctmpfs,__cpmfs, __cnvs,__cdnvs])
    patterns = ('','','////','','\\\\\\\\')
    for bar, pattern in zip(rects3,patterns):
        bar.set_hatch(pattern)

    legend_l.append(rects1)



    # add some text for labels, title and axes tick
    ax.set_ylabel('normalized snapshot time' , fontsize = '6')
    ax.set_xlabel('# of ranks',fontsize=6)
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    ax.set_xticks([x +(width*5/2) for x in xl])
    ax.set_xticklabels(('n=4', 'n=16','n=64'))

    ax.tick_params(axis='x',which='both',top='off',bottom=False,labelsize='6')
    ax.tick_params(axis='y',which='both',right='off',labelsize='6')
    ax.margins(0.1,0)




if __name__ == '__main__':

    pp = PdfPages('gtc-cm1-write.pdf')

    fig, (ax) = plt.subplots(nrows=2, ncols=1,figsize=(2.7,2.5));


    y=[]

    nth = [4,16,64]

    snum = 8 #rank to get the sampling data

    for idx1,n in enumerate(nth):
        tlist = []
        for idx2,item in enumerate(llist):
            location = '../gtc/results/' + item + '/gtc_' + item +'_t' + str(n)
            tlist.append(snap_time(location,n))

        y.append(tlist)


    print y[0]
    print y[1]
    print y[2]

    bar_plot(ax[0], y)


    y=[]

    for idx1,n in enumerate(nth):
        tlist = []
        for idx2,item in enumerate(llist):
            location = '../CM1/results/' + item + '/cm1_' + item +'_t' + str(n)
            tlist.append(snap_time(location,n))

        y.append(tlist)

    print ''
    print y[0]
    print y[1]
    print y[2]
    bar_plot(ax[1], y)


    ax[0].set_title('gtc',fontsize='6')
    ax[1].set_title('cm1',fontsize='6')

    plt.legend( (legend_l[0][0], legend_l[0][1], legend_l[0][2],
                 legend_l[0][3],
                 legend_l[0][4]
                 ),
                ('memcpy', 'tmpfs', 'pmfs', 'nvs','nvs+delta'),
                #('memcpy', 'tmpfs', 'pmfs', 'nvs'),
                fontsize='6',ncol=2,bbox_to_anchor=(.75, 2.75))

    plt.tight_layout(h_pad=0)
    #plt.subplots_adjust(top=0.98, bottom=0.18)
    pp.savefig(figure=fig)
    pp.close()

