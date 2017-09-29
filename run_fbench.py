#!/usr/bin/env python
import argparse
import sys
import os
import shutil

DBG=1




__home = os.getcwd()
__fbench_root = '/home/pradeep/checkout/nvm-yuma/yuma/bench-yuma'  # root of fbench script location

__chunk_size = ['64' '256']

__empty = ''
__newfile = 'newfile'
__bigfile = 'bigfile'
__mmap = 'mmap'

__workload_l = []
__workload_l.append(__newfile)
__workload_l.append(__bigfile)
__workload_l.append(__mmap)




parser = argparse.ArgumentParser(prog="runscript", description="script to run yuma")
parser.add_argument('-w', dest='workload', default=__empty , help='', choices=__workload_l)

try:
    args = parser.parse_args()

except:
    sys.exit(0)

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

    msg(cmd)
    try:
        os.system(cmd)
    except:
        print 'invalid cmd', cmd
        sys.exit(0)



def fb(wl, iosize):

    cmd = 'filebench'
    cmd +=  ' -f ' + __fbench_root + '/' + wl
    msg(cmd)
    sh(cmd)

    cd(__home)


if __name__ == '__main__':

    w = args.workload

    if w == __newfile:
        for i in __chunk_size:
            fb('write_newfile.f', i)
    elif w == __bigfile:
        for i in __chunk_size:
            fb('write_bigfile.f', i)
    elif w == __mmap:
        print "invalid"
    else:
        sys.exit(0)
