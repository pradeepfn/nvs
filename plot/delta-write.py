#!/usr/bin/env python
# a bar plot with errorbars
import csv
import os
import numpy as np

from matplotlib.backends.backend_pdf import PdfPages
import matplotlib.pyplot as plt

color = ['#e34a33','#fdbb84','#ece7f2']

__cmemcpy = '#fee090'
__ctmpfs = '#636363'
__cpmfs = '#ce1256'
__cnvs = '#2ca25f'
__cdnvs = '#2ca25f'

def sh(cmd):

    #msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)




def snap_size(location, dfile):
    time_l = []
    ave_t = 0.0
    cmd = 'grep "iteration snapshot size (MB) :"   ' +  location + '/' + dfile + ' | awk \'{$1=$2=$3=$4=$5="";print $0}\' >' + dfile

    sh(cmd)

    with open(dfile) as f:
        size_l=f.read().strip().split()
    os.remove(dfile)

    size_l = [float(x) for x in size_l]
    ave_s = sum(size_l)/len(size_l)
    return ave_s




legend_l=[]
def bar_plot(ax,y):

    ind = np.arange(len(y[0]))

    start = 1.0
    width = 0.25


    for idx, item in enumerate(y[0]):
        y[1][idx] = y[1][idx]/item
        y[0][idx] = y[0][idx]/item

    print y[0]
    print y[1]
    rects1 = ax.bar(ind, tuple(y[0]), width,yerr=None,linewidth=0.1, color =__cnvs)
    rects2 = ax.bar(ind+width, tuple(y[1]), width,yerr=None,linewidth=0.1, color =__cdnvs, hatch='\\\\\\\\')


    legend_l.append(rects1)
    legend_l.append(rects2)

    # add some text for labels, title and axes tick
    ax.set_ylabel('normalized snapshot size' , fontsize = '6')
    ax.set_xlabel('application',fontsize=6)
    ax.spines['right'].set_visible(False)
    ax.spines['top'].set_visible(False)

    ax.set_xticks(ind+ width)
    ax.set_xticklabels(('GTC','CM1','miniAMR'))

    ax.tick_params(axis='x',which='both',top='off',bottom=False,labelsize='6')
    ax.tick_params(axis='y',which='both',right='off',labelsize='6')
    ax.margins(0.1,0)


if __name__ == '__main__':

    pp = PdfPages('delta-write.pdf')
    fig, (ax) = plt.subplots(nrows=1, ncols=1,figsize=(3.5,1.5));

    wlist=['gtc', 'CM1', 'amr']
    y=[]
    n = 4

    for idx1,item2 in enumerate(['nvs','dnvs']):
        tlist = []
        for idx,item in enumerate(wlist):
            location = '../'+ item +'/results/' + item2 + '/'+  item.lower()  + '_' + item2.lower() +'_t' + str(n)
            file = 'store_3.txt'
            tlist.append(snap_size(location,file))

        y.append(tlist)


    print y[0]
    print y[1]
    bar_plot(ax,y)



    plt.tight_layout(h_pad=0)
    #plt.subplots_adjust(top=0.98, bottom=0.18)
    pp.savefig(figure=fig)
    pp.close()

