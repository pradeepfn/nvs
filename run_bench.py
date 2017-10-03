#!/usr/bin/env python
import argparse
import sys
import os
import shutil

DBG = 1

__home = os.getcwd()
__fbench_root = '/home/pradeep/checkout/nvm-yuma/yuma/bench-yuma'  # root of fbench script location

__empty = ''
__newfile = 'newfile'
__bigfile = 'bigfile'
__mmap = 'mmap'

__workload_l = []
__workload_l.append(__newfile)
__workload_l.append(__bigfile)
__workload_l.append(__mmap)

parser = argparse.ArgumentParser(prog="runscript", description="script to run yuma")
parser.add_argument('-w', dest='workload', default=__empty, help='', choices=__workload_l)
parser.add_argument('-s', dest='stepsize', default=__empty, help='')
parser.add_argument('-c', dest='chunksize', default=__empty, help='')
parser.add_argument('-t', dest='totalsize', default=__empty, help='')

try:
    args = parser.parse_args()

except:
    sys.exit(0)


def dbg(s):
    if DBG == 1:
        print s


def msg(s):
    print '\n' + '>>>' + s + '\n'


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
    msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)


def tonum(str):
    ch = str.strip()[-1]
    num = long(str[:-1])
    if (ch == 'k'):
        return num * 1024
    elif (ch == 'm'):
        return num * 1024 ** 2
    elif (ch == 'g'):
        return num * 1024 ** 3
    else:
        return num


def fb(wl, data):
    __t = wl + '.template'
    __out = wl + '.f'

    # generate workload file from template
    cd('bench-yuma')

    template = open(__t, "rt").read()

    with open(__out, "wt") as output:
        output.write(template % data)

    cd(__home)
    cmd = 'filebench'
    cmd += ' -f ' + __fbench_root + '/' + __out
    msg(cmd)
    sh(cmd)

    # delete the generated file
    os.remove(__fbench_root + '/' + __out)

    cd(__home)

def mmapb(mapsize, stepsize, chunksize):

    #clean /dev/shm
    try:
        os.remove('/dev/shm/yumamapbench')
    except:
        print '/dev/shm/yumamapbench does not exist'

    cd('mmapbench/build')
    cmd = './bench -t ' + mapsize + ' -s ' + stepsize + ' -c ' + chunksize
    sh(cmd)
    cd(__home)


if __name__ == '__main__':

    w = args.workload
    t = args.totalsize
    s = args.stepsize
    c = args.chunksize

    if w == __newfile:
        __chunk_size = c
        __step_size = s
        __nfiles = tonum(t) / tonum(s)
        msg("chunk size : " + __chunk_size + " step_size : " + __step_size + " nfiles : " + str(__nfiles))
        data = {"chunk_size": __chunk_size, "step_size": __step_size, "nfiles": __nfiles}
        fb('write_newfile', data)

    elif w == __bigfile:
        __chunk_size = c
        __step_size = s
        __nchunks = tonum(t) / tonum(c)
        data = {"chunk_size": __chunk_size, "nchunks": __nchunks}
        fb('write_bigfile', data)
    elif w == __mmap:
        mmapb(t, s, c)
    else:
        sys.exit(0)
